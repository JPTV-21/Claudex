#include "menu_markup.hpp"

#include "platform.hpp"
#include "text_util.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <sstream>

using namespace ftxui;

namespace claudex {

std::optional<Color> parseMenuColor(const std::string& spec) {
    const std::string token = normalizeMenuToken(spec);
    if (token.empty() || token == "default" || token == "none" ||
        token == "transparent") {
        return std::nullopt;
    }

    const bool allDigits =
        std::all_of(token.begin(), token.end(), [](char c) {
            return std::isdigit(static_cast<unsigned char>(c)) != 0;
        });
    if (allDigits) {
        try {
            int index = std::stoi(token);
            if (index >= 0 && index <= 255) {
                return Color(static_cast<Color::Palette256>(index));
            }
        } catch (...) {
        }
    }

    const bool hadHash = token[0] == '#';
    std::string hex = hadHash ? token.substr(1) : token;
    if ((hadHash || hex.size() == 6 || hex.size() == 3) &&
        std::all_of(hex.begin(), hex.end(), [](char c) {
            return std::isxdigit(static_cast<unsigned char>(c)) != 0;
        })) {
        if (hex.size() == 3) {
            hex = {hex[0], hex[0], hex[1], hex[1], hex[2], hex[2]};
        }
        if (hex.size() == 6) {
            auto byte = [&](std::size_t offset) {
                return static_cast<std::uint8_t>(
                    std::stoi(hex.substr(offset, 2), nullptr, 16));
            };
            return Color(byte(0), byte(2), byte(4));
        }
    }

    // Names are not built in: the theme's variables table is the only source,
    // so a spec that reaches here is a literal color or nothing.
    return std::nullopt;
}

std::optional<Color> parseThemeColor(
    const std::string& spec,
    const std::map<std::string, std::string>* colors,
    int depth) {
    if (colors != nullptr && depth < 8) {
        const std::string token = normalizeMenuToken(spec);
        if (!token.empty()) {
            auto found = colors->find(token);
            if (found != colors->end()) {
                return parseThemeColor(found->second, colors, depth + 1);
            }
        }
    }
    return parseMenuColor(spec);
}

namespace {

std::optional<Color> parseBackgroundTag(
    const std::string& raw, const std::string& prefix,
    const std::map<std::string, std::string>* colors) {
    std::string token = lowerAscii(trim(raw));
    if (token.size() <= prefix.size() ||
        token.compare(0, prefix.size(), prefix) != 0) {
        return std::nullopt;
    }
    std::string rest = token.substr(prefix.size());
    while (!rest.empty() &&
           (rest.front() == '-' || rest.front() == '_' ||
            rest.front() == ' ' || rest.front() == ':')) {
        rest.erase(rest.begin());
    }
    if (rest.empty()) {
        return std::nullopt;
    }
    return parseThemeColor(rest, colors);
}

// `<line bg=green fg=black bold>`: the directive name must be followed by
// whitespace, so a theme variable that happens to be named "line" keeps
// working as an ordinary color tag.
std::optional<MenuLineStyle> parseMenuLineDirective(
    const std::string& raw,
    const std::map<std::string, std::string>* colors) {
    const std::string body = trim(raw);
    if (body.size() <= 4 || lowerAscii(body.substr(0, 4)) != "line" ||
        !isAsciiSpace(body[4])) {
        return std::nullopt;
    }

    MenuLineStyle style;
    bool any = false;
    std::istringstream stream(body.substr(4));
    std::string token;
    while (stream >> token) {
        const std::size_t equals = token.find('=');
        if (equals == std::string::npos) {
            const std::string key = normalizeMenuToken(token);
            if (key == "bold" || key == "b") {
                style.bold = true;
            } else if (key == "dim") {
                style.dim = true;
            } else if (key == "italic" || key == "i") {
                style.italic = true;
            } else if (key == "underline" || key == "underlined" ||
                       key == "u") {
                style.underline = true;
            } else if (key == "inverted" || key == "inverse") {
                style.inverted = true;
            } else {
                return std::nullopt;
            }
            any = true;
            continue;
        }

        const std::string key = normalizeMenuToken(token.substr(0, equals));
        const auto color = parseThemeColor(token.substr(equals + 1), colors);
        if (!color) {
            return std::nullopt;
        }
        if (key == "bg" || key == "background" || key == "on") {
            style.background = color;
        } else if (key == "fg" || key == "foreground" || key == "color") {
            style.foreground = color;
        } else {
            return std::nullopt;
        }
        any = true;
    }

    if (!any) {
        return std::nullopt;
    }
    return style;
}

bool isLineClosingTag(const std::string& body) {
    return normalizeMenuToken(body) == "line";
}

}  // namespace

std::optional<MenuLineStyle> parseMenuLineStyle(
    const std::string& text,
    const std::map<std::string, std::string>* variables) {
    for (std::size_t i = 0; i < text.size();) {
        const std::size_t open = text.find('<', i);
        if (open == std::string::npos) {
            break;
        }
        const std::size_t close = text.find('>', open + 1);
        if (close == std::string::npos) {
            break;
        }
        const std::string raw = text.substr(open + 1, close - open - 1);
        if (!raw.empty() && raw.front() != '/') {
            if (auto style = parseMenuLineDirective(raw, variables)) {
                return style;
            }
        }
        i = close + 1;
    }
    return std::nullopt;
}

std::optional<MenuMarkupFrame> parseMenuMarkupTag(
    const std::string& raw,
    const std::map<std::string, std::string>* colors) {
    const std::string token = normalizeMenuToken(raw);
    if (token.empty()) {
        return std::nullopt;
    }

    MenuMarkupFrame frame;
    frame.name = token;
    if (token == "bold" || token == "b") {
        frame.kind = MenuMarkupKind::Bold;
        return frame;
    }
    if (token == "dim") {
        frame.kind = MenuMarkupKind::Dim;
        return frame;
    }
    if (token == "italic" || token == "i") {
        frame.kind = MenuMarkupKind::Italic;
        return frame;
    }
    if (token == "underline" || token == "underlined" || token == "u") {
        frame.kind = MenuMarkupKind::Underline;
        return frame;
    }
    if (token == "inverted" || token == "inverse") {
        frame.kind = MenuMarkupKind::Inverted;
        return frame;
    }
    if (auto color = parseBackgroundTag(raw, "on", colors)) {
        frame.kind = MenuMarkupKind::Background;
        frame.color = color;
        return frame;
    }
    if (auto color = parseBackgroundTag(raw, "bg", colors)) {
        frame.kind = MenuMarkupKind::Background;
        frame.color = color;
        return frame;
    }
    if (auto color = parseThemeColor(raw, colors)) {
        frame.kind = MenuMarkupKind::Color;
        frame.color = color;
        return frame;
    }
    return std::nullopt;
}

bool markupTagsMatch(const MenuMarkupFrame& open,
                     const MenuMarkupFrame& close) {
    if (open.kind != close.kind) {
        return false;
    }
    if (open.kind == MenuMarkupKind::Color ||
        open.kind == MenuMarkupKind::Background) {
        return open.name == close.name;
    }
    return true;
}

namespace {

// One run of text with the style that was active when it was parsed.
struct MarkupSegment {
    std::string text;
    std::optional<Color> foreground;
    std::optional<Color> background;
    bool boldText = false;
    bool dimText = false;
    bool italicText = false;
    bool underlineText = false;
    bool invertedText = false;

