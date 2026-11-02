#!/usr/bin/env node
// Regenerates the ClaudeX icon assets from the master sprite (icon111.png).
//
// Usage: node tools/generate-icon.js [source.png] [--preview]
//
// The source must be a square, 8-bit, non-interlaced PNG with an alpha
// channel (color type 6, or color type 2 without transparency). Sizes below
// the source are box-filtered so small taskbar icons do not lose features;
// larger sizes are nearest-neighbor upscaled to keep the pixel-art look.
'use strict';

const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const ROOT = path.resolve(__dirname, '..');
const RESOURCES = path.join(ROOT, 'resources');
const DEFAULT_SOURCE = path.join(ROOT, 'icon111.png');
const ICO_SIZES = [16, 24, 32, 48, 64, 128, 256];

// ---------------------------------------------------------------------------
// PNG decoder (8-bit RGB/RGBA, no interlacing).
// ---------------------------------------------------------------------------
function decodePng(file) {
  const buffer = fs.readFileSync(file);
  const signature = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
  if (buffer.length < 8 || !buffer.subarray(0, 8).equals(signature)) {
    throw new Error(`${file}: not a PNG file`);
  }

  let offset = 8;
  let header = null;
  const idat = [];
  while (offset + 8 <= buffer.length) {
    const length = buffer.readUInt32BE(offset);
    const type = buffer.toString('ascii', offset + 4, offset + 8);
    const data = buffer.subarray(offset + 8, offset + 8 + length);
    if (type === 'IHDR') {
      header = {
        width: data.readUInt32BE(0),
        height: data.readUInt32BE(4),
        bitDepth: data[8],
        colorType: data[9],
        interlace: data[12],
      };
    } else if (type === 'IDAT') {
      idat.push(data);
    } else if (type === 'IEND') {
      break;
    }
    offset += 12 + length;
  }

  if (!header) {
    throw new Error(`${file}: missing IHDR`);
  }
  if (header.bitDepth !== 8) {
    throw new Error(`${file}: only 8-bit PNGs are supported (got ${header.bitDepth})`);
  }
  if (header.colorType !== 2 && header.colorType !== 6) {
    throw new Error(`${file}: expected RGB or RGBA PNG (got color type ${header.colorType})`);
  }
  if (header.interlace !== 0) {
    throw new Error(`${file}: interlaced PNGs are not supported`);
  }

  const { width, height, colorType } = header;
  const bpp = colorType === 6 ? 4 : 3;
  const stride = width * bpp;
  const raw = zlib.inflateSync(Buffer.concat(idat));
  const out = Buffer.alloc(stride * height);

  function paeth(a, b, c) {
    const p = a + b - c;
    const pa = Math.abs(p - a);
    const pb = Math.abs(p - b);
    const pc = Math.abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
  }

  let position = 0;
  for (let y = 0; y < height; ++y) {
    const filter = raw[position++];
    for (let x = 0; x < stride; ++x) {
      const value = raw[position++];
      const left = x >= bpp ? out[y * stride + x - bpp] : 0;
      const up = y > 0 ? out[(y - 1) * stride + x] : 0;
      const upLeft = y > 0 && x >= bpp ? out[(y - 1) * stride + x - bpp] : 0;
      let decoded;
      switch (filter) {
        case 0: decoded = value; break;
        case 1: decoded = value + left; break;
        case 2: decoded = value + up; break;
        case 3: decoded = value + ((left + up) >> 1); break;
        case 4: decoded = value + paeth(left, up, upLeft); break;
        default: throw new Error(`${file}: bad filter type ${filter}`);
      }
      out[y * stride + x] = decoded & 0xff;
    }
  }

  return { width, height, colorType, bpp, stride, data: out };
}

// ---------------------------------------------------------------------------
// Master sprite.
// ---------------------------------------------------------------------------
const sourceArg = process.argv.slice(2).find((arg) => !arg.startsWith('--'));
const sourcePath = sourceArg ? path.resolve(sourceArg) : DEFAULT_SOURCE;
const decoded = decodePng(sourcePath);
const { width: SIZE, height } = decoded;
if (SIZE !== height) {
  throw new Error(`${sourcePath}: icon must be square (got ${SIZE}x${height})`);
}

const pixels = Array.from({ length: SIZE }, () =>
  new Array(SIZE).fill(null)
);
for (let y = 0; y < SIZE; ++y) {
  for (let x = 0; x < SIZE; ++x) {
    const offset = y * decoded.stride + x * decoded.bpp;
    const alpha = decoded.bpp === 4 ? decoded.data[offset + 3] : 255;
    if (alpha === 0) {
      continue;
    }
    pixels[y][x] = [
      decoded.data[offset],
      decoded.data[offset + 1],
      decoded.data[offset + 2],
      alpha,
    ];
  }
}

