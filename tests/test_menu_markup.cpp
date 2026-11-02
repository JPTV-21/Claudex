#include "test_framework.hpp"

#include "config_manager.hpp"
#include "palette.hpp"
#include "menu_markup.hpp"
#include "render_helper.hpp"
#include "text_util.hpp"

using namespace claudex;
using ftxui::Color;
using testfw::renderPlain;
using testfw::renderRaw;
using testfw::TempDir;

namespace {

bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

const char* kEsc = "\x1b[";

// Markup color names resolve through the theme palette, exactly like the
// launcher does; the shipped default theme is the source of those names.
ftxui::Element markup(const std::string& text, int maxWidth = 0) {
    return renderMenuMarkup(text, &testfw::defaultPalette(), maxWidth);
}

ftxui::Element markup(const std::string& text,
                      const std::map<std::string, std::string>* variables,
                      int maxWidth = 0) {
    return renderMenuMarkup(text, variables, maxWidth);
}

}  // namespace

// --- colors ------------------------------------------------------------------

TEST_CASE("markup/parseMenuColor has no built-in names") {
    // Color names live in the theme; the parser only understands literals.
    CHECK_FALSE(parseMenuColor("red").has_value());
    CHECK_FALSE(parseMenuColor("  Green ").has_value());
    CHECK_FALSE(parseMenuColor("Gray-Dark").has_value());
    CHECK_FALSE(parseMenuColor("grey").has_value());
    CHECK_FALSE(parseMenuColor("orange").has_value());
    CHECK_FALSE(parseMenuColor("BlueLight").has_value());
}

TEST_CASE("markup/the default theme resolves the usual color names") {
    const auto& palette = testfw::defaultPalette();
    CHECK(parseThemeColor("red", &palette) == Color(204, 0, 0));
    CHECK(parseThemeColor("success", &palette) == Color(0, 170, 0));
    CHECK(parseThemeColor("muted", &palette) == Color(128, 128, 128));
    // Names the theme does not define stay unresolved.
    CHECK_FALSE(parseThemeColor("orange", &palette).has_value());
}

TEST_CASE("markup/parseMenuColor hex and palette values") {
    CHECK(parseMenuColor("#ff0000") == Color(255, 0, 0));
    CHECK(parseMenuColor("#F00") == Color(255, 0, 0));
    CHECK(parseMenuColor("00ff7f") == Color(0, 255, 127));
    CHECK(parseMenuColor("208") == Color(static_cast<Color::Palette256>(208)));
    CHECK(parseMenuColor("0") == Color(static_cast<Color::Palette256>(0)));
}

TEST_CASE("markup/parseMenuColor rejects everything else") {
    CHECK_FALSE(parseMenuColor("").has_value());
    CHECK_FALSE(parseMenuColor("default").has_value());
    CHECK_FALSE(parseMenuColor("none").has_value());
    CHECK_FALSE(parseMenuColor("Transparent").has_value());
    CHECK_FALSE(parseMenuColor("not-a-color").has_value());
    CHECK_FALSE(parseMenuColor("#12345").has_value());
    CHECK_FALSE(parseMenuColor("#gggggg").has_value());
    CHECK_FALSE(parseMenuColor("9999").has_value());
}

TEST_CASE("markup/parseThemeColor resolves named theme colors") {
    const std::map<std::string, std::string> colors = {
        {"brand", "#ff0000"},
        {"accent", "brand"},
        {"loopa", "loopb"},
        {"loopb", "loopa"},
    };
    CHECK(parseThemeColor("brand", &colors) == Color(255, 0, 0));
    CHECK(parseThemeColor("Accent", &colors) == Color(255, 0, 0));
    // The map has no "blue", and names are not built in.
    CHECK_FALSE(parseThemeColor("blue", &colors).has_value());
    CHECK(parseThemeColor("blue", &testfw::defaultPalette()) ==
          Color(0, 0, 204));
    CHECK_FALSE(parseThemeColor("blue", nullptr).has_value());
    // Cycles stop at the depth limit instead of recursing forever.
    CHECK_FALSE(parseThemeColor("loopa", &colors).has_value());
}

