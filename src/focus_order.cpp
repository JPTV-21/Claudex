#include "focus_order.hpp"

#include "text_util.hpp"

#include <algorithm>
#include <string>

namespace claudex {

namespace {

// A theme is user input; cap the recursion so a pathological nesting depth
// cannot overflow the stack while collecting focus targets.
constexpr int kMaxBlockDepth = 32;

void collectPathFieldOrderFrom(const json& blocks,
                               std::vector<PathField>& order, int depth) {
    if (!blocks.is_array() || depth > kMaxBlockDepth) {
        return;
    }
    for (const json& block : blocks) {
        if (!block.is_object()) {
            continue;
        }
        const std::string type =
            normalizeMenuToken(jsonStringField(block, "type", ""));
        PathField field = PathField::Path;
        bool found = false;
        if (type == "nameinput") {
            field = PathField::Name;
            found = true;
        } else if (type == "pathinput") {
            field = PathField::Path;
            found = true;
        }
        if (found && std::find(order.begin(), order.end(), field) ==
                         order.end()) {
            order.push_back(field);
        }
        if (block.contains("children") && block["children"].is_array()) {
            collectPathFieldOrderFrom(block["children"], order, depth + 1);
        }
    }
}

void collectMainFocusOrderFrom(const json& blocks, std::vector<int>& order,
                               int depth) {
    if (!blocks.is_array() || depth > kMaxBlockDepth) {
        return;
    }
    for (const json& block : blocks) {
        if (!block.is_object()) {
            continue;
        }
        const std::string type =
            normalizeMenuToken(jsonStringField(block, "type", ""));
        int node = -1;
        if (type == "projectlist") {
            node = kMainFocusProjects;
        } else if (type == "recentlist") {
            node = kMainFocusRecent;
        } else if (type == "quickinput") {
            node = kMainFocusQuickInput;
        } else if (type == "cwdlist") {
            node = kMainFocusCwd;
        }
        if (node >= 0 && std::find(order.begin(), order.end(), node) ==
                             order.end()) {
            order.push_back(node);
        }
        if (block.contains("children") && block["children"].is_array()) {
            collectMainFocusOrderFrom(block["children"], order, depth + 1);
        }
    }
}

}  // namespace

void collectPathFieldOrder(const json& blocks, std::vector<PathField>& order) {
    collectPathFieldOrderFrom(blocks, order, 0);
}

void collectMainFocusOrder(const json& blocks, std::vector<int>& order) {
    collectMainFocusOrderFrom(blocks, order, 0);
}

int focusIndexOf(const std::vector<int>& order, int node) {
    const auto found = std::find(order.begin(), order.end(), node);
    return found == order.end()
               ? -1
               : static_cast<int>(found - order.begin());
}

int moveEnabledIndex(const std::vector<bool>& enabled, int current, int step,
                     bool wrap) {
    std::vector<int> available;
    available.reserve(enabled.size());
    for (int index = 0; index < static_cast<int>(enabled.size()); ++index) {
        if (enabled[static_cast<std::size_t>(index)]) {
            available.push_back(index);
        }
    }
    if (available.empty()) {
        return -1;
    }

    const auto found = std::find(available.begin(), available.end(), current);
    const int position =
        found == available.end()
            ? 0
            : static_cast<int>(found - available.begin());
    int next = position + step;
    if (wrap) {
        next = (next + static_cast<int>(available.size())) %
               static_cast<int>(available.size());
    } else if (next < 0 || next >= static_cast<int>(available.size())) {
        return -1;
    }
    return available[static_cast<std::size_t>(next)];
}

}  // namespace claudex
