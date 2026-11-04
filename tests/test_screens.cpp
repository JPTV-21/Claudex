#include "test_framework.hpp"

#include "config_manager.hpp"
#include "render_helper.hpp"
#include "screens.hpp"

using namespace claudex;
using testfw::renderPlain;
using testfw::TempDir;

namespace {

bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

// A loaded config in a scratch folder plus a render context bound to it.
struct Fixture {
    TempDir root;
    ConfigManager config;
    BlockRenderContext context;

    Fixture() : config(root.path(), root.path() / "claudex-config.json") {
        config.load();
        context.config = &config;
        context.theme = &config.menuTheme;
    }
};

std::string renderBlocks(const json& blocks, const BlockRenderContext& context) {
    ScreenTheme screen;
    screen.blocks = blocks;
    return renderPlain(renderScreen(screen, context));
}

}  // namespace

// --- block search --------------------------------------------------------------

TEST_CASE("screens/findBlockByType finds nested blocks") {
    const json blocks = json::array({
        {{"type", "text"}},
        {{"type", "box"},
         {"children", json::array({{{"type", "Path-Input"}, {"id", 7}}})}},
    });
    const json* found = findBlockByType(blocks, "pathInput");
    REQUIRE(found != nullptr);
    CHECK_EQ(found->at("id").get<int>(), 7);
    CHECK(findBlockByType(blocks, "text") == &blocks[0]);
    CHECK(findBlockByType(blocks, "buttons") == nullptr);
    CHECK(findBlockByType(json::object(), "text") == nullptr);
    CHECK(findBlockByType(blocks, "text", 33) == nullptr);
}

TEST_CASE("screens/screenThemeFor looks screens up by name") {
    MenuTheme theme;
    theme.screens["main"].width = 42;
    CHECK_EQ(screenThemeFor(theme, "main").width, 42);
    const ScreenTheme& missing = screenThemeFor(theme, "nope");
    CHECK_EQ(missing.width, 0);
    CHECK(missing.blocks.empty());
    // The same fallback object is reused.
    CHECK(&missing == &screenThemeFor(theme, "also-nope"));
}

// --- conditions ----------------------------------------------------------------

TEST_CASE("screens/conditions on selection, status and errors") {
    BlockRenderContext context;
    auto check = [&](const char* key, const char* condition) {
        return blockConditionSatisfied(json{{key, condition}}, context);
    };

    CHECK_FALSE(check("if", "selection"));
    CHECK(check("if", "noselection"));
    context.hasSelection = true;
    CHECK(check("if", "selected"));
    CHECK_FALSE(check("if", "noSelection"));

    CHECK(check("if", "nostatus"));
    context.statusText = "hello";
    CHECK(check("if", "status"));
    CHECK_FALSE(check("unless", "nostatus") == false);

    context.errorText = "boom";
    CHECK(check("if", "error"));
    CHECK_FALSE(check("if", "noerror"));

    context.showAddProject = true;
    CHECK(check("if", "addProject"));
    CHECK_FALSE(check("if", "noAddProject"));

    context.recentCount = 2;
    CHECK(check("if", "recent"));
    CHECK_FALSE(check("if", "norecent"));
    context.hasQuickInput = true;
    CHECK(check("if", "quickInput"));
}

TEST_CASE("screens/unless inverts and unknown conditions are ignored") {
    BlockRenderContext context;
    context.hasSelection = true;
    CHECK_FALSE(blockConditionSatisfied(json{{"unless", "selection"}}, context));
    CHECK(blockConditionSatisfied(json{{"unless", "noselection"}}, context));
    CHECK(blockConditionSatisfied(json{{"if", "totally-unknown"}}, context));
    CHECK(blockConditionSatisfied(json{{"if", 5}}, context));
    CHECK(blockConditionSatisfied(json::object(), context));
}

TEST_CASE("screens/project conditions need a config") {
    Fixture fixture;
    CHECK(blockConditionSatisfied(json{{"if", "noprojects"}}, fixture.context));
    CHECK_FALSE(blockConditionSatisfied(json{{"if", "projects"}}, fixture.context));
    fixture.config.projects.push_back({"x", "", ""});
    CHECK(blockConditionSatisfied(json{{"if", "projects"}}, fixture.context));

    const BlockRenderContext detached;
    CHECK_FALSE(blockConditionSatisfied(json{{"if", "projects"}}, detached));
    CHECK_FALSE(blockConditionSatisfied(json{{"if", "noprojects"}}, detached));
}