// --- tags --------------------------------------------------------------------

TEST_CASE("markup/parseMenuMarkupTag style tags") {
    for (const char* tag : {"bold", "B"}) {
        auto frame = parseMenuMarkupTag(tag, nullptr);
        REQUIRE(frame.has_value());
        CHECK(frame->kind == MenuMarkupKind::Bold);
    }
    CHECK(parseMenuMarkupTag("dim", nullptr)->kind == MenuMarkupKind::Dim);
    CHECK(parseMenuMarkupTag("italic", nullptr)->kind == MenuMarkupKind::Italic);
    CHECK(parseMenuMarkupTag("u", nullptr)->kind == MenuMarkupKind::Underline);
    CHECK(parseMenuMarkupTag("inverse", nullptr)->kind ==
          MenuMarkupKind::Inverted);
}

TEST_CASE("markup/parseMenuMarkupTag colors and backgrounds") {
    auto fg = parseMenuMarkupTag("red", &testfw::defaultPalette());
    REQUIRE(fg.has_value());
    CHECK(fg->kind == MenuMarkupKind::Color);
    CHECK(fg->color == Color(204, 0, 0));

    for (const char* tag : {"on red", "on-red", "onred", "bg:red", "bg-red"}) {
        auto bg = parseMenuMarkupTag(tag, &testfw::defaultPalette());
        REQUIRE(bg.has_value());
        CHECK(bg->kind == MenuMarkupKind::Background);
        CHECK(bg->color == Color(204, 0, 0));
    }
}

TEST_CASE("markup/parseMenuMarkupTag rejects unknown tags") {
    CHECK_FALSE(parseMenuMarkupTag("", nullptr).has_value());
    CHECK_FALSE(parseMenuMarkupTag("title", nullptr).has_value());
    CHECK_FALSE(parseMenuMarkupTag("on", nullptr).has_value());
}

TEST_CASE("markup/markupTagsMatch compares names only for colors") {
    MenuMarkupFrame red =
        *parseMenuMarkupTag("red", &testfw::defaultPalette());
    MenuMarkupFrame blue =
        *parseMenuMarkupTag("blue", &testfw::defaultPalette());
    MenuMarkupFrame bold = *parseMenuMarkupTag("bold", nullptr);
    MenuMarkupFrame b = *parseMenuMarkupTag("b", nullptr);
    CHECK(markupTagsMatch(red, red));
    CHECK_FALSE(markupTagsMatch(red, blue));
    CHECK_FALSE(markupTagsMatch(red, bold));
    // Style tags match by kind, so <bold> closes with </b>.
    CHECK(markupTagsMatch(bold, b));
}

// --- rendering -----------------------------------------------------------------

TEST_CASE("markup/renderMenuMarkup plain text") {
    CHECK_EQ(renderPlain(markup("hello world")), "hello world");
}

TEST_CASE("markup/renderMenuMarkup strips known tags and applies style") {
    const std::string raw = renderRaw(markup("<red>a</red>b"));
    CHECK(contains(raw, kEsc));
    CHECK_EQ(testfw::stripAnsi(raw), "ab");

    CHECK_EQ(renderPlain(markup("<bold><green>hi</green></bold>!")),
             "hi!");
    CHECK(contains(renderRaw(markup("<bold>x</bold>")), kEsc));
}

TEST_CASE("markup/renderMenuMarkup keeps unknown and unmatched tags literal") {
    CHECK_EQ(renderPlain(markup("<foo>x</foo>")), "<foo>x</foo>");
    CHECK_EQ(renderPlain(markup("x</red>")), "x</red>");
    CHECK_EQ(renderPlain(markup("a < b")), "a < b");
    CHECK_EQ(renderPlain(markup("<red>open")), "open");
}

