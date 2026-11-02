#include "test_framework.hpp"

#include "config_json.hpp"
#include "config_manager.hpp"
#include "render_helper.hpp"
#include "screens.hpp"

#include <fstream>

using namespace claudex;
using testfw::renderPlain;
using testfw::TempDir;

namespace {

fs::path presetDirectory() {
    return fs::path(CLAUDEX_SOURCE_DIR) / "resources" / "presets";
}

json readPreset(const std::string& name) {
    const fs::path file = presetDirectory() / name;
    std::ifstream input(file, std::ios::binary);
    REQUIRE_MESSAGE(input.good(), "missing preset " + pathToUtf8(file));
    return json::parse(input);
}

// Presets are partial themes: merge them onto the defaults the same way
// ConfigManager::loadTheme does.
// Mirrors ConfigManager::loadTheme: the preset's own definitions are kept
// as-is, and the palette falls back to the embedded default theme.
MenuTheme mergedTheme(const json& preset) {
    const MenuTheme defaults = menuThemeFromDocument(embeddedDefaultTheme());
    const json merged = mergeThemeDefaults(embeddedDefaultTheme(), preset);
    return menuThemeFromDocument(merged, &defaults);
}

}  // namespace

TEST_CASE("presets/every preset is a JSON object") {
    std::error_code ec;
    std::vector<fs::path> files;
    for (const fs::directory_entry& entry :
         fs::directory_iterator(presetDirectory(), ec)) {
        if (entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    REQUIRE(!ec);
    REQUIRE(!files.empty());

    for (const fs::path& file : files) {
        std::ifstream input(file, std::ios::binary);
        REQUIRE(input.good());
        json document;
        REQUIRE_NOTHROW(document = json::parse(input));
        CHECK(document.is_object());
    }
}

TEST_CASE("presets/two-column theme is a valid partial theme") {
    const json preset = readPreset("two-column.theme.json");
    const MenuTheme theme = mergedTheme(preset);
    CHECK_FALSE(theme.titleFormat.empty());
    CHECK(theme.screens.count("main") == 1);
    CHECK_FALSE(theme.screens.at("main").blocks.empty());
    // The preset does not define a palette, but names still resolve.
    CHECK_EQ(theme.variables.count("red"), static_cast<std::size_t>(1));
    CHECK_EQ(theme.variables.count("accent"), static_cast<std::size_t>(1));
}

TEST_CASE("presets/two-column theme renders a left column, divider and right column") {
    TempDir root;
    ConfigManager config(root.path(), root.path() / "claudex-config.json");
    config.load();

    const MenuTheme theme = mergedTheme(readPreset("two-column.theme.json"));

    BlockRenderContext context;
    context.config = &config;
    context.theme = &theme;
    context.projectList = ftxui::text("alpha\nbravo");
    context.hasProjectList = true;
    context.recentList = ftxui::text("recent-one");
    context.hasRecentList = true;
    context.recentCount = 1;
    context.quickInput = ftxui::text("[ type here ]");
    context.hasQuickInput = true;

    ScreenTheme screen = theme.screens.at("main");
    ftxui::Element element = renderScreen(screen, context);
    ftxui::Screen canvas = ftxui::Screen::Create(ftxui::Dimension::Fixed(76),
                                                  ftxui::Dimension::Fit(element));
    ftxui::Render(canvas, element);
    const std::string out = testfw::stripAnsi(canvas.ToString());

    CHECK(out.find("alpha") != std::string::npos);
    CHECK(out.find("recent-one") != std::string::npos);
    CHECK(out.find("[ type here ]") != std::string::npos);
    // The vertical divider sits right after the fixed-width left box.
    CHECK(out.find("\xE2\x94\x90\xE2\x94\x82") != std::string::npos);  // "┐│"

    // Both columns share every row of the hbox.
    std::size_t dividerRows = 0;
    std::size_t lineStart = 0;
    while (lineStart < out.size()) {
        std::size_t lineEnd = out.find('\n', lineStart);
        if (lineEnd == std::string::npos) {
            lineEnd = out.size();
        }
        const std::string line = out.substr(lineStart, lineEnd - lineStart);
        if (line.find("\xE2\x94\x82") != std::string::npos) {  // "│"
            ++dividerRows;
        }
        lineStart = lineEnd + 1;
    }
    CHECK(dividerRows >= 3);
}

TEST_CASE("presets/the shipped default theme is not one of the presets") {
    // The presets are examples only; the embedded default must stay separate.
    std::error_code ec;
    for (const fs::directory_entry& entry :
         fs::directory_iterator(presetDirectory(), ec)) {
        if (entry.path().extension() != ".json") {
            continue;
        }
        std::ifstream input(entry.path(), std::ios::binary);
        const json document = json::parse(input);
        CHECK(document != embeddedDefaultConfig());
    }
}