    Element render() const {
        Element element = ftxui::text(text);
        if (foreground) {
            element = element | color(*foreground);
        }
        if (background) {
            element = element | bgcolor(*background);
        }
        if (boldText) {
            element = element | bold;
        }
        if (dimText) {
            element = element | dim;
        }
        if (italicText) {
            element = element | italic;
        }
        if (underlineText) {
            element = element | underlined;
        }
        if (invertedText) {
            element = element | inverted;
        }
        return element;
    }
};

int segmentWidth(const MarkupSegment& segment) {
    return ftxui::string_width(segment.text);
}

// Drops glyphs from the end until the segments fit, then marks the cut with
// dots that inherit the style of the segment they replace. The dots are clipped
// to the cell budget, so the result never exceeds maxWidth.
void truncateSegments(std::vector<MarkupSegment>& segments, int maxWidth) {
    int total = 0;
    for (const MarkupSegment& segment : segments) {
        total += segmentWidth(segment);
    }
    if (maxWidth <= 0 || total <= maxWidth) {
        return;
    }

    const int dotsWidth = std::min(3, maxWidth);
    const int budget = maxWidth - dotsWidth;

    std::vector<MarkupSegment> kept;
    int used = 0;
    MarkupSegment cutStyle;
    bool haveCutStyle = false;
    for (const MarkupSegment& segment : segments) {
        if (used >= budget) {
            if (!haveCutStyle) {
                cutStyle = segment;
                haveCutStyle = true;
            }
            break;
        }
        MarkupSegment copy = segment;
        copy.text.clear();
        for (const std::string& glyph : ftxui::Utf8ToGlyphs(segment.text)) {
            const int width = ftxui::string_width(glyph);
            if (used + width > budget) {
                break;
            }
            copy.text += glyph;
            used += width;
        }
        if (!haveCutStyle || copy.text.size() < segment.text.size()) {
            cutStyle = segment;
            haveCutStyle = true;
        }
        if (!copy.text.empty()) {
            kept.push_back(std::move(copy));
        }
        if (used >= budget || kept.size() < segments.size()) {
            break;
        }
    }

    MarkupSegment dots;
    dots.text = std::string(static_cast<std::size_t>(dotsWidth), '.');
    if (haveCutStyle) {
        dots.foreground = cutStyle.foreground;
        dots.background = cutStyle.background;
        dots.boldText = cutStyle.boldText;
        dots.dimText = cutStyle.dimText;
        dots.italicText = cutStyle.italicText;
        dots.underlineText = cutStyle.underlineText;
        dots.invertedText = cutStyle.invertedText;
    }
    kept.push_back(std::move(dots));
    segments = std::move(kept);
}

struct ParsedMenuMarkup {
    std::vector<MarkupSegment> segments;
    std::optional<MenuLineStyle> line;
};

ParsedMenuMarkup parseMenuMarkupSegments(
    const std::string& text,
    const std::map<std::string, std::string>* variables) {
    ParsedMenuMarkup parsed;
    std::vector<MarkupSegment>& segments = parsed.segments;
    std::string current;
    std::vector<MenuMarkupFrame> stack;

    auto flush = [&] {
        if (current.empty()) {
            return;
        }

        MarkupSegment segment;
        for (const MenuMarkupFrame& frame : stack) {
            switch (frame.kind) {
                case MenuMarkupKind::Color:
                    segment.foreground = frame.color;
                    break;
                case MenuMarkupKind::Background:
                    segment.background = frame.color;
                    break;
                case MenuMarkupKind::Bold:
                    segment.boldText = true;
                    break;
                case MenuMarkupKind::Dim:
                    segment.dimText = true;
                    break;
                case MenuMarkupKind::Italic:
                    segment.italicText = true;
                    break;
                case MenuMarkupKind::Underline:
                    segment.underlineText = true;
                    break;
                case MenuMarkupKind::Inverted:
                    segment.invertedText = true;
                    break;
            }
        }
        segment.text = std::move(current);
        current.clear();
        segments.push_back(std::move(segment));
    };

    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '<') {
            const std::size_t close = text.find('>', i + 1);
            if (close != std::string::npos) {
                const std::string raw =
                    text.substr(i + 1, close - i - 1);
                const bool closing = !raw.empty() && raw.front() == '/';
                const std::string body =
                    closing ? raw.substr(1) : raw;

                if (!closing) {
                    if (auto style = parseMenuLineDirective(body, variables)) {
                        // The first directive wins, matching
                        // parseMenuLineStyle().
                        if (!parsed.line) {
                            parsed.line = style;
                        }
                        i = close + 1;
                        continue;
                    }
                } else if (parsed.line && isLineClosingTag(body)) {
                    // `</line>` is optional; accept it so a closed directive
                    // does not leak into the visible text.
                    i = close + 1;
                    continue;
                }

                if (auto frame = parseMenuMarkupTag(body, variables)) {
                    if (closing) {
                        std::size_t match = stack.size();
                        while (match > 0) {
                            --match;
                            if (markupTagsMatch(stack[match], *frame)) {
                                flush();
                                stack.resize(match);
                                i = close + 1;
                                break;
                            }
                        }
                        if (i == close + 1) {
                            continue;
                        }
                    } else {
                        flush();
                        stack.push_back(*frame);
                        i = close + 1;
                        continue;
                    }
                }

                // Unknown or unmatched tags are literal text.
                current.append(text, i, close - i + 1);
                i = close + 1;
                continue;
            }
        }
        current.push_back(text[i]);
        ++i;
    }
    flush();