// --- text expansion --------------------------------------------------------------

TEST_CASE("screens/block text placeholders") {
    Fixture fixture;
    fixture.context.pathText = "C:/some/path";
    fixture.context.errorText = "oops";
    fixture.context.modeText = "Edit";
    fixture.context.statusText = "ready";

    const auto placeholders = blockPlaceholders(fixture.context);
    CHECK_EQ(placeholders.at("path"), "C:/some/path");
    CHECK_EQ(placeholders.at("error"), "oops");
    CHECK_EQ(placeholders.at("mode"), "Edit");
    CHECK_EQ(placeholders.at("status"), "ready");

    CHECK_EQ(expandBlockText("{mode}: {path}", fixture.context),
             "Edit: C:/some/path");
    // Without a config the text is returned untouched.
    BlockRenderContext detached;
    detached.pathText = "x";
    CHECK_EQ(expandBlockText("{path}", detached), "{path}");
}

TEST_CASE("screens/appendRenderedText splits lines and skips empty text") {
    Fixture fixture;
    std::vector<ftxui::Element> out;
    appendRenderedText("", "left", fixture.context, out);
    CHECK(out.empty());
    appendRenderedText("one\ntwo", "left", fixture.context, out);
    CHECK_EQ(out.size(), static_cast<std::size_t>(2));
}

// --- borders and alignment ---------------------------------------------------------

TEST_CASE("screens/applyBorderSpec") {
    MenuTheme theme;
    theme.borderStyle = "rounded";

    CHECK_EQ(renderPlain(applyBorderSpec(ftxui::text("x"), "", theme)), "x");
    CHECK_EQ(renderPlain(applyBorderSpec(ftxui::text("x"), "none", theme)), "x");
    CHECK_EQ(renderPlain(applyBorderSpec(ftxui::text("x"), "a b c", theme)), "x");

    CHECK(contains(renderPlain(applyBorderSpec(ftxui::text("x"), "double", theme)),
                   "\xE2\x95\x94"));  // double top-left corner
    CHECK(contains(renderPlain(applyBorderSpec(ftxui::text("x"), "theme", theme)),
                   "\xE2\x95\xAD"));  // the theme's rounded frame
    CHECK(contains(renderPlain(applyBorderSpec(ftxui::text("x"), "#", theme)),
                   "#"));
}

TEST_CASE("screens/alignElement") {
    auto line = [](const char* alignment) {
        ftxui::Element element = alignElement(ftxui::text("ab"), alignment);
        ftxui::Screen screen(10, 1);
        ftxui::Render(screen, element);
        return testfw::stripAnsi(screen.ToString());
    };
    CHECK_EQ(line("left"), "ab        ");
    CHECK_EQ(line("center"), "    ab    ");
    CHECK_EQ(line("right"), "        ab");
}

// --- directional navigation ----------------------------------------------------------

TEST_CASE("screens/nearestBoxInDirection follows a vertical stack") {
    // x_min, x_max, y_min, y_max
    const std::vector<ftxui::Box> boxes = {
        {0, 20, 0, 3}, {0, 20, 4, 7}, {0, 20, 8, 9}};
    CHECK_EQ(nearestBoxInDirection(boxes, 0, NavDir::Down), 1);
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Down), 2);
    CHECK_EQ(nearestBoxInDirection(boxes, 2, NavDir::Up), 1);
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Up), 0);
    CHECK_EQ(nearestBoxInDirection(boxes, 0, NavDir::Up), -1);
    CHECK_EQ(nearestBoxInDirection(boxes, 2, NavDir::Down), -1);
    // Everything sits in one column, so there is nothing left or right.
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Left), -1);
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Right), -1);
}

TEST_CASE("screens/nearestBoxInDirection follows columns") {
    const std::vector<ftxui::Box> boxes = {
        {0, 17, 0, 10},    // left column
        {19, 40, 0, 4},    // right, top
        {19, 40, 6, 10},   // right, bottom
    };
    CHECK_EQ(nearestBoxInDirection(boxes, 0, NavDir::Right), 1);
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Left), 0);
    CHECK_EQ(nearestBoxInDirection(boxes, 2, NavDir::Left), 0);
    CHECK_EQ(nearestBoxInDirection(boxes, 2, NavDir::Up), 1);
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Down), 2);
}

