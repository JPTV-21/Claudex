#pragma once

#include "config_json.hpp"
#include "text_util.hpp"

#include <map>
#include <string>

namespace testfw {

// The color and size definitions shipped in resources/default-theme.json.
// Markup tags only resolve names that the active theme defines, so tests that
// use <red> or <success> must pass this table the way the launcher does.
inline const std::map<std::string, std::string>& defaultPalette() {
    static const std::map<std::string, std::string> palette = [] {
        std::map<std::string, std::string> definitions;
        const claudex::json& variables =
            claudex::embeddedDefaultTheme().at("variables");
        for (auto item = variables.begin(); item != variables.end(); ++item) {
            if (item.value().is_string()) {
                definitions[claudex::normalizeMenuToken(item.key())] =
                    item.value().get<std::string>();
            }
        }
        return definitions;
    }();
    return palette;
}

}  // namespace testfw
