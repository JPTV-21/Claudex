#pragma once

#include <string>
#include <string_view>

namespace claudex {

bool isAsciiSpace(char c);
std::string trim(std::string value);
std::string lowerAscii(std::string value);
std::string stripMatchingQuotes(std::string value);
void replaceAll(std::string& text, std::string_view from, std::string_view to);

// Lowercases and drops '-', '_' and ' ' so theme tokens compare loosely.
std::string normalizeMenuToken(std::string value);

// Quotes a value for the platform shell used by launchCommand.
std::string quoteShellArgument(const std::string& value);

bool containsWildcard(std::string_view value);
// Matches a filename against '*' and '?' wildcards.
bool wildcardMatch(std::string_view pattern, std::string_view value,
                   bool caseInsensitive);

}  // namespace claudex