TEST_CASE("screens/nearestBoxInDirection prefers the aligned box") {
    // Both candidates are below; the one sharing the horizontal span wins even
    // when it is slightly farther away.
    const std::vector<ftxui::Box> boxes = {
        {0, 10, 0, 2},    // origin
        {40, 60, 3, 5},   // far away, no overlap
        {0, 10, 4, 6},    // aligned below
    };
    CHECK_EQ(nearestBoxInDirection(boxes, 0, NavDir::Down), 2);
}

TEST_CASE("screens/nearestBoxInDirection ignores empty boxes") {
    const std::vector<ftxui::Box> boxes = {
        {0, 20, 0, 3}, {1, 0, 1, 0}, {0, 20, 8, 9}};
    CHECK_EQ(nearestBoxInDirection(boxes, 0, NavDir::Down), 2);
    // An empty origin has no direction at all.
    CHECK_EQ(nearestBoxInDirection(boxes, 1, NavDir::Down), -1);
    CHECK_EQ(nearestBoxInDirection(boxes, 7, NavDir::Down), -1);
    CHECK_EQ(nearestBoxInDirection({}, 0, NavDir::Down), -1);
}

// --- separators ----------------------------------------------------------------------

TEST_CASE("screens/a horizontal separator block is one row tall") {
    Fixture fixture;
    fixture.config.menuTheme.separatorStyle = "light";
    fixture.config.menuTheme.separatorCharacter.clear();
    const json blocks = json::array({
        {{"type", "vbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "abcd"}},
            {{"type", "separator"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context),
             "abcd\n\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80");  // "────"
}

TEST_CASE("screens/a vertical separator block is one column wide") {
    Fixture fixture;
    fixture.config.menuTheme.separatorStyle = "light";
    fixture.config.menuTheme.separatorCharacter.clear();
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "ab"}},
            {{"type", "separator"}, {"vertical", true}},
            {{"type", "text"}, {"text", "cd"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "ab\xE2\x94\x82" "cd");
}

TEST_CASE("screens/a vertical separator fills the height of its row") {
    Fixture fixture;
    fixture.config.menuTheme.separatorStyle = "light";
    fixture.config.menuTheme.separatorCharacter.clear();
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "a\nb\nc"}},
            {{"type", "separator"}, {"orientation", "Vertical"}},
            {{"type", "text"}, {"text", "x\ny\nz"}},
         })}},
    });
    // Every row gets the divider glyph.
    CHECK_EQ(renderBlocks(blocks, fixture.context),
             "a\xE2\x94\x82" "x\nb\xE2\x94\x82" "y\nc\xE2\x94\x82" "z");
}

TEST_CASE("screens/a vertical separator honors a custom character and style") {
    Fixture fixture;
    fixture.config.menuTheme.separatorCharacter = "-";
    const json custom = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "separator"}, {"vertical", true}, {"character", "l"}},
            {{"type", "text"}, {"text", "x"}},
         })}},
    });
    CHECK_EQ(renderBlocks(custom, fixture.context), "l" "x");

    // Without a character the theme style decides the glyph.
    fixture.config.menuTheme.separatorCharacter.clear();
    fixture.config.menuTheme.separatorStyle = "double";
    const json styled = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "separator"}, {"vertical", true}},
            {{"type", "text"}, {"text", "x"}},
         })}},
    });
    CHECK_EQ(renderBlocks(styled, fixture.context), "\xE2\x95\x91" "x");  // ║
}

TEST_CASE("screens/a separator styled none is dropped") {
    Fixture fixture;
    for (const bool vertical : {false, true}) {
        const json blocks = json::array({
            {{"type", "separator"}, {"style", "none"}, {"vertical", vertical}},
        });
        CHECK_EQ(renderBlocks(blocks, fixture.context), "");
    }
}

TEST_CASE("screens/a vertical separator stays one column in a wide row") {
    Fixture fixture;
    fixture.config.menuTheme.separatorStyle = "light";
    fixture.config.menuTheme.separatorCharacter.clear();
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "separator"}, {"vertical", true}},
            {{"type", "text"}, {"text", "x"}},
         })}},
    });
    ScreenTheme screen;
    screen.blocks = blocks;
    ftxui::Element element = renderScreen(screen, fixture.context);
    ftxui::Screen canvas(10, 1);
    ftxui::Render(canvas, element);
    CHECK_EQ(testfw::stripAnsi(canvas.ToString()),
             "\xE2\x94\x82" "x        ");  // "│x        "
}