TEST_CASE("markup/renderMenuMarkup uses theme color names") {
    const std::map<std::string, std::string> colors = {{"brand", "#ff0000"}};
    const std::string raw = renderRaw(markup("<brand>x</brand>", &colors));
    CHECK(contains(raw, kEsc));
    CHECK_EQ(testfw::stripAnsi(raw), "x");
    // Without the theme the same tag is just text.
    CHECK_EQ(renderPlain(markup("<brand>x</brand>")),
             "<brand>x</brand>");
}

// --- truncation ----------------------------------------------------------------

TEST_CASE("markup/truncates to the width with dots") {
    CHECK_EQ(renderPlain(markup("abcdefghij")), "abcdefghij");
    CHECK_EQ(renderPlain(markup("abcdefghij", 8)),
             "abcde...");
    CHECK_EQ(renderPlain(markup("abcdefghij", 10)),
             "abcdefghij");
    CHECK_EQ(renderPlain(markup("abcdefghij", 9)),
             "abcdef...");
}

TEST_CASE("markup/the dots themselves never overflow") {
    CHECK_EQ(renderPlain(markup("abcdef", 1)), ".");
    CHECK_EQ(renderPlain(markup("abcdef", 2)), "..");
    CHECK_EQ(renderPlain(markup("abcdef", 3)), "...");
    CHECK_EQ(renderPlain(markup("abcdef", 0)), "abcdef");
}

TEST_CASE("markup/truncation respects wide glyphs") {
    // Four CJK glyphs are eight cells wide.
    const std::string text = "\xE9\xA1\xB9\xE7\x9B\xAE\xE4\xB8\xAD\xE6\x96\x87";  // 项目中文
    CHECK_EQ(renderPlain(markup(text, 8)), text);
    // Five cells leave room for one glyph (2 cells) plus three dots.
    CHECK_EQ(renderPlain(markup(text, 5)),
             "\xE9\xA1\xB9" "...");
    // A narrow cell cannot split a double-width glyph.
    CHECK_EQ(renderPlain(markup(text, 4)), "...");
}

TEST_CASE("markup/truncation happens after tags are parsed") {
    const std::string raw =
        renderRaw(markup("<red>abc</red><blue>defgh</blue>", 6));
    CHECK_EQ(testfw::stripAnsi(raw), "abc...");
    // The dots keep the style of the segment they replace.
    CHECK(contains(raw, kEsc));

    // Tags alone are not text and do not consume width.
    CHECK_EQ(renderPlain(markup("<bold>ab</bold>", 3)),
             "ab");
}

TEST_CASE("markup/unknown tags count toward the width") {
    CHECK_EQ(renderPlain(markup("<foo>abc", 6)),
             "<fo...");
}

TEST_CASE("markup/menuMarkupDisplayWidth ignores tags") {
    CHECK_EQ(menuMarkupDisplayWidth("plain"), 5);
    CHECK_EQ(menuMarkupDisplayWidth("<red>abc</red>", &testfw::defaultPalette()), 3);
    CHECK_EQ(menuMarkupDisplayWidth("<red>abc</red>def", &testfw::defaultPalette()), 6);
    CHECK_EQ(menuMarkupDisplayWidth(""), 0);
    CHECK_EQ(menuMarkupDisplayWidth("\xE9\xA1\xB9\xE7\x9B\xAE"), 4);
}

TEST_CASE("markup/styleMenuEntry truncates the label but keeps the prefix") {
    const ftxui::EntryState state{"abcdefghij", false, false, false, 0};
    CHECK_EQ(renderPlain(styleMenuEntry(state, "abcdefghij", false, true,
                                        "> ", "  ", false, nullptr, 8)),
             "  abc...");
    CHECK_EQ(renderPlain(styleMenuEntry(state, "abcdefghij", false, true,
                                        "> ", "  ", false, nullptr, 0)),
             "  abcdefghij");
}

TEST_CASE("markup/renderMenuMarkup keeps non-ASCII text") {
    CHECK_EQ(renderPlain(markup("<cyan>\xE9\xA1\xB9\xE7\x9B\xAE</cyan>")),
             "\xE9\xA1\xB9\xE7\x9B\xAE");
}

// --- borders -----------------------------------------------------------------

