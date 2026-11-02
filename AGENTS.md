# Agent notes

## Releases

- **Always include `SHASUMS256.txt` in every GitHub release.**
- The release workflow generates it with `sha256sum ClaudeX-* > SHASUMS256.txt`
  and uploads it next to the platform archives.
- The setup scripts in
  [JPTV-21/claude-code-portable](https://github.com/JPTV-21/claude-code-portable)
  verify downloads against this file; a missing checksum makes them fall back
  to an unverified install.
- If a release is missing the file, run the **Release checksums** workflow
  (`.github/workflows/checksums.yml`) for that tag.