// --- sizing --------------------------------------------------------------------------

TEST_CASE("screens/exact width pads a short block") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "a"}, {"width", 4}},
            {{"type", "text"}, {"text", "b"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "a   b");
}

TEST_CASE("screens/exact width truncates with dots") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "text"}, {"text", "abcdefgh"}, {"width", 5}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "ab...");
}

TEST_CASE("screens/maxWidth shrinks instead of padding") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "a"}, {"maxWidth", 6}},
            {{"type", "text"}, {"text", "b"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "ab");
}

TEST_CASE("screens/maxWidth truncates long text") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "text"}, {"text", "abcdefgh"}, {"maxWidth", 6}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "abc...");
}

TEST_CASE("screens/exact height pads with blank rows") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "text"}, {"text", "a"}, {"height", 3}},
        {{"type", "text"}, {"text", "z"}},
    });
    // Rows the block pads are blank cells; the screen pads every row to its
    // widest line.
    CHECK_EQ(renderBlocks(blocks, fixture.context), "a\n \n \nz");
}

TEST_CASE("screens/height applies to the whole text block, not each line") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "text"}, {"text", "one\ntwo\nthree"}, {"height", 2}},
        {{"type", "text"}, {"text", "after"}},
    });
    // The cut is marked on the last visible line of the block.
    CHECK_EQ(renderBlocks(blocks, fixture.context),
             "one   \ntwo...\nafter ");
}

TEST_CASE("screens/a box border counts toward the exact width") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "box"},
         {"border", "light"},
         {"width", 6},
         {"children", json::array({{{"type", "text"}, {"text", "ab"}}})}},
    });
    const std::string out = renderBlocks(blocks, fixture.context);
    // Six cells are six three-byte glyphs.
    const std::string border =
        "\xE2\x94\x8C" "\xE2\x94\x80" "\xE2\x94\x80" "\xE2\x94\x80"
        "\xE2\x94\x80" "\xE2\x94\x90";
    CHECK_EQ(out.find('\n'), border.size());
    CHECK_EQ(out.substr(0, border.size()), border);
}

TEST_CASE("screens/the page width truncates top-level text") {
    Fixture fixture;
    ScreenTheme screen;
    screen.width = 5;
    screen.blocks = json::array(
        {{{"type", "text"}, {"text", "abcdefghij"}}});
    CHECK_EQ(renderPlain(renderScreen(screen, fixture.context)), "ab...");
}

TEST_CASE("screens/sizes can come from theme variables") {
    Fixture fixture;
    fixture.config.menuTheme.variables["narrow"] = "4";
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "a"}, {"width", "narrow"}},
            {{"type", "text"}, {"text", "b"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "a   b");
}

TEST_CASE("screens/maxHeight bounds the block without padding") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "text"}, {"text", "one"}, {"maxHeight", 5}},
        {{"type", "text"}, {"text", "two"}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "one\ntwo");
}

TEST_CASE("screens/minWidth widens a block beyond its content") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "a"}, {"minWidth", 6}},
            {{"type", "text"}, {"text", "b"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "a     b");
}

TEST_CASE("screens/minWidth with width fixes the column size") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "hbox"},
         {"children", json::array({
            {{"type", "text"}, {"text", "a"}, {"minWidth", 6}, {"width", 6}},
            {{"type", "text"}, {"text", "b"}},
         })}},
    });
    CHECK_EQ(renderBlocks(blocks, fixture.context), "a     b");
}

TEST_CASE("screens/minHeight adds empty rows inside a box") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "box"},
         {"border", "light"},
         {"minHeight", 4},
         {"children", json::array({{{"type", "text"}, {"text", "x"}}})}},
    });
    const std::string out = renderBlocks(blocks, fixture.context);
    // The minimum covers the whole block, border rows included.
    CHECK_EQ(out, "\xE2\x94\x8C" "\xE2\x94\x80" "\xE2\x94\x90\n"
                  "\xE2\x94\x82" "x" "\xE2\x94\x82\n"
                  "\xE2\x94\x82" " " "\xE2\x94\x82\n"
                  "\xE2\x94\x94" "\xE2\x94\x80" "\xE2\x94\x98");
}

// --- screens ------------------------------------------------------------------------

