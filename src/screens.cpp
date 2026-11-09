#include "screens.hpp"

#include "config_json.hpp"
#include "platform.hpp"
#include "text_util.hpp"

#include <ftxui/util/autoreset.hpp>

#include <algorithm>

using namespace ftxui;

namespace claudex {

namespace {

bool isNamedBorderStyle(const std::string& token) {
    return token == "light" || token == "dashed" || token == "heavy" ||
           token == "double" || token == "rounded" || token == "empty";
}

}  // namespace

int borderSpecWidth(const std::string& spec, const MenuTheme& theme) {
    const std::string token = normalizeMenuToken(spec);
    if (token.empty() || token == "none" || token == "empty") {
        return 0;
    }
    if (token == "theme") {
        if (parseBorderCharacters(theme.borderCharacters)) {
            return 2;
        }
        const std::string style = normalizeMenuToken(theme.borderStyle);
        if (style.empty() || style == "none" || style == "empty") {
            return 0;
        }
        return 2;
    }
    if (isNamedBorderStyle(token) || parseBorderCharacters(spec)) {
        return 2;
    }
    return 0;
}

Element applyBorderSpec(Element element, const std::string& spec,
                        const MenuTheme& theme) {
    const std::string token = normalizeMenuToken(spec);
    if (token.empty() || token == "none") {
        return element;
    }
    if (token == "theme") {
        return applyMenuBorder(std::move(element), theme);
    }
    if (isNamedBorderStyle(token)) {
        return element | borderStyled(parseBorderStyle(spec));
    }
    if (const auto charset = parseBorderCharacters(spec)) {
        return element | borderWithCharset(*charset);
    }
    return element;
}

Element alignElement(Element element, const std::string& alignment) {
    const std::string value = normalizeMenuToken(alignment);
    if (value == "center") {
        return element | hcenter;
    }
    if (value == "right") {
        return hbox({filler(), std::move(element)});
    }
    return element;
}

Element scrollHorizontally(Element content, int offset) {
    // The frame scrolls to the focused position, so a point focus at |offset|
    // pans the content without having to slice the (possibly multi-byte)
    // text by hand.
    return std::move(content) | focusPosition(std::max(0, offset), 0) | xframe;
}

Element renderScrollingText(const MenuTheme& theme, const std::string& text,
                            int width, int offset) {
    if (text.empty() || width <= 0) {
        return ftxui::text("");
    }
    Element line = renderThemeText(theme, text);
    if (menuMarkupDisplayWidth(text, &theme.variables) <= width) {
        return alignElement(std::move(line), "center");
    }
    return scrollHorizontally(std::move(line), offset);
}

Element renderTinyHint(const MenuTheme& theme, int width, int height,
                       int offset) {
    if (width <= 0 || height <= 0) {
        return ftxui::text("");
    }
    return renderScrollingText(theme, theme.tinyHintText, width, offset);
}

std::map<std::string, std::string> blockPlaceholders(
    const BlockRenderContext& context) {
    return {{"path", context.pathText},
            {"name", context.projectName},
            {"error", context.errorText},
            {"mode", context.modeText},
            {"status", context.statusText},
            {"cwd", context.cwdPath}};
}

std::string expandBlockText(const std::string& text,
                            const BlockRenderContext& context) {
    if (context.config == nullptr) {
        return text;
    }
    return expandMenuText(text, *context.config, context.projectPath,
                          blockPlaceholders(context));
}

namespace {

// Renders |child| but drops anything that falls outside the box it was given,
// so a shrunken vbox cannot paint over its siblings.
class ClipNode : public Node {
public:
    explicit ClipNode(Element child) : Node(Elements{std::move(child)}) {}

    void ComputeRequirement() override {
        Node::ComputeRequirement();
        requirement_ = children_[0]->requirement();
    }

    void SetBox(Box box) override {
        Node::SetBox(box);
        children_[0]->SetBox(box);
    }

