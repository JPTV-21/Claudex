#include "test_framework.hpp"

#include "text_util.hpp"

using namespace claudex;

TEST_CASE("text/trim removes surrounding ASCII whitespace") {
    CHECK_EQ(trim("  hello \t\r\n"), "hello");
    CHECK_EQ(trim("no-space"), "no-space");
    CHECK_EQ(trim("   "), "");
    CHECK_EQ(trim(""), "");
    CHECK_EQ(trim("  a b  "), "a b");
}

TEST_CASE("text/isAsciiSpace") {
    CHECK(isAsciiSpace(' '));
    CHECK(isAsciiSpace('\t'));
    CHECK(isAsciiSpace('\n'));
    CHECK_FALSE(isAsciiSpace('a'));
    // Bytes of multi-byte UTF-8 sequences must never count as whitespace.
    CHECK_FALSE(isAsciiSpace(static_cast<char>(0xA0)));
}

TEST_CASE("text/lowerAscii only touches ASCII letters") {
    CHECK_EQ(lowerAscii("ClaudeX-ABC_123"), "claudex-abc_123");
    CHECK_EQ(lowerAscii("\xE9\xA1\xB9\xE7\x9B\xAE"), "\xE9\xA1\xB9\xE7\x9B\xAE");
}

TEST_CASE("text/stripMatchingQuotes") {
    CHECK_EQ(stripMatchingQuotes("\"abc\""), "abc");
    CHECK_EQ(stripMatchingQuotes("'abc'"), "abc");
    CHECK_EQ(stripMatchingQuotes("  \"abc\"  "), "abc");
    CHECK_EQ(stripMatchingQuotes("\"abc'"), "\"abc'");
    CHECK_EQ(stripMatchingQuotes("\""), "\"");
    CHECK_EQ(stripMatchingQuotes("\"\""), "");
    CHECK_EQ(stripMatchingQuotes("plain"), "plain");
}

TEST_CASE("text/replaceAll") {
    std::string text = "a{x}b{x}c";
    replaceAll(text, "{x}", "1");
    CHECK_EQ(text, "a1b1c");

    // The replacement is not rescanned, so self-containing values terminate.
    text = "aa";
    replaceAll(text, "a", "aa");
    CHECK_EQ(text, "aaaa");

    text = "unchanged";
    replaceAll(text, "", "x");
    CHECK_EQ(text, "unchanged");

    text = "gone gone";
    replaceAll(text, "gone", "");
    CHECK_EQ(text, " ");
}

TEST_CASE("text/normalizeMenuToken") {
    CHECK_EQ(normalizeMenuToken("  Gray-Dark "), "graydark");
    CHECK_EQ(normalizeMenuToken("project_List"), "projectlist");
    CHECK_EQ(normalizeMenuToken("Quick Input"), "quickinput");
    CHECK_EQ(normalizeMenuToken(""), "");
}

TEST_CASE("text/containsWildcard") {
    CHECK(containsWildcard("claude*"));
    CHECK(containsWildcard("a?b"));
    CHECK_FALSE(containsWildcard("plain.txt"));
    CHECK_FALSE(containsWildcard(""));
}

TEST_CASE("text/wildcardMatch star and question mark") {
    CHECK(wildcardMatch("claude*", "claude", false));
    CHECK(wildcardMatch("claude*", "claude.exe", false));
    CHECK(wildcardMatch("*", "anything", false));
    CHECK(wildcardMatch("*", "", false));
    CHECK(wildcardMatch("c?aude", "claude", false));
    CHECK(wildcardMatch("*.exe", "a.b.exe", false));
    CHECK(wildcardMatch("a*b*c", "aXXbYYc", false));
    CHECK_FALSE(wildcardMatch("claude*", "xclaude", false));
    CHECK_FALSE(wildcardMatch("c?aude", "caude", false));
    CHECK_FALSE(wildcardMatch("a*b", "abc", false));
    CHECK_FALSE(wildcardMatch("", "x", false));
    CHECK(wildcardMatch("", "", false));
}

TEST_CASE("text/wildcardMatch case sensitivity") {
    CHECK_FALSE(wildcardMatch("Claude*", "claude.exe", false));
    CHECK(wildcardMatch("Claude*", "claude.exe", true));
    CHECK(wildcardMatch("CLAUDE?EXE", "claude.exe", true));
}

TEST_CASE("text/wildcardMatch backtracking stays fast") {
    const std::string value(5000, 'a');
    const std::string pattern = std::string(50, '*') + "b";
    CHECK_FALSE(wildcardMatch(pattern, value, false));
    CHECK(wildcardMatch(std::string(50, '*') + "a", value, false));
}

TEST_CASE("text/quoteShellArgument escapes platform quotes") {
#ifdef _WIN32
    CHECK_EQ(quoteShellArgument("C:/a b/claude.exe"), "\"C:/a b/claude.exe\"");
    CHECK_EQ(quoteShellArgument("say \"hi\""), "\"say \"\"hi\"\"\"");
#else
    CHECK_EQ(quoteShellArgument("/a b/claude"), "'/a b/claude'");
    CHECK_EQ(quoteShellArgument("it's"), "'it'\\''s'");
#endif
    CHECK_EQ(quoteShellArgument("").size(), static_cast<std::size_t>(2));
}