TEST_CASE("screens/text blocks render with placeholders and markup") {
    Fixture fixture;
    fixture.context.pathText = "D:/work";
    const json blocks = json::array({
        {{"type", "text"}, {"text", "<bold>Hello</bold> {path}"}},
        {{"type", "text"}, {"text", "line1\nline2"}},
    });
    const std::string out = renderBlocks(blocks, fixture.context);
    CHECK(contains(out, "Hello D:/work"));
    CHECK(contains(out, "line1"));
    CHECK(contains(out, "line2"));
}

TEST_CASE("screens/an empty screen renders as empty text") {
    Fixture fixture;
    CHECK_EQ(renderBlocks(json::array(), fixture.context), "");
}

TEST_CASE("screens/blocks hidden by conditions are skipped") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "text"}, {"text", "visible"}},
        {{"type", "text"}, {"text", "hidden"}, {"if", "selection"}},
        {{"type", "text"}, {"text", "unless-visible"}, {"unless", "selection"}},
    });
    const std::string out = renderBlocks(blocks, fixture.context);
    CHECK(contains(out, "visible"));
    CHECK_FALSE(contains(out, "hidden"));
    CHECK(contains(out, "unless-visible"));
}

TEST_CASE("screens/a context without a theme renders nothing") {
    BlockRenderContext context;
    ScreenTheme screen;
    screen.blocks = json::array({{{"type", "text"}, {"text", "x"}}});
    CHECK_EQ(renderPlain(renderScreen(screen, context)), "");
}

TEST_CASE("screens/nested containers render their children") {
    Fixture fixture;
    const json blocks = json::array({
        {{"type", "vbox"},
         {"children", json::array({{{"type", "text"}, {"text", "inner-1"}},
                                   {{"type", "text"}, {"text", "inner-2"}}})}},
    });
    const std::string out = renderBlocks(blocks, fixture.context);
    CHECK(contains(out, "inner-1"));
    CHECK(contains(out, "inner-2"));
}

TEST_CASE("screens/a screen border wraps the page") {
    Fixture fixture;
    ScreenTheme screen;
    screen.border = "light";
    screen.blocks = json::array({{{"type", "text"}, {"text", "boxed"}}});
    const std::string out = renderPlain(renderScreen(screen, fixture.context));
    CHECK(contains(out, "boxed"));
    CHECK(contains(out, "\xE2\x94\x8C"));  // light top-left corner
}

TEST_CASE("screens/dialog buttons are appended to legacy screens") {
    Fixture fixture;
    fixture.context.hasDialogButtons = true;
    fixture.context.dialogButtons = ftxui::text("[Y] Yes");
    ScreenTheme legacy;
    legacy.blocks = json::array({{{"type", "text"}, {"text", "Question?"}}});
    const std::string appended =
        renderPlain(renderScreen(legacy, fixture.context));
    CHECK(contains(appended, "Question?"));
    CHECK(contains(appended, "[Y] Yes"));
}

TEST_CASE("screens/a squeezed quick launch section keeps its input") {
    Fixture fixture;
    fixture.context.quickInput = ftxui::text("quick-input");
    fixture.context.hasQuickInput = true;

    ScreenTheme screen;
    screen.width = 30;
    screen.blocks = json::array({
        {{"type", "text"}, {"text", "header"}, {"height", 5}},
        {{"type", "quickInput"}, {"title", "Section title"}, {"label", ""}},
    });

    auto renderAt = [&](int height) {
        ftxui::Element element = renderScreen(screen, fixture.context);
        ftxui::Screen canvas(30, height);
        ftxui::Render(canvas, element);
        return testfw::stripAnsi(canvas.ToString());
    };

    // The page needs seven rows. One row short, the heading gives way so the
    // input stays usable.
    const std::string tight = renderAt(6);
    CHECK(contains(tight, "quick-input"));
    CHECK_FALSE(contains(tight, "Section title"));

    // With room to spare nothing is dropped.
    const std::string roomy = renderAt(9);
    CHECK(contains(roomy, "Section title"));
    CHECK(contains(roomy, "quick-input"));
}

TEST_CASE("screens/scrollHorizontally pans content through a narrow view") {
    auto view = [](int offset) {
        ftxui::Element element =
            scrollHorizontally(ftxui::text("abcdef"), offset);
        ftxui::Screen canvas(3, 1);
        ftxui::Render(canvas, element);
        return testfw::stripAnsi(canvas.ToString());
    };
    CHECK_EQ(view(0), "abc");
    CHECK_EQ(view(2), "bcd");
    CHECK_EQ(view(5), "def");
    // Past the end the view stays on the last column.
    CHECK_EQ(view(99), "def");
}

