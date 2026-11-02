#include "config_types.hpp"

#include "text_util.hpp"

#include <algorithm>
#include <cctype>

namespace claudex {

std::string messageText(const MenuTheme& theme, const char* key) {
    return theme.messages.at(key);
}

std::string messageText(
    const MenuTheme& theme, const char* key,
    std::initializer_list<std::pair<std::string, std::string>> values) {
    std::string text = theme.messages.at(key);
    for (const auto& value : values) {
        replaceAll(text, "{" + value.first + "}", value.second);
    }
    return text;
}

std::string jsonStringField(const json& object, const char* key,
                            const std::string& fallback) {
    if (object.is_object()) {
        auto item = object.find(key);
        if (item != object.end() && item->is_string()) {
            return item->get<std::string>();
        }
    }
    return fallback;
}

bool jsonBoolField(const json& object, const char* key, bool fallback) {
    if (object.is_object()) {
        auto item = object.find(key);
        if (item != object.end() && item->is_boolean()) {
            return item->get<bool>();
        }
    }
    return fallback;
}

int jsonIntField(const json& object, const char* key, int fallback) {
    if (object.is_object()) {
        auto item = object.find(key);
        if (item != object.end() && item->is_number_integer()) {
            return item->get<int>();
        }
    }
    return fallback;
}

std::optional<int> resolveThemeInt(
    const json& value, const std::map<std::string, std::string>* variables,
    int depth) {
    if (value.is_number_integer()) {
        return value.get<int>();
    }
    if (!value.is_string()) {
        return std::nullopt;
    }

    const std::string text = trim(value.get<std::string>());
    if (text.empty()) {
        return std::nullopt;
    }
    const bool numeric =
        std::all_of(text.begin(), text.end(), [](char c) {
            return std::isdigit(static_cast<unsigned char>(c)) != 0;
        });
    if (numeric) {
        try {
            return std::stoi(text);
        } catch (...) {
            return std::nullopt;
        }
    }

    if (variables == nullptr || depth >= 8) {
        return std::nullopt;
    }
    auto found = variables->find(normalizeMenuToken(text));
    if (found == variables->end()) {
        return std::nullopt;
    }
    return resolveThemeInt(json(found->second), variables, depth + 1);
}

int jsonIntFieldResolved(const json& object, const char* key, int fallback,
                         const std::map<std::string, std::string>* variables) {
    if (!object.is_object()) {
        return fallback;
    }
    auto item = object.find(key);
    if (item == object.end()) {
        return fallback;
    }
    const std::optional<int> value = resolveThemeInt(*item, variables);
    return value ? *value : fallback;
}

}  // namespace claudex
