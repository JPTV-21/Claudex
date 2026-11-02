#pragma once

#include "config_manager.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace claudex {

// Literal colors only: #rgb/#rrggbb (or bare 3/6 digit) hex and 0-255 palette
// indexes. Empty, "default", "none" and "transparent" mean "no color". Color
// names are defined by the theme's variables, not by this function.
std::optional<ftxui::Color> parseMenuColor(const std::string& spec);

// Resolves `spec` against the theme's named colors first, then the built-in
// names. `depth` bounds the chain so cycles cannot recurse forever.
std::optional<ftxui::Color> parseThemeColor(
    const std::string& spec, const std::map<std::string, std::string>* variables,
    int depth = 0);

enum class MenuMarkupKind {
    Color,
    Background,
    Bold,
    Dim,
    Italic,
    Underline,
    Inverted
};

struct MenuMarkupFrame {
    MenuMarkupKind kind = MenuMarkupKind::Color;
    std::string name;
    std::optional<ftxui::Color> color;
};

// Style carried by a `<line ...>` directive. Unlike the inline tags, a line
// directive applies to the whole row: `bg=` paints the row's full width, and
// the other attributes fill in defaults for text that does not set its own.
struct MenuLineStyle {
    std::optional<ftxui::Color> foreground;
    std::optional<ftxui::Color> background;
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
    bool inverted = false;
};

// Finds the first `<line ...>` directive in |text| and parses it. Returns
// nullopt when the line has no directive, or when any part of one is invalid
// (bad attribute name, unknown color), in which case it stays literal text.
std::optional<MenuLineStyle> parseMenuLineStyle(
    const std::string& text,
    const std::map<std::string, std::string>* variables = nullptr);

std::optional<MenuMarkupFrame> parseMenuMarkupTag(
    const std::string& raw, const std::map<std::string, std::string>* variables);
bool markupTagsMatch(const MenuMarkupFrame& open, const MenuMarkupFrame& close);

// Renders <tag>text</tag> markup. Unknown or unmatched tags stay literal.
// maxWidth > 0 truncates the styled result to that many cells and marks the
// cut with dots; the dots never push past the limit.
ftxui::Element renderMenuMarkup(
    const std::string& text,
    const std::map<std::string, std::string>* variables = nullptr,
    int maxWidth = 0);

// Display width of the text after markup tags are removed.
int menuMarkupDisplayWidth(
    const std::string& text,
    const std::map<std::string, std::string>* variables = nullptr);

using BorderCharset = std::array<std::string, 6>;

std::vector<std::string> splitUtf8Glyphs(std::string_view text);
std::optional<BorderCharset> parseBorderCharacters(const std::string& spec);
ftxui::Decorator borderWithCharset(const BorderCharset& charset);
ftxui::BorderStyle parseBorderStyle(const std::string& spec);
ftxui::Element applyMenuBorder(ftxui::Element element, const MenuTheme& theme);
std::optional<ftxui::Element> makeMenuSeparatorFor(
    const std::string& styleSpec, const std::string& characterSpec);
ftxui::Element alignMenuLine(ftxui::Element element,
                             const std::string& alignment);

// Expands {name} and <name> templates in menu text.
std::string expandMenuText(
    std::string text, const ConfigManager& config,
    const std::string& projectPath,
    const std::map<std::string, std::string>& extra = {});
// Commands run through a shell, so only brace placeholders are expanded.
std::string expandCommandText(std::string text, const ConfigManager& config,
                              const std::string& projectPath);

std::vector<std::string> splitMenuLines(const std::string& text);

ftxui::Element styleMenuEntry(
    const ftxui::EntryState& state, const std::string& label,
    bool labelHasMarkup, bool menuFocused, const std::string& activePrefix,
    const std::string& inactivePrefix, bool inverseHighlight,
    const std::map<std::string, std::string>* variables, int maxWidth = 0);

// Expands {path}/{lastUsed}/{name} (and the <...> spellings) in each entry
// line. The name is expanded last, so markup inside it is never expanded again.
// Lines that end up empty are dropped; an entirely empty entry falls back to
// the path.
std::vector<std::string> expandEntryLines(
    const std::vector<std::string>& lines, const std::string& name,
    const std::string& path, const std::string& lastUsed,
    const std::map<std::string, std::string>* variables);

// A multi-line menu entry: the prefix on the first line, indented continuation
// lines, and the inverse highlight across the whole block.
ftxui::Element styleMenuEntryLines(
    const ftxui::EntryState& state, const std::vector<std::string>& lines,
    bool labelHasMarkup, bool menuFocused, const std::string& activePrefix,
    const std::string& inactivePrefix, bool inverseHighlight,
    const std::map<std::string, std::string>* variables, int maxWidth = 0);

}  // namespace claudex
