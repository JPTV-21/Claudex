#pragma once

#include "menu_markup.hpp"

#include <ftxui/dom/elements.hpp>

#include <map>
#include <string>
#include <vector>

namespace claudex {

// Everything a screen's blocks can reference while rendering.
struct BlockRenderContext {
    const ConfigManager* config = nullptr;
    const MenuTheme* theme = nullptr;
    std::string projectPath;
    std::string pathText;
    std::string projectName;
    std::string errorText;
    std::string modeText;
    std::string statusText;
    bool statusIsError = false;
    bool showAddProject = false;
    bool hasSelection = false;
    int recentCount = 0;
    ftxui::Element projectList;
    ftxui::Element recentList;
    ftxui::Element actionList;
    ftxui::Element pathInput;
    ftxui::Element projectNameInput;
    ftxui::Element quickInput;
    ftxui::Element dialogButtons;
    bool hasProjectList = false;
    bool hasRecentList = false;
    bool hasActionList = false;
    bool hasPathInput = false;
    bool hasProjectNameInput = false;
    bool hasQuickInput = false;
    bool hasDialogButtons = false;
};

ftxui::Element applyBorderSpec(ftxui::Element element, const std::string& spec,
                               const MenuTheme& theme);
// Columns the border of |spec| adds on each side (0 or 2 total).
int borderSpecWidth(const std::string& spec, const MenuTheme& theme);
ftxui::Element alignElement(ftxui::Element element,
                            const std::string& alignment);

std::map<std::string, std::string> blockPlaceholders(
    const BlockRenderContext& context);
std::string expandBlockText(const std::string& text,
                            const BlockRenderContext& context);
// maxWidth truncates each line with dots; maxLines limits the line count and
// marks the cut on the last visible line.
void appendRenderedText(const std::string& text, const std::string& alignment,
                        const BlockRenderContext& context,
                        std::vector<ftxui::Element>& out, int maxWidth = 0,
                        int maxLines = 0);

// Evaluates a block's "if" / "unless" conditions. Unknown conditions are
// ignored rather than hiding content.
bool blockConditionSatisfied(const json& block,
                             const BlockRenderContext& context);

void renderBlockElements(const json& block, const BlockRenderContext& context,
                         std::vector<ftxui::Element>& out,
                         int availableWidth = 0);
void renderBlockChildren(const json& block, const BlockRenderContext& context,
                         std::vector<ftxui::Element>& out,
                         int availableWidth = 0);

const json* findBlockByType(const json& blocks, const std::string& type,
                            int depth = 0);

ftxui::Element renderScreen(const ScreenTheme& screen,
                            const BlockRenderContext& context);
const ScreenTheme& screenThemeFor(const MenuTheme& theme, const char* name);

enum class NavDir { Left, Right, Up, Down };

// Index of the box that lies in `direction` from boxes[current]. Closer boxes
// win; at equal distance, boxes sharing an edge with the origin win. Empty
// boxes are ignored, and -1 means "nothing in that direction".
int nearestBoxInDirection(const std::vector<ftxui::Box>& boxes, int current,
                          NavDir direction);

}  // namespace claudex