TEST_CASE("markup/splitUtf8Glyphs") {
    const auto glyphs = splitUtf8Glyphs("a\xE2\x95\xAD\xE4\xB8\xAD\xF0\x9F\x98\x80");
    REQUIRE(glyphs.size() == 4);
    CHECK_EQ(glyphs[0], "a");
    CHECK_EQ(glyphs[1], "\xE2\x95\xAD");
    CHECK_EQ(glyphs[2], "\xE4\xB8\xAD");
    CHECK_EQ(glyphs[3], "\xF0\x9F\x98\x80");
    // A truncated trailing sequence is dropped rather than read out of bounds.
    CHECK_EQ(splitUtf8Glyphs("a\xE2\x95").size(), static_cast<std::size_t>(1));
    CHECK(splitUtf8Glyphs("").empty());
}

TEST_CASE("markup/parseBorderCharacters") {
    auto single = parseBorderCharacters("#");
    REQUIRE(single.has_value());
    for (const std::string& glyph : *single) {
        CHECK_EQ(glyph, "#");
    }

    // Input order: top-left, top, top-right, side, bottom-left, bottom-right.
    // Stored order: corners first, then horizontal and vertical edges.
    const BorderCharset expected = {"\xE2\x95\xAD", "\xE2\x95\xAE", "\xE2\x95\xB0",
                                    "\xE2\x95\xAF", "\xE2\x94\x80", "\xE2\x94\x82"};
    auto packed = parseBorderCharacters(
        "\xE2\x95\xAD\xE2\x94\x80\xE2\x95\xAE\xE2\x94\x82\xE2\x95\xB0\xE2\x95\xAF");
    REQUIRE(packed.has_value());
    CHECK(*packed == expected);

    auto spaced = parseBorderCharacters(
        "\xE2\x95\xAD \xE2\x94\x80 \xE2\x95\xAE \xE2\x94\x82 \xE2\x95\xB0 \xE2\x95\xAF");
    REQUIRE(spaced.has_value());
    CHECK(*spaced == expected);
}

TEST_CASE("markup/parseBorderCharacters rejects wrong glyph counts") {
    CHECK_FALSE(parseBorderCharacters("").has_value());
    CHECK_FALSE(parseBorderCharacters("a b").has_value());
    CHECK_FALSE(parseBorderCharacters("a b c").has_value());
    CHECK_FALSE(parseBorderCharacters("a b c d e").has_value());
    CHECK_FALSE(parseBorderCharacters("   ").has_value());
}

TEST_CASE("markup/parseBorderStyle") {
    CHECK(parseBorderStyle("dashed") == ftxui::DASHED);
    CHECK(parseBorderStyle("Heavy") == ftxui::HEAVY);
    CHECK(parseBorderStyle("double") == ftxui::DOUBLE);
    CHECK(parseBorderStyle("rounded") == ftxui::ROUNDED);
    CHECK(parseBorderStyle("empty") == ftxui::EMPTY);
    CHECK(parseBorderStyle("light") == ftxui::LIGHT);
    CHECK(parseBorderStyle("whatever") == ftxui::LIGHT);
}

TEST_CASE("markup/applyMenuBorder honors custom characters, style and none") {
    MenuTheme theme;
    theme.borderStyle = "rounded";
    const std::string rounded = renderPlain(applyMenuBorder(ftxui::text("x"), theme));
    CHECK(contains(rounded, "\xE2\x95\xAD"));  // top-left of the rounded frame
    CHECK(contains(rounded, "x"));

    theme.borderCharacters = "#";
    const std::string custom = renderPlain(applyMenuBorder(ftxui::text("x"), theme));
    CHECK(contains(custom, "#"));
    CHECK_FALSE(contains(custom, "\xE2\x95\xAD"));

    theme.borderCharacters = "";
    theme.borderStyle = "none";
    CHECK_EQ(renderPlain(applyMenuBorder(ftxui::text("x"), theme)), "x");
}