TEST_CASE("screens/renderScrollingText centers what fits and pans the rest") {
    MenuTheme theme;

    ftxui::Screen roomy(10, 1);
    ftxui::Render(roomy, renderScrollingText(theme, "abc", 10, 0));
    CHECK_EQ(testfw::stripAnsi(roomy.ToString()), "   abc    ");

    ftxui::Screen tight(4, 1);
    ftxui::Render(tight, renderScrollingText(theme, "abcdefgh", 4, 6));
    CHECK_EQ(testfw::stripAnsi(tight.ToString()), "efgh");
}

TEST_CASE("screens/renderTinyHint shows the theme text and scrolls it") {
    MenuTheme theme;
    theme.tinyHintText = "TOO SMALL";

    auto view = [&](int width, int offset) {
        ftxui::Element element = renderTinyHint(theme, width, 1, offset);
        ftxui::Screen canvas(width, 1);
        ftxui::Render(canvas, element);
        return testfw::stripAnsi(canvas.ToString());
    };

    CHECK_EQ(view(12, 0), " TOO SMALL  ");
    // On a four-column terminal the offsets walk across every character.
    CHECK_EQ(view(4, 0), "TOO ");
    CHECK_EQ(view(4, 5), "SMAL");
    CHECK_EQ(view(4, 8), "MALL");

    // The hint is ordinary theme markup.
    theme.tinyHintText = "<warning>HI</warning>";
    theme.variables["warning"] = "#ff0000";
    CHECK_EQ(view(6, 0), "  HI  ");
}

TEST_CASE("screens/every default screen renders") {
    Fixture fixture;
    fixture.context.pathText = "D:/work";
    fixture.context.modeText = "Add project";
    fixture.context.projectPath = "D:/work";
    fixture.context.projectList = ftxui::text("project-list");
    fixture.context.hasProjectList = true;
    fixture.context.quickInput = ftxui::text("quick-input");
    fixture.context.hasQuickInput = true;
    fixture.context.recentList = ftxui::text("recent-list");
    fixture.context.hasRecentList = true;
    fixture.context.recentCount = 1;
    fixture.context.actionList = ftxui::text("action-list");
    fixture.context.hasActionList = true;
    fixture.context.pathInput = ftxui::text("path-input");
    fixture.context.hasPathInput = true;
    fixture.context.projectNameInput = ftxui::text("name-input");
    fixture.context.hasProjectNameInput = true;
    fixture.context.projectName = "My project";
    fixture.context.dialogButtons = ftxui::text("buttons");
    fixture.context.hasDialogButtons = true;

    for (const char* name :
         {"main", "path", "confirmCreate", "projectActions", "confirmRemove"}) {
        const ScreenTheme& screen = screenThemeFor(fixture.config.menuTheme, name);
        CHECK_FALSE(screen.blocks.empty());
        const std::string out = renderPlain(renderScreen(screen, fixture.context));
        CHECK_FALSE(out.empty());
    }

    const std::string main = renderPlain(renderScreen(
        screenThemeFor(fixture.config.menuTheme, "main"), fixture.context));
    CHECK(contains(main, "Claude Code Portable Launcher"));
    CHECK(contains(main, "project-list"));
    CHECK(contains(main, "quick-input"));
    CHECK(contains(main, "recent-list"));
    CHECK(contains(main, "[Q] Quit"));

    const std::string path = renderPlain(renderScreen(
        screenThemeFor(fixture.config.menuTheme, "path"), fixture.context));
    CHECK(contains(path, "Add project"));
    CHECK(contains(path, "name-input"));
    CHECK(contains(path, "path-input"));
    CHECK(contains(path, "buttons"));

    const std::string create = renderPlain(renderScreen(
        screenThemeFor(fixture.config.menuTheme, "confirmCreate"),
        fixture.context));
    CHECK(contains(create, "Folder does not exist"));
    CHECK(contains(create, "D:/work"));

    const std::string remove = renderPlain(renderScreen(
        screenThemeFor(fixture.config.menuTheme, "confirmRemove"),
        fixture.context));
    CHECK(contains(remove, "Remove project?"));

    const std::string actions = renderPlain(renderScreen(
        screenThemeFor(fixture.config.menuTheme, "projectActions"),
        fixture.context));
    CHECK(contains(actions, "action-list"));
}