    if (parsed.line) {
        for (MarkupSegment& segment : parsed.segments) {
            if (parsed.line->foreground && !segment.foreground) {
                segment.foreground = parsed.line->foreground;
            }
            if (parsed.line->background && !segment.background) {
                segment.background = parsed.line->background;
            }
            segment.boldText = segment.boldText || parsed.line->bold;
            segment.dimText = segment.dimText || parsed.line->dim;
            segment.italicText = segment.italicText || parsed.line->italic;
            segment.underlineText =
                segment.underlineText || parsed.line->underline;
            segment.invertedText =
                segment.invertedText || parsed.line->inverted;
        }
    }
    return parsed;
}

std::string truncatePlainText(const std::string& text, int maxWidth) {
    if (maxWidth <= 0) {
        return text;
    }
    std::vector<MarkupSegment> segments;
    MarkupSegment segment;
    segment.text = text;
    segments.push_back(std::move(segment));
    truncateSegments(segments, maxWidth);
    std::string result;
    for (const MarkupSegment& kept : segments) {
        result += kept.text;
    }
    return result;
}

// A `<line ...>` row paints beyond the text: the trailing filler lets the
// background reach the row's full allotted width, including the prefix and any
// padding the menu reserves. The foreground is applied here as well so the
// prefix is colored too; inline tags inside the label still override it.
Element paintLineStyle(Element element, const MenuLineStyle* style) {
    if (style == nullptr) {
        return element;
    }
    if (style->foreground) {
        element = element | color(*style->foreground);
    }
    if (style->background) {
        element = hbox({std::move(element), filler()}) |
                  bgcolor(*style->background);
    }
    return element;
}