function colorOf(value) {
  return value ? value : [0, 0, 0, 0];
}

function sourceAt(x, y) {
  if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
    return null;
  }
  return pixels[y][x];
}

// Nearest-neighbor copy, used for sizes >= SIZE.
function renderNearest(size) {
  const rgba = Buffer.alloc(size * size * 4);
  for (let y = 0; y < size; ++y) {
    for (let x = 0; x < size; ++x) {
      const sx = Math.min(SIZE - 1, Math.floor(((x + 0.5) * SIZE) / size));
      const sy = Math.min(SIZE - 1, Math.floor(((y + 0.5) * SIZE) / size));
      const color = colorOf(sourceAt(sx, sy));
      color.forEach((value, index) => {
        rgba[(y * size + x) * 4 + index] = value;
      });
    }
  }
  return rgba;
}

// Area-average downscale (premultiplied alpha), used for sizes below SIZE.
function renderBox(size) {
  const rgba = Buffer.alloc(size * size * 4);
  const scale = SIZE / size;
  for (let y = 0; y < size; ++y) {
    const y0 = y * scale;
    const y1 = Math.min(SIZE, (y + 1) * scale);
    for (let x = 0; x < size; ++x) {
      const x0 = x * scale;
      const x1 = Math.min(SIZE, (x + 1) * scale);
      let area = 0;
      let alpha = 0;
      let red = 0;
      let green = 0;
      let blue = 0;
      for (let sy = Math.floor(y0); sy < Math.ceil(y1); ++sy) {
        const wy = Math.min(y1, sy + 1) - Math.max(y0, sy);
        for (let sx = Math.floor(x0); sx < Math.ceil(x1); ++sx) {
          const wx = Math.min(x1, sx + 1) - Math.max(x0, sx);
          const weight = wx * wy;
          if (weight <= 0) {
            continue;
          }
          area += weight;
          const color = colorOf(sourceAt(sx, sy));
          const a = color[3] / 255;
          alpha += a * weight;
          red += color[0] * a * weight;
          green += color[1] * a * weight;
          blue += color[2] * a * weight;
        }
      }
      const offset = (y * size + x) * 4;
      if (alpha > 0) {
        rgba[offset] = Math.round(red / alpha);
        rgba[offset + 1] = Math.round(green / alpha);
        rgba[offset + 2] = Math.round(blue / alpha);
        rgba[offset + 3] = Math.round((alpha / area) * 255);
      }
    }
  }
  return rgba;
}

function render(size) {
  return size < SIZE ? renderBox(size) : renderNearest(size);
}

function colorKey(color) {
  return color ? color.join(',') : 'transparent';
}

function colorHex(color) {
  return (
    '#' +
    color
      .slice(0, 3)
      .map((value) => value.toString(16).padStart(2, '0'))
      .join('')
  );
}

