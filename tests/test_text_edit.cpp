#include "test_framework.hpp"

#include "text_edit.hpp"

using namespace claudex;

TEST_CASE("text_edit/cursorHasLine sees wrapped lines in both directions") {
    const std::string text = "first\nsecond\nthird";
    // At the start: there is a line below, none above.
    CHECK(cursorHasLine(text, 0, true));
    CHECK_FALSE(cursorHasLine(text, 0, false));
    // On the first line: still a line below.
    CHECK(cursorHasLine(text, 3, true));
    // At the last line: no line below, one above.
    CHECK_FALSE(cursorHasLine(text, text.size(), true));
    CHECK(cursorHasLine(text, text.size(), false));
    // Right after a newline, the caret is on the next line.
    CHECK(cursorHasLine(text, 6, true));
    CHECK(cursorHasLine(text, 6, false));
}

TEST_CASE("text_edit/cursorHasLine clamps an out-of-range cursor") {
    CHECK(cursorHasLine("a\nb", -5, true));
    CHECK_FALSE(cursorHasLine("a\nb", 99, true));
    CHECK(cursorHasLine("a\nb", 99, false));
}

TEST_CASE("text_edit/insertLineBreak inserts at the caret and advances it") {
    std::string text = "abcd";
    int cursor = 2;
    insertLineBreak(text, cursor);
    CHECK_EQ(text, "ab\ncd");
    CHECK_EQ(cursor, 3);
}

TEST_CASE("text_edit/insertLineBreak clamps the caret to the text") {
    std::string text = "abc";
    int cursor = 99;
    insertLineBreak(text, cursor);
    CHECK_EQ(text, "abc\n");
    CHECK_EQ(cursor, 4);

    int negative = -3;
    insertLineBreak(text, negative);
    CHECK_EQ(text, "\nabc\n");
    CHECK_EQ(negative, 1);
}