Element renderSegments(std::vector<MarkupSegment>& segments) {
    if (segments.empty()) {
        return ftxui::text("");
    }
    if (segments.size() == 1) {
        return segments.front().render();
    }
    Elements elements;
    elements.reserve(segments.size());
    for (const MarkupSegment& segment : segments) {
        elements.push_back(segment.render());
    }
    return hbox(std::move(elements));
}

}  // namespace

Element renderMenuMarkup(const std::string& text,
                         const std::map<std::string, std::string>* variables,
                         int maxWidth) {
    ParsedMenuMarkup parsed = parseMenuMarkupSegments(text, variables);
    truncateSegments(parsed.segments, maxWidth);
    return renderSegments(parsed.segments);
}

Element renderThemeText(const MenuTheme& theme, const std::string& text,
                        int maxWidth) {
    return renderMenuMarkup(text, &theme.variables, maxWidth);
}

void applyThemePlaceholder(InputOption& options, const MenuTheme& theme,
                           const std::string* placeholder) {
    const auto* variables = &theme.variables;
    options.transform = [variables, placeholder](InputState state) {
        if (state.is_placeholder && placeholder != nullptr) {
            Element element = renderMenuMarkup(*placeholder, variables);
            // Match the decorations the input applies to its own placeholder
            // so the cursor and clipping behave the same.
            element = std::move(element) | focusCursorBarBlinking | xflex |
                      frame | dim;
            if (state.focused) {
                element = std::move(element) | inverted;
            } else if (state.hovered) {
                element = std::move(element) | underlined;
            }
            return element;
        }
        if (state.is_placeholder) {
            state.element |= dim;
        }
        if (state.focused) {
            state.element |= inverted;
        } else if (state.hovered) {
            state.element |= underlined;
        }
        return state.element;
    };
}

