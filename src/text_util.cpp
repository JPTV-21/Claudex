#include "text_util.hpp"

#include <algorithm>
#include <cctype>

namespace claudex {

bool isAsciiSpace(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string trim(std::string value) {
    std::size_t first = 0;
    while (first < value.size() && isAsciiSpace(value[first])) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && isAsciiSpace(value[last - 1])) {
        --last;
    }
    return value.substr(first, last - first);
}

std::string lowerAscii(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

std::string stripMatchingQuotes(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

void replaceAll(std::string& text, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return;
    }
    std::size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.size(), to);
        position += to.size();
    }
}

bool containsWildcard(std::string_view value) {
    return value.find('*') != std::string_view::npos ||
           value.find('?') != std::string_view::npos;
}

namespace {

bool wildcardCharacterMatches(char pattern, char value,
                              bool caseInsensitive) {
    if (pattern == '?') {
        return true;
    }
    if (!caseInsensitive) {
        return pattern == value;
    }
    return std::tolower(static_cast<unsigned char>(pattern)) ==
           std::tolower(static_cast<unsigned char>(value));
}

}  // namespace

// Matches a filename against '*' and '?' wildcards. The greedy backtracking
// algorithm keeps the worst case linear in the filename length.
bool wildcardMatch(std::string_view pattern, std::string_view value,
                   bool caseInsensitive) {
    std::size_t patternIndex = 0;
    std::size_t valueIndex = 0;
    std::size_t starIndex = std::string_view::npos;
    std::size_t starValueIndex = 0;

    while (valueIndex < value.size()) {
        if (patternIndex < pattern.size() &&
            wildcardCharacterMatches(pattern[patternIndex], value[valueIndex],
                                     caseInsensitive)) {
            ++patternIndex;
            ++valueIndex;
        } else if (patternIndex < pattern.size() &&
                   pattern[patternIndex] == '*') {
            starIndex = patternIndex++;
            starValueIndex = valueIndex;
        } else if (starIndex != std::string_view::npos) {
            patternIndex = starIndex + 1;
            valueIndex = ++starValueIndex;
        } else {
            return false;
        }
    }
    while (patternIndex < pattern.size() && pattern[patternIndex] == '*') {
        ++patternIndex;
    }
    return patternIndex == pattern.size();
}

std::string normalizeMenuToken(std::string value) {
    value = lowerAscii(trim(std::move(value)));
    value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
                    return c == '-' || c == '_' || c == ' ';
                }),
                value.end());
    return value;
}

// launchCommand is a shell command, so the resolved executable path has to be
// quoted for the platform shell rather than for CreateProcess/execv.
std::string quoteShellArgument(const std::string& value) {
#ifdef _WIN32
    std::string result = "\"";
    for (char character : value) {
        if (character == '"') {
            result += "\"\"";
        } else {
            result += character;
        }
    }
    result += '"';
    return result;
#else
    std::string result = "'";
    for (char character : value) {
        if (character == '\'') {
            result += "'\\''";
        } else {
            result += character;
        }
    }
    result += "'";
    return result;
#endif
}

}  // namespace claudex