// ---------------------------------------------------------------------------
// PNG and ICO writers (no external dependencies).
// ---------------------------------------------------------------------------
const crcTable = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; ++n) {
    let c = n;
    for (let k = 0; k < 8; ++k) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(buffer) {
  let crc = 0xffffffff;
  for (const byte of buffer) {
    crc = crcTable[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function chunk(type, data) {
  const typeBuffer = Buffer.from(type, 'ascii');
  const length = Buffer.alloc(4);
  length.writeUInt32BE(data.length, 0);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(Buffer.concat([typeBuffer, data])), 0);
  return Buffer.concat([length, typeBuffer, data, crc]);
}

function encodePngRgba(size, rgba) {
  const raw = Buffer.alloc((size * 4 + 1) * size);
  for (let y = 0; y < size; ++y) {
    raw[y * (size * 4 + 1)] = 0; // filter: none
    rgba.copy(raw, y * (size * 4 + 1) + 1, y * size * 4, (y + 1) * size * 4);
  }

  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(size, 0);
  ihdr.writeUInt32BE(size, 4);
  ihdr[8] = 8; // bit depth
  ihdr[9] = 6; // RGBA
  ihdr[10] = 0;
  ihdr[11] = 0;
  ihdr[12] = 0;

  return Buffer.concat([
    Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
    chunk('IHDR', ihdr),
    chunk('IDAT', zlib.deflateSync(raw, { level: 9 })),
    chunk('IEND', Buffer.alloc(0)),
  ]);
}

function encodePng(size) {
  return encodePngRgba(size, render(size));
}

// QA helper: flatten the transparent icon onto a solid background so contrast
// can be checked against a dark taskbar.
function compositeOn(size, hex) {
  const background = [
    parseInt(hex.slice(1, 3), 16),
    parseInt(hex.slice(3, 5), 16),
    parseInt(hex.slice(5, 7), 16),
  ];
  const source = render(size);
  const out = Buffer.alloc(source.length);
  for (let i = 0; i < source.length; i += 4) {
    const alpha = source[i + 3] / 255;
    for (let channel = 0; channel < 3; ++channel) {
      out[i + channel] = Math.round(
        source[i + channel] * alpha +
          background[channel] * (1 - alpha)
      );
    }
    out[i + 3] = 255;
  }
  return out;
}

// PNG-compressed entries are supported by Windows Vista and newer.
function encodeIco(sizes) {
  const images = sizes.map((size) => ({ size, png: encodePng(size) }));
  const header = Buffer.alloc(6);
  header.writeUInt16LE(0, 0);
  header.writeUInt16LE(1, 2);
  header.writeUInt16LE(images.length, 4);

  const entries = Buffer.alloc(16 * images.length);
  let offset = 6 + entries.length;
  images.forEach((image, index) => {
    const entry = 16 * index;
    entries[entry] = image.size >= 256 ? 0 : image.size;
    entries[entry + 1] = image.size >= 256 ? 0 : image.size;
    entries[entry + 2] = 0;
    entries[entry + 3] = 0;
    entries.writeUInt16LE(1, entry + 4);
    entries.writeUInt16LE(32, entry + 6);
    entries.writeUInt32LE(image.png.length, entry + 8);
    entries.writeUInt32LE(offset, entry + 12);
    offset += image.png.length;
  });

  return Buffer.concat([header, entries, ...images.map((image) => image.png)]);
}

function encodeSvg() {
  const runs = [];
  for (let y = 0; y < SIZE; ++y) {
    let x = 0;
    while (x < SIZE) {
      const color = pixels[y][x];
      let width = 1;
      while (
        x + width < SIZE &&
        colorKey(pixels[y][x + width]) === colorKey(color)
      ) {
        ++width;
      }
      if (color && color[3] > 0) {
        const opacity =
          color[3] < 255 ? ` fill-opacity="${(color[3] / 255).toFixed(3)}"` : '';
        runs.push(
          `<rect x="${x}" y="${y}" width="${width}" height="1" fill="${colorHex(color)}"${opacity}/>`
        );
      }
      x += width;
    }
  }
  return [
    `<svg xmlns="http://www.w3.org/2000/svg" width="${SIZE}" height="${SIZE}"`,
    ` viewBox="0 0 ${SIZE} ${SIZE}" shape-rendering="crispEdges">`,
    ...runs,
    '</svg>',
    '',
  ].join('\n');
}

fs.mkdirSync(RESOURCES, { recursive: true });
fs.writeFileSync(path.join(RESOURCES, 'claudex-icon.svg'), encodeSvg());
fs.writeFileSync(path.join(RESOURCES, 'claudex-icon.png'), encodePng(256));
fs.writeFileSync(
  path.join(RESOURCES, 'claudex.ico'),
  encodeIco(ICO_SIZES)
);

console.log(`Read ${path.relative(ROOT, sourcePath)} (${SIZE}x${SIZE})`);
console.log('Wrote resources/claudex-icon.svg');
console.log('Wrote resources/claudex-icon.png (256x256)');
console.log(`Wrote resources/claudex.ico (${ICO_SIZES.join(', ')})`);

// Optional QA previews of the downscaled sizes.
if (process.argv.includes('--preview')) {
  const buildDir = path.join(ROOT, 'build');
  fs.mkdirSync(buildDir, { recursive: true });
  for (const size of [16, 24, 32, 48]) {
    fs.writeFileSync(
      path.join(buildDir, `icon-preview-${size}.png`),
      encodePng(size)
    );
  }
  fs.writeFileSync(
    path.join(buildDir, 'icon-preview-16-nearest.png'),
    encodePngRgba(16, renderNearest(16))
  );
  fs.writeFileSync(
    path.join(buildDir, 'icon-preview-dark.png'),
    encodePngRgba(256, compositeOn(256, '#20242B'))
  );
  console.log('Wrote build/icon-preview-{16,24,32,48}.png');
  console.log('Wrote build/icon-preview-dark.png');
}