int menuMarkupDisplayWidth(const std::string& text,
                           const std::map<std::string, std::string>* variables) {
    int width = 0;
    for (const MarkupSegment& segment :
         parseMenuMarkupSegments(text, variables).segments) {
        width += segmentWidth(segment);
    }
    return width;
}

std::vector<std::string> splitUtf8Glyphs(std::string_view text) {
    std::vector<std::string> glyphs;
    std::size_t index = 0;
    while (index < text.size()) {
        const auto lead = static_cast<unsigned char>(text[index]);
        std::size_t length = 1;
        if ((lead & 0xE0) == 0xC0) {
            length = 2;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
        }
        if (index + length > text.size()) {
            break;
        }
        glyphs.emplace_back(text.substr(index, length));
        index += length;
    }
    return glyphs;
}

std::optional<BorderCharset> parseBorderCharacters(const std::string& spec) {
    std::vector<std::string> glyphs = splitUtf8Glyphs(trim(spec));
    if (glyphs.size() != 1 && glyphs.size() != 6) {
        // Also accept whitespace between the characters:
        // "╭ ─ ╮ │ ╰ ╯".
        std::vector<std::string> tokens;
        std::istringstream stream(trim(spec));
        std::string token;
        while (stream >> token) {
            tokens.push_back(std::move(token));
        }
        if (tokens.size() != 1 && tokens.size() != 6) {
            return std::nullopt;
        }
        glyphs = std::move(tokens);
    }

    if (glyphs.size() == 1) {
        return BorderCharset{glyphs[0], glyphs[0], glyphs[0],
                             glyphs[0], glyphs[0], glyphs[0]};
    }
    // Accepted order: top-left, top, top-right, side, bottom-left,
    // bottom-right. BorderCharset stores corners first, then the edges:
    // top-left, top-right, bottom-left, bottom-right, horizontal, vertical.
    return BorderCharset{glyphs[0], glyphs[2], glyphs[4], glyphs[5],
                         glyphs[1], glyphs[3]};
}

namespace {

class CustomBorderNode final : public Node {
public:
    CustomBorderNode(Element child, BorderCharset charset)
        : Node(Elements{std::move(child)}), charset_(std::move(charset)) {}

    void ComputeRequirement() override {
        Node::ComputeRequirement();
        requirement_ = children_[0]->requirement();
        requirement_.min_x += 2;
        requirement_.min_y += 2;
        requirement_.focused.box.x_min++;
        requirement_.focused.box.x_max++;
        requirement_.focused.box.y_min++;
        requirement_.focused.box.y_max++;
    }

    void SetBox(Box box) override {
        Node::SetBox(box);
        box.x_min++;
        box.x_max--;
        box.y_min++;
        box.y_max--;
        children_[0]->SetBox(box);
    }

    void Render(Screen& screen) override {
        children_[0]->Render(screen);
        if (box_.x_min >= box_.x_max || box_.y_min >= box_.y_max) {
            return;
        }

        screen.at(box_.x_min, box_.y_min) = charset_[0];
        screen.at(box_.x_max, box_.y_min) = charset_[1];
        screen.at(box_.x_min, box_.y_max) = charset_[2];
        screen.at(box_.x_max, box_.y_max) = charset_[3];

        for (int x = box_.x_min + 1; x < box_.x_max; ++x) {
            Cell& top = screen.CellAt(x, box_.y_min);
            Cell& bottom = screen.CellAt(x, box_.y_max);
            top.character = charset_[4];
            bottom.character = charset_[4];
            top.automerge = true;
            bottom.automerge = true;
        }
        for (int y = box_.y_min + 1; y < box_.y_max; ++y) {
            Cell& left = screen.CellAt(box_.x_min, y);
            Cell& right = screen.CellAt(box_.x_max, y);
            left.character = charset_[5];
            right.character = charset_[5];
            left.automerge = true;
            right.automerge = true;
        }
    }

private:
    BorderCharset charset_;
};

}  // namespace