TEST_CASE("markup/custom border glyphs surround the content") {
    MenuTheme theme;
    theme.borderCharacters =
        "\xE2\x95\xAD\xE2\x94\x80\xE2\x95\xAE\xE2\x94\x82\xE2\x95\xB0\xE2\x95\xAF";
    const std::string text = renderPlain(applyMenuBorder(ftxui::text("ab"), theme));
    CHECK_EQ(text,
             "\xE2\x95\xAD\xE2\x94\x80\xE2\x94\x80\xE2\x95\xAE\n"
             "\xE2\x94\x82" "ab" "\xE2\x94\x82\n"
             "\xE2\x95\xB0\xE2\x94\x80\xE2\x94\x80\xE2\x95\xAF");
}

TEST_CASE("markup/makeMenuSeparatorFor") {
    CHECK(makeMenuSeparatorFor("light", "").has_value());
    CHECK(makeMenuSeparatorFor("dashed", "").has_value());
    CHECK(makeMenuSeparatorFor("heavy", "").has_value());
    CHECK(makeMenuSeparatorFor("double", "").has_value());
    CHECK(makeMenuSeparatorFor("empty", "").has_value());
    CHECK(makeMenuSeparatorFor("unknown", "").has_value());
    CHECK_FALSE(makeMenuSeparatorFor("none", "").has_value());
    // An explicit character wins over the style, even "none".
    CHECK(makeMenuSeparatorFor("none", "=").has_value());
}

TEST_CASE("markup/alignMenuLine pads according to alignment") {
    auto line = [](const char* alignment) {
        ftxui::Element element = alignMenuLine(ftxui::text("ab"), alignment);
        ftxui::Screen screen(10, 1);
        ftxui::Render(screen, element);
        return testfw::stripAnsi(screen.ToString());
    };
    CHECK_EQ(line("left"), "ab        ");
    CHECK_EQ(line("right"), "        ab");
    CHECK_EQ(line("center"), "    ab    ");
    CHECK_EQ(line("anything"), "ab        ");
}

// --- text helpers ------------------------------------------------------------

TEST_CASE("markup/splitMenuLines handles all newline styles") {
    CHECK_EQ(splitMenuLines("a\nb"), (std::vector<std::string>{"a", "b"}));
    CHECK_EQ(splitMenuLines("a\r\nb\r\n"),
             (std::vector<std::string>{"a", "b", ""}));
    CHECK_EQ(splitMenuLines(""), std::vector<std::string>{""});
    CHECK_EQ(splitMenuLines("\n\n"), (std::vector<std::string>{"", "", ""}));
}

TEST_CASE("markup/expandMenuText substitutes brace and angle templates") {
    TempDir root;
    ConfigManager config(root.path(), root.path() / "claudex-config.json");
    config.load();

    const std::string rootText = pathToUtf8(root.path());
    CHECK_EQ(expandMenuText("{root}|<root>", config, ""),
             rootText + "|" + rootText);
    CHECK_EQ(expandMenuText("{project}|<project>", config, "P"), "P|P");
    CHECK_EQ(expandMenuText("<claude>", config, ""), "auto");
    CHECK_EQ(expandMenuText("{config}", config, ""),
             pathToUtf8(config.configFile()));
    CHECK_EQ(expandMenuText("{theme}", config, ""),
             pathToUtf8(config.themeFile()));
    CHECK_EQ(expandMenuText("{log}", config, ""),
             pathToUtf8(config.logFilePath()));
    CHECK_EQ(expandMenuText("{mode}-<mode>", config, "", {{"mode", "edit"}}),
             "edit-edit");
    CHECK_EQ(expandMenuText("{unknown}", config, ""), "{unknown}");
}

TEST_CASE("markup/expandMenuText reports a disabled log") {
    TempDir root;
    ConfigManager config(root.path(), root.path() / "claudex-config.json");
    testfw::writeFile(root.path() / "claudex-config.json", "{\"logFile\": \"\"}");
    config.load();
    CHECK_EQ(expandMenuText("{log}", config, ""), "disabled");
}