    void Render(Screen& screen) override {
        const AutoReset<Box> stencil(
            &screen.stencil, Box::Intersection(box_, screen.stencil));
        children_[0]->Render(screen);
    }
};

Element clipToBox(Element child) {
    return std::make_shared<ClipNode>(std::move(child));
}

}  // namespace

void appendRenderedText(const std::string& text,
                        const std::string& alignment,
                        const BlockRenderContext& context,
                        std::vector<Element>& out, int maxWidth,
                        int maxLines) {
    if (text.empty()) {
        return;
    }
    const std::string expanded = expandBlockText(text, context);
    const auto* colors =
        context.theme != nullptr ? &context.theme->variables : nullptr;
    std::vector<std::string> lines = splitMenuLines(expanded);
    // A height limit cuts whole lines; the last visible line says so with dots
    // before it is truncated to the width.
    if (maxLines > 0 && static_cast<int>(lines.size()) > maxLines) {
        lines.resize(static_cast<std::size_t>(maxLines));
        lines.back() += "...";
    }
    for (const std::string& line : lines) {
        out.push_back(
            alignMenuLine(renderMenuMarkup(line, colors, maxWidth), alignment));
    }
}

bool blockConditionSatisfied(const json& block,
                             const BlockRenderContext& context) {
    auto matches = [&](const std::string& condition) {
        const std::string token = normalizeMenuToken(condition);
        if (token == "projects") {
            return context.config != nullptr &&
                   !context.config->projects.empty();
        }
        if (token == "noprojects" || token == "empty") {
            return context.config != nullptr &&
                   context.config->projects.empty();
        }
        if (token == "selection" || token == "selected") {
            return context.hasSelection;
        }
        if (token == "noselection" || token == "noselected") {
            return !context.hasSelection;
        }
        if (token == "status") {
            return !context.statusText.empty();
        }
        if (token == "nostatus") {
            return context.statusText.empty();
        }
        if (token == "error") {
            return !context.errorText.empty();
        }
        if (token == "noerror") {
            return context.errorText.empty();
        }
        if (token == "addproject") {
            return context.showAddProject;
        }
        if (token == "noaddproject") {
            return !context.showAddProject;
        }
        if (token == "recent" || token == "recentprojects") {
            return context.recentCount > 0;
        }
        if (token == "norecent" || token == "norecentprojects") {
            return context.recentCount == 0;
        }
        if (token == "quickinput") {
            return context.hasQuickInput;
        }
        if (token == "cwd" || token == "cwdlist" || token == "currentfolder") {
            return context.hasCwdList;
        }
        if (token == "nocwd" || token == "nocwdlist") {
            return !context.hasCwdList;
        }
        // Unknown conditions are ignored rather than hiding content.
        return true;
    };

    if (block.contains("if") && block["if"].is_string()) {
        if (!matches(block["if"].get<std::string>())) {
            return false;
        }
    }
    if (block.contains("unless") && block["unless"].is_string()) {
        if (matches(block["unless"].get<std::string>())) {
            return false;
        }
    }
    return true;
}

void renderBlockChildren(const json& block,
                         const BlockRenderContext& context,
                         std::vector<Element>& out, int availableWidth) {
    if (!block.contains("children") || !block["children"].is_array()) {
        return;
    }
    for (const json& child : block["children"]) {
        renderBlockElements(child, context, out, availableWidth);
    }
}

void renderBlockElements(const json& block,
                         const BlockRenderContext& context,
                         std::vector<Element>& out, int availableWidth) {
    if (!block.is_object() || context.theme == nullptr ||
        !blockConditionSatisfied(block, context)) {
        return;
    }

    const MenuTheme& theme = *context.theme;
    const auto* variables = &theme.variables;
    const std::string type =
        normalizeMenuToken(jsonStringField(block, "type", "text"));
    const std::string align = jsonStringField(block, "align", "left");

    // width/height are exact sizes; maxWidth/maxHeight bound from above and
    // minWidth/minHeight from below.
    const int exactWidth = jsonIntFieldResolved(block, "width", 0, variables);
    const int exactHeight = jsonIntFieldResolved(block, "height", 0, variables);
    const int maxWidth = jsonIntFieldResolved(block, "maxWidth", 0, variables);
    const int maxHeight = jsonIntFieldResolved(block, "maxHeight", 0, variables);
    const int minWidth = jsonIntFieldResolved(block, "minWidth", 0, variables);
    const int minHeight = jsonIntFieldResolved(block, "minHeight", 0, variables);

    // Text is truncated to the tightest width this block knows about.
    const int textWidth =
        exactWidth > 0 ? exactWidth : (maxWidth > 0 ? maxWidth : availableWidth);
    const int textLines = exactHeight > 0 ? exactHeight : maxHeight;

    std::vector<Element> produced;

    if (type == "text") {
        appendRenderedText(jsonStringField(block, "text", ""), align,
                           context, produced, textWidth, textLines);
    } else if (type == "title") {
        const bool project =
            normalizeMenuToken(jsonStringField(block, "scope", "main")) ==
            "project";
        const std::string defaultText =
            project ? theme.projectMenu.titleFormat : theme.titleFormat;
        const std::string text = jsonStringField(block, "text", defaultText);
        const std::string titleAlign =
            jsonStringField(block, "align", theme.titleAlign);
        appendRenderedText(text, titleAlign, context, produced, textWidth,
                           textLines);
    } else if (type == "projectformat") {
        const std::string titleAlign =
            jsonStringField(block, "align", theme.titleAlign);
        appendRenderedText(theme.projectMenu.projectFormat, titleAlign,
                           context, produced, textWidth, textLines);
    } else if (type == "separator") {
        const std::string orientation =
            normalizeMenuToken(jsonStringField(block, "orientation", ""));
        const bool vertical =
            jsonBoolField(block, "vertical", false) || orientation == "vertical";
        if (auto separator = makeMenuSeparatorFor(
                jsonStringField(block, "style", theme.separatorStyle),
                jsonStringField(block, "character",
                                theme.separatorCharacter))) {
            Element line = std::move(*separator);
            // The style-based separators pick their glyph from the shape of
            // their box, so pinning the width to one column makes it vertical.
            if (vertical) {
                line = std::move(line) | size(WIDTH, EQUAL, 1);
            }
            produced.push_back(std::move(line));
        }
    } else if (type == "blank") {
        produced.push_back(text(""));
    } else if (type == "status") {
        if (!context.statusText.empty()) {
            produced.push_back(
                hbox({text(" "),
                      renderThemeText(theme, context.statusText)}));
        }
    } else if (type == "hint") {
        if (context.statusText.empty()) {
            produced.push_back(
                hbox({text(" "),
                      renderThemeText(theme,
                                      expandBlockText(theme.hintText,
                                                      context))}));
        }
    } else if (type == "statusorhint") {
        if (!context.statusText.empty()) {
            produced.push_back(
                hbox({text(" "),
                      renderThemeText(theme, context.statusText)}));
        } else {
            produced.push_back(
                hbox({text(" "),
                      renderThemeText(theme,
                                      expandBlockText(theme.hintText,
                                                      context))}));
        }
    } else if (type == "error") {
        if (!context.errorText.empty()) {
            produced.push_back(
                hbox({text(" "),
                      renderThemeText(theme, context.errorText)}));
        } else {
            appendRenderedText(jsonStringField(block, "fallback", ""),
                               align, context, produced, textWidth, textLines);
        }
    } else if (type == "footer") {
        std::vector<std::string> lines;
        if (block.contains("lines") && block["lines"].is_array()) {
            for (const json& line : block["lines"]) {
                if (line.is_string()) {
                    lines.push_back(line.get<std::string>());
                }
            }
        } else if (normalizeMenuToken(
                       jsonStringField(block, "scope", "main")) ==
                   "project") {
            lines = theme.projectMenu.footerLines;
        } else {
            lines = theme.footerLines;
        }

        const std::string footerAlign =
            jsonStringField(block, "align", theme.footerAlign);
        std::vector<std::string> rendered;
        for (const std::string& line : lines) {
            for (const std::string& part :
                 splitMenuLines(expandBlockText(line, context))) {
                rendered.push_back(part);
            }
        }
        if (textLines > 0 &&
            static_cast<int>(rendered.size()) > textLines) {
            rendered.resize(static_cast<std::size_t>(textLines));
            rendered.back() += "...";
        }
        for (const std::string& part : rendered) {
            produced.push_back(alignMenuLine(
                renderThemeText(theme, part, textWidth), footerAlign));
        }
    } else if (type == "cwdlist") {
        if (context.hasCwdList && context.cwdList) {
            const std::string titleText =
                jsonStringField(block, "title", theme.cwdTitleFormat);
            if (!trim(titleText).empty()) {
                appendRenderedText(
                    titleText,
                    jsonStringField(block, "titleAlign", "left"), context,
                    produced, textWidth, textLines);
            }
            produced.push_back(context.cwdList);
        }
    } else if (type == "projectlist") {
        if (context.hasProjectList && context.projectList) {
            produced.push_back(context.projectList);
        }
    } else if (type == "recentlist") {
        if (context.recentCount > 0) {
            const std::string titleText =
                jsonStringField(block, "title", theme.recentTitleFormat);
            const std::string titleAlign =
                jsonStringField(block, "titleAlign", "left");
            appendRenderedText(titleText, titleAlign, context, produced, textWidth, textLines);
            if (context.hasRecentList && context.recentList) {
                produced.push_back(context.recentList);
            }
        } else {
            appendRenderedText(
                jsonStringField(block, "emptyText", theme.recentEmptyText),
                align, context, produced, textWidth, textLines);
        }
    } else if (type == "quickinput") {
        if (context.hasQuickInput && context.quickInput) {
            // An optional title turns the input into its own section, the same
            // way recentList draws its heading above the list.
            const std::string title = jsonStringField(block, "title", "");
            if (!title.empty()) {
                const std::size_t firstTitleLine = produced.size();
                appendRenderedText(
                    title, jsonStringField(block, "titleAlign", align),
                    context, produced, textWidth, textLines);
                // The input is the feature; the heading only labels it. When
                // vertical space is so tight that the section cannot show
                // both, let the heading collapse first instead of pushing the
                // input out of the page.
                for (std::size_t i = firstTitleLine; i < produced.size();
                     ++i) {
                    produced[i] = std::move(produced[i]) | yflex_shrink;
                }
            }
            const std::string label = jsonStringField(
                block, "label", theme.quickInputLabel);
            if (!label.empty()) {
                produced.push_back(hbox(
                    {renderThemeText(theme,
                                     expandBlockText(label, context)),
                     text(" "),
                     context.quickInput | flex}));
            } else {
                produced.push_back(
                    hbox({text(" "), context.quickInput | flex}));
            }
        }
    } else if (type == "actionlist") {
        if (context.hasActionList && context.actionList) {
            produced.push_back(context.actionList);
        }
    } else if (type == "pathinput") {
        if (context.hasPathInput && context.pathInput) {
            produced.push_back(context.pathInput);
        }
    } else if (type == "nameinput") {
        if (context.hasProjectNameInput && context.projectNameInput) {
            produced.push_back(context.projectNameInput);
        }
    } else if (type == "buttons" || type == "dialogbuttons") {
        if (context.hasDialogButtons && context.dialogButtons) {
            produced.push_back(context.dialogButtons);
        }
    } else if (type == "spacer") {
        produced.push_back(filler());
    } else if (type == "box") {
        std::vector<Element> children;
        const std::string border = jsonStringField(block, "border", "none");
        const int innerWidth =
            textWidth > 0 ? textWidth - borderSpecWidth(border, theme) : 0;
        renderBlockChildren(block, context, children, innerWidth);
        if (!children.empty()) {
            Element inner = vbox(std::move(children));
            inner = applyBorderSpec(std::move(inner), border, theme);
            produced.push_back(std::move(inner));
        }
    } else if (type == "hbox") {
        std::vector<Element> children;
        if (block.contains("children") && block["children"].is_array()) {
            for (const json& child : block["children"]) {
                std::vector<Element> childElements;
                // The width is shared by the row, so each child only knows the
                // whole width; documented as an approximation.
                renderBlockElements(child, context, childElements, textWidth);
                if (childElements.empty()) {
                    continue;
                }
                Element childElement =
                    childElements.size() == 1
                        ? childElements.front()
                        : vbox(std::move(childElements));
                if (jsonBoolField(child, "flex", false)) {
                    childElement = childElement | flex;
                }
                children.push_back(std::move(childElement));
            }
        }
        if (!children.empty()) {
            produced.push_back(hbox(std::move(children)));
        }
    } else if (type == "vbox") {
        std::vector<Element> children;
        renderBlockChildren(block, context, children, textWidth);
        if (!children.empty()) {
            produced.push_back(vbox(std::move(children)));
        }
    }

    if (produced.empty()) {
        return;
    }

    const auto blockForeground = parseThemeColor(
        jsonStringField(block, "color", ""), &theme.variables);
    const auto blockBackground = parseThemeColor(
        jsonStringField(block, "backgroundColor",
                        jsonStringField(block, "bgColor", "")),
        &theme.variables);
    const bool layoutAlign =
        type == "box" || type == "hbox" || type == "vbox" ||
        type == "projectlist" || type == "recentlist" ||
        type == "cwdlist" || type == "actionlist" ||
        type == "pathinput" ||
        type == "nameinput" || type == "quickinput" || type == "spacer" ||
        type == "buttons" || type == "dialogbuttons";

    // Every line the block produced is one block for sizing purposes, so a
    // height limit applies to the group instead of to each line separately.
    Element group = produced.size() == 1
                        ? std::move(produced.front())
                        : vbox(std::move(produced));
    group = clipToBox(std::move(group));

    if (blockForeground) {
        group = group | color(*blockForeground);
    }
    if (blockBackground) {
        group = group | bgcolor(*blockBackground);
    }
    if (minWidth > 0) {
        group = group | size(WIDTH, GREATER_THAN, minWidth);
    }
    if (exactWidth > 0) {
        group = group | size(WIDTH, GREATER_THAN, exactWidth);
        group = group | size(WIDTH, LESS_THAN, exactWidth);
    } else if (maxWidth > 0) {
        group = group | size(WIDTH, LESS_THAN, maxWidth);
    }
    if (minHeight > 0) {
        group = group | size(HEIGHT, GREATER_THAN, minHeight);
    }
    if (exactHeight > 0) {
        group = group | size(HEIGHT, GREATER_THAN, exactHeight);
        group = group | size(HEIGHT, LESS_THAN, exactHeight);
    } else if (maxHeight > 0) {
        group = group | size(HEIGHT, LESS_THAN, maxHeight);
    }
    if (layoutAlign && normalizeMenuToken(align) != "left") {
        group = alignElement(std::move(group), align);
    }

    if (jsonBoolField(block, "separatorBefore", false)) {
        if (auto separator = makeMenuSeparatorFor(
                jsonStringField(block, "separatorStyle",
                                theme.separatorStyle),
                jsonStringField(block, "separatorCharacter",
                                theme.separatorCharacter))) {
            out.push_back(std::move(*separator));
        }
    }
    out.push_back(std::move(group));
    if (jsonBoolField(block, "separatorAfter", false)) {
        if (auto separator = makeMenuSeparatorFor(
                jsonStringField(block, "separatorStyle",
                                theme.separatorStyle),
                jsonStringField(block, "separatorCharacter",
                                theme.separatorCharacter))) {
            out.push_back(std::move(*separator));
        }
    }
}

Element renderScreen(const ScreenTheme& screen,
                     const BlockRenderContext& context) {
    std::vector<Element> rows;
    // Text on the page is truncated at the page edge, border included.
    const int contentWidth =
        screen.width > 0 && context.theme != nullptr
            ? screen.width - borderSpecWidth(screen.border, *context.theme)
            : 0;
    for (const json& block : screen.blocks) {
        renderBlockElements(block, context, rows, contentWidth);
    }
    // Themes written before the buttons block existed still get clickable
    // dialog buttons: put them at the end of the page instead.
    const bool hasButtonsBlock =
        findBlockByType(screen.blocks, "buttons") != nullptr ||
        findBlockByType(screen.blocks, "dialogButtons") != nullptr;
    if (!hasButtonsBlock && context.hasDialogButtons &&
        context.dialogButtons) {
        rows.push_back(text(""));
        rows.push_back(context.dialogButtons);
    }
    if (rows.empty()) {
        return text("");
    }

    Element page = vbox(std::move(rows));
    if (context.theme != nullptr) {
        page = applyBorderSpec(std::move(page), screen.border,
                               *context.theme);
    }
    if (screen.width > 0) {
        page = page | size(WIDTH, LESS_THAN, screen.width);
    }
    return alignElement(std::move(page), screen.align);
}

namespace {

int overlapGap(int aMin, int aMax, int bMin, int bMax) {
    if (aMax < bMin) {
        return bMin - aMax;
    }
    if (bMax < aMin) {
        return aMin - bMax;
    }
    return 0;
}

}  // namespace

int nearestBoxInDirection(const std::vector<Box>& boxes, int current,
                          NavDir direction) {
    if (current < 0 || current >= static_cast<int>(boxes.size())) {
        return -1;
    }
    const Box& origin = boxes[static_cast<std::size_t>(current)];
    if (origin.IsEmpty()) {
        return -1;
    }
    const int originX = (origin.x_min + origin.x_max) / 2;
    const int originY = (origin.y_min + origin.y_max) / 2;

    int best = -1;
    int bestScore = 0;
    for (int index = 0; index < static_cast<int>(boxes.size()); ++index) {
        if (index == current) {
            continue;
        }
        const Box& box = boxes[static_cast<std::size_t>(index)];
        if (box.IsEmpty()) {
            continue;
        }
        const int centerX = (box.x_min + box.x_max) / 2;
        const int centerY = (box.y_min + box.y_max) / 2;
        int primary = 0;
        int secondary = 0;
        switch (direction) {
            case NavDir::Left:
                if (centerX >= originX) {
                    continue;
                }
                primary = originX - centerX;
                secondary = overlapGap(origin.y_min, origin.y_max, box.y_min,
                                       box.y_max);
                break;
            case NavDir::Right:
                if (centerX <= originX) {
                    continue;
                }
                primary = centerX - originX;
                secondary = overlapGap(origin.y_min, origin.y_max, box.y_min,
                                       box.y_max);
                break;
            case NavDir::Up:
                if (centerY >= originY) {
                    continue;
                }
                primary = originY - centerY;
                secondary = overlapGap(origin.x_min, origin.x_max, box.x_min,
                                       box.x_max);
                break;
            case NavDir::Down:
                if (centerY <= originY) {
                    continue;
                }
                primary = centerY - originY;
                secondary = overlapGap(origin.x_min, origin.x_max, box.x_min,
                                       box.x_max);
                break;
        }
        // Staying in the same row/column matters more than being slightly
        // closer: a section that lines up with the origin wins over one that
        // is nearer but sits across the divider.
        const int score = secondary * 8 + primary;
        if (best < 0 || score < bestScore) {
            best = index;
            bestScore = score;
        }
    }
    return best;
}

const ScreenTheme& screenThemeFor(const MenuTheme& theme,
                                  const char* name) {
    static const ScreenTheme empty;
    auto item = theme.screens.find(std::string(name));
    return item == theme.screens.end() ? empty : item->second;
}

const json* findBlockByType(const json& blocks, const std::string& type,
                            int depth) {
    if (!blocks.is_array() || depth > 32) {
        return nullptr;
    }
    const std::string normalized = normalizeMenuToken(type);
    for (const json& block : blocks) {
        if (!block.is_object()) {
            continue;
        }
        if (normalizeMenuToken(jsonStringField(block, "type", "")) ==
            normalized) {
            return &block;
        }
        if (block.contains("children") && block["children"].is_array()) {
            if (const json* found = findBlockByType(block["children"], type,
                                                    depth + 1)) {
                return found;
            }
        }
    }
    return nullptr;
}

}  // namespace claudex