Decorator borderWithCharset(const BorderCharset& charset) {
    return [charset](Element child) {
        return std::make_shared<CustomBorderNode>(std::move(child), charset);
    };
}

BorderStyle parseBorderStyle(const std::string& spec) {
    const std::string token = normalizeMenuToken(spec);
    if (token == "dashed") return DASHED;
    if (token == "heavy") return HEAVY;
    if (token == "double") return DOUBLE;
    if (token == "rounded") return ROUNDED;
    if (token == "empty") return EMPTY;
    return LIGHT;
}

Element applyMenuBorder(Element element, const MenuTheme& theme) {
    if (const auto charset = parseBorderCharacters(theme.borderCharacters)) {
        return element | borderWithCharset(*charset);
    }

    const std::string style = normalizeMenuToken(theme.borderStyle);
    if (style.empty() || style == "none") {
        return element;
    }
    return element | borderStyled(parseBorderStyle(theme.borderStyle));
}

std::optional<Element> makeMenuSeparatorFor(
    const std::string& styleSpec, const std::string& characterSpec) {
    std::string character = trim(characterSpec);
    if (!character.empty()) {
        return separatorCharacter(character);
    }

    const std::string style = normalizeMenuToken(styleSpec);
    if (style == "none") {
        return std::nullopt;
    }
    if (style == "dashed") {
        return separatorDashed();
    }
    if (style == "heavy") {
        return separatorHeavy();
    }
    if (style == "double") {
        return separatorDouble();
    }
    if (style == "empty") {
        return separatorEmpty();
    }
    return separatorLight();
}

Element alignMenuLine(Element element, const std::string& alignment) {
    const std::string value = normalizeMenuToken(alignment);
    if (value == "center") {
        return element | hcenter;
    }
    if (value == "right") {
        return hbox({filler(), element});
    }
    return element;
}

std::string expandMenuText(
    std::string text, const ConfigManager& config,
    const std::string& projectPath,
    const std::map<std::string, std::string>& extra) {
    const std::string logPath = config.logFilePath().empty()
                                    ? std::string("disabled")
                                    : pathToUtf8(config.logFilePath());
    const std::string claude = config.claudePathText();
    const std::string root = pathToUtf8(config.rootDir());
    const std::string configPath = pathToUtf8(config.configFile());
    const std::string themePath = config.themeFile().empty()
                                      ? std::string("disabled")
                                      : pathToUtf8(config.themeFile());

    auto replaceTemplate = [&](const std::string& name,
                               const std::string& value) {
        replaceAll(text, "{" + name + "}", value);
        replaceAll(text, "<" + name + ">", value);
    };
    for (const auto& entry : extra) {
        replaceTemplate(entry.first, entry.second);
    }
    replaceTemplate("root", root);
    replaceTemplate("config", configPath);
    replaceTemplate("theme", themePath);
    replaceTemplate("log", logPath);
    replaceTemplate("claude", claude);
    replaceTemplate("project", projectPath);
    return text;
}