TEST_CASE("markup/expandCommandText only expands brace placeholders") {
    TempDir root;
    ConfigManager config(root.path(), root.path() / "claudex-config.json");
    config.load();

    CHECK_EQ(expandCommandText("{root}", config, "P"), pathToUtf8(root.path()));
    CHECK_EQ(expandCommandText("{project}:{path}", config, "P"), "P:P");
    CHECK_EQ(expandCommandText("{claude}", config, ""), "auto");
    // Angle brackets are shell redirection, not templates.
    CHECK_EQ(expandCommandText("echo <root> > out", config, ""),
             "echo <root> > out");
}

TEST_CASE("markup/expandEntryLines substitutes name, path and lastUsed") {
    const auto& palette = testfw::defaultPalette();
    const auto lines = expandEntryLines(
        {"{name}", "<muted>{path}</muted>", "<lastUsed>"},
        "My <red>project</red>", "/p", "2026-01-02", &palette);
    REQUIRE(lines.size() == 3);
    CHECK_EQ(lines[0], "My <red>project</red>");
    CHECK_EQ(lines[1], "<muted>/p</muted>");
    CHECK_EQ(lines[2], "2026-01-02");
}

TEST_CASE("markup/expandEntryLines drops lines that end up empty") {
    CHECK_EQ(expandEntryLines({"{name}", "{path}"}, "", "/p", "", nullptr),
             std::vector<std::string>{"/p"});
    CHECK_EQ(expandEntryLines({"{name}"}, "", "/p", "", nullptr),
             std::vector<std::string>{"/p"});
    // Markup without text is empty too.
    const auto& palette = testfw::defaultPalette();
    CHECK_EQ(expandEntryLines({"{name}", "/x"}, "<red></red>", "/p", "",
                              &palette),
             std::vector<std::string>{"/x"});
    // Nothing at all still falls back to the path.
    CHECK_EQ(expandEntryLines({"/x"}, "", "/p", "", nullptr),
             std::vector<std::string>{"/x"});
}

TEST_CASE("markup/a name's own markup is not expanded again") {
    const auto lines = expandEntryLines({"{name}"}, "{path}", "/p", "", nullptr);
    REQUIRE(lines.size() == 1);
    CHECK_EQ(lines[0], "{path}");
}

TEST_CASE("markup/styleMenuEntryLines indents continuation lines") {
    const auto& palette = testfw::defaultPalette();
    const ftxui::EntryState state{"", false, true, false, 0};
    const ftxui::Element element = styleMenuEntryLines(
        state, {"name", "path"}, true, true, "> ", "  ", false, &palette);
    CHECK_EQ(renderPlain(element), "> name\n  path");
}

TEST_CASE("markup/styleMenuEntryLines truncates every line to the same width") {
    const auto& palette = testfw::defaultPalette();
    const ftxui::EntryState state{"", false, false, false, 0};
    const ftxui::Element element = styleMenuEntryLines(
        state, {"abcdefgh", "ijklmnop"}, true, true, "  ", "  ", false,
        &palette, 8);
    CHECK_EQ(renderPlain(element), "  abc...\n  ijk...");
}

TEST_CASE("markup/styleMenuEntry shows the cursor only when focused and active") {
    const std::string active = ">> ";
    const std::string inactive = "   ";
    auto render = [&](bool entryActive, bool menuFocused) {
        const ftxui::EntryState state{"item", false, entryActive, false, 0};
        return renderPlain(styleMenuEntry(state, "item", false, menuFocused,
                                          active, inactive, true, nullptr));
    };
    CHECK_EQ(render(true, true), ">> item");
    CHECK_EQ(render(true, false), "   item");
    CHECK_EQ(render(false, true), "   item");
}

TEST_CASE("markup/styleMenuEntry can render markup labels") {
    const ftxui::EntryState state{"<red>x</red>", false, false, false, 0};
    CHECK_EQ(renderPlain(styleMenuEntry(state, "<red>x</red>", true, true, "> ",
                                        "  ", false,
                                        &testfw::defaultPalette())),
             "  x");
    CHECK_EQ(renderPlain(styleMenuEntry(state, "<red>x</red>", false, true, "> ",
                                        "  ", false, nullptr)),
             "  <red>x</red>");
}