// Commands run through a shell, so only brace placeholders are expanded here.
// Angle-bracket templates would collide with shell redirection.
std::string expandCommandText(std::string text, const ConfigManager& config,
                              const std::string& projectPath) {
    const std::string logPath = config.logFilePath().empty()
                                    ? std::string("disabled")
                                    : pathToUtf8(config.logFilePath());
    const std::string claude = config.firstClaudePathText();
    replaceAll(text, "{root}", pathToUtf8(config.rootDir()));
    replaceAll(text, "{config}", pathToUtf8(config.configFile()));
    replaceAll(text, "{theme}",
               config.themeFile().empty()
                   ? std::string("disabled")
                   : pathToUtf8(config.themeFile()));
    replaceAll(text, "{log}", logPath);
    replaceAll(text, "{claude}", claude);
    replaceAll(text, "{project}", projectPath);
    replaceAll(text, "{path}", projectPath);
    return text;
}

std::vector<std::string> splitMenuLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (char c : text) {
        if (c == '\n') {
            if (!current.empty() && current.back() == '\r') {
                current.pop_back();
            }
            lines.push_back(std::move(current));
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty() && current.back() == '\r') {
        current.pop_back();
    }
    lines.push_back(std::move(current));
    return lines;
}

Element styleMenuEntry(
    const EntryState& state, const std::string& label, bool labelHasMarkup,
    bool menuFocused, const std::string& activePrefix,
    const std::string& inactivePrefix, bool inverseHighlight,
    const std::map<std::string, std::string>* variables, int maxWidth) {
    // The cursor belongs to the focused section only. Every other section
    // renders its selected row with the inactive prefix, so the whole screen
    // never shows more than one active row.
    const bool active = state.active && menuFocused;
    const std::string prefix = active ? activePrefix : inactivePrefix;

    // The prefix keeps its width; the label is truncated to whatever is left
    // so the dots land inside the cell rather than outside it.
    int labelWidth = 0;
    if (maxWidth > 0) {
        labelWidth = maxWidth - menuMarkupDisplayWidth(prefix, variables);
        if (labelWidth < 1) {
            labelWidth = 1;
        }
    }

    Element prefixElement = renderMenuMarkup(prefix, variables, maxWidth);
    Element labelElement =
        labelHasMarkup
            ? renderMenuMarkup(label, variables, labelWidth)
            : ftxui::text(truncatePlainText(label, labelWidth));
    Element element = hbox({std::move(prefixElement),
                            std::move(labelElement)});
    if (labelHasMarkup) {
        if (auto line = parseMenuLineStyle(label, variables)) {
            element = paintLineStyle(std::move(element), &*line);
        }
    }
    if (active && inverseHighlight) {
        element = element | inverted;
    }
    return element;
}

namespace {

// The value a template token expands to, or nullptr when the token is not one
// of ours and should stay literal.
const std::string* entryTokenValue(
    std::string_view token, const std::string& name, const std::string& path,
    const std::string& lastUsed, bool hasVisibleName, const std::string& empty) {
    if (token == "{path}" || token == "<path>") {
        return &path;
    }
    if (token == "{lastUsed}" || token == "<lastUsed>") {
        return &lastUsed;
    }
    if (token == "{name}" || token == "<name>") {
        return &name;
    }
    if (token == "{nameOrPath}" || token == "<nameOrPath>") {
        return hasVisibleName ? &name : &path;
    }
    if (token == "{pathIfNamed}" || token == "<pathIfNamed>") {
        return hasVisibleName ? &path : &empty;
    }
    if (token == "{pathIfUnnamed}" || token == "<pathIfUnnamed>") {
        return hasVisibleName ? &empty : &path;
    }
    return nullptr;
}

// One scan over the authored template. Tokens inserted for a substitution are
// appended verbatim and never visited again, which keeps markup inside a
// project name (or a path that happens to look like a token) literal.
std::vector<std::string> expandEntryLinesImpl(
    const std::vector<std::string>& lines, const std::string& name,
    const std::string& path, const std::string& lastUsed,
    const std::map<std::string, std::string>* variables,
    bool fallbackToPath) {
    const bool hasVisibleName =
        menuMarkupDisplayWidth(name, variables) > 0;
    const std::string empty;
    std::vector<std::string> expanded;
    for (const std::string& line : lines) {
        std::string text;
        text.reserve(line.size());
        std::size_t index = 0;
        while (index < line.size()) {
            if (line[index] == '{' || line[index] == '<') {
                const char closer = line[index] == '{' ? '}' : '>';
                const std::size_t end = line.find(closer, index + 1);
                if (end != std::string::npos) {
                    const std::string_view token(line.data() + index,
                                                 end - index + 1);
                    if (const std::string* value = entryTokenValue(
                            token, name, path, lastUsed, hasVisibleName,
                            empty)) {
                        text.append(*value);
                        index = end + 1;
                        continue;
                    }
                }
            }
            text.push_back(line[index]);
            ++index;
        }
        if (menuMarkupDisplayWidth(text, variables) == 0) {
            continue;
        }
        expanded.push_back(std::move(text));
    }
    if (expanded.empty() && fallbackToPath && !path.empty()) {
        expanded.push_back(path);
    }
    return expanded;
}

}  // namespace

std::vector<std::string> expandEntryLines(
    const std::vector<std::string>& lines, const std::string& name,
    const std::string& path, const std::string& lastUsed,
    const std::map<std::string, std::string>* variables) {
    return expandEntryLinesImpl(lines, name, path, lastUsed, variables, true);
}

std::vector<std::string> entryLinesFor(
    const std::vector<std::string>& focusedLines,
    const std::vector<std::string>& entryLines,
    const std::string& entryFormat, bool focused, const std::string& name,
    const std::string& path, const std::string& lastUsed,
    const std::map<std::string, std::string>* variables) {
    if (focused && !focusedLines.empty()) {
        std::vector<std::string> lines = expandEntryLinesImpl(
            focusedLines, name, path, lastUsed, variables, false);
        if (!lines.empty()) {
            return lines;
        }
    }
    if (!entryLines.empty()) {
        std::vector<std::string> lines = expandEntryLinesImpl(
            entryLines, name, path, lastUsed, variables, false);
        if (!lines.empty()) {
            return lines;
        }
    } else {
        const std::vector<std::string> fallback{entryFormat};
        std::vector<std::string> lines = expandEntryLinesImpl(
            fallback, name, path, lastUsed, variables, false);
        if (!lines.empty()) {
            return lines;
        }
    }
    if (!path.empty()) {
        return {path};
    }
    return {};
}

Element styleMenuEntryLines(
    const EntryState& state, const std::vector<std::string>& lines,
    bool labelHasMarkup, bool menuFocused, const std::string& activePrefix,
    const std::string& inactivePrefix, bool inverseHighlight,
    const std::map<std::string, std::string>* variables, int maxWidth) {
    if (lines.empty()) {
        return ftxui::text("");
    }

    const bool active = state.active && menuFocused;
    const std::string prefix = active ? activePrefix : inactivePrefix;
    const int prefixWidth = menuMarkupDisplayWidth(prefix, variables);
    int labelWidth = 0;
    if (maxWidth > 0) {
        labelWidth = std::max(1, maxWidth - prefixWidth);
    }

    Elements rows;
    rows.reserve(lines.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        Element label =
            labelHasMarkup
                ? renderMenuMarkup(lines[i], variables, labelWidth)
                : ftxui::text(truncatePlainText(lines[i], labelWidth));
        Element row;
        if (i == 0) {
            row = hbox({renderMenuMarkup(prefix, variables),
                        std::move(label)});
        } else {
            // Continuation lines line up under the label, not the cursor.
            row = hbox(
                {ftxui::text(std::string(static_cast<std::size_t>(
                                            std::max(0, prefixWidth)),
                                        ' ')),
                 std::move(label)});
        }
        if (labelHasMarkup) {
            if (auto line = parseMenuLineStyle(lines[i], variables)) {
                row = paintLineStyle(std::move(row), &*line);
            }
        }
        rows.push_back(std::move(row));
    }

    Element element =
        rows.size() == 1 ? rows.front() : vbox(std::move(rows));
    if (active && inverseHighlight) {
        element = element | inverted;
    }
    return element;
}

}  // namespace claudex
