#include "test_framework.hpp"

#include "key_events.hpp"

using namespace claudex;
using ftxui::Event;

TEST_CASE("keys/single characters match either case") {
    CHECK(eventMatchesKeySpec(Event::Character('q'), "q"));
    CHECK(eventMatchesKeySpec(Event::Character('Q'), "q"));
    CHECK(eventMatchesKeySpec(Event::Character('q'), "Q"));
    CHECK_FALSE(eventMatchesKeySpec(Event::Character('w'), "q"));
    CHECK(eventMatchesKeySpec(Event::Character('1'), "1"));
}

TEST_CASE("keys/named keys") {
    CHECK(eventMatchesKeySpec(Event::Escape, "esc"));
    CHECK(eventMatchesKeySpec(Event::Escape, "Escape"));
    CHECK(eventMatchesKeySpec(Event::Return, "enter"));
    CHECK(eventMatchesKeySpec(Event::Return, "return"));
    CHECK(eventMatchesKeySpec(Event::Tab, "tab"));
    CHECK(eventMatchesKeySpec(Event::TabReverse, "shift-tab"));
    CHECK(eventMatchesKeySpec(Event::TabReverse, "tabReverse"));
    CHECK(eventMatchesKeySpec(Event::Character(' '), "space"));
    CHECK(eventMatchesKeySpec(Event::Backspace, "backspace"));
    CHECK(eventMatchesKeySpec(Event::Delete, "delete"));
    CHECK(eventMatchesKeySpec(Event::Delete, "del"));
    CHECK(eventMatchesKeySpec(Event::Insert, "ins"));
    CHECK(eventMatchesKeySpec(Event::Home, "home"));
    CHECK(eventMatchesKeySpec(Event::End, "end"));
    CHECK(eventMatchesKeySpec(Event::PageUp, "pgup"));
    CHECK(eventMatchesKeySpec(Event::PageDown, "PageDown"));
    CHECK(eventMatchesKeySpec(Event::ArrowUp, "up"));
    CHECK(eventMatchesKeySpec(Event::ArrowDown, "down"));
    CHECK(eventMatchesKeySpec(Event::ArrowLeft, "left"));
    CHECK(eventMatchesKeySpec(Event::ArrowRight, "right"));
}

TEST_CASE("keys/function keys") {
    CHECK(eventMatchesKeySpec(Event::F1, "f1"));
    CHECK(eventMatchesKeySpec(Event::F2, "F2"));
    CHECK(eventMatchesKeySpec(Event::F12, "f12"));
    CHECK_FALSE(eventMatchesKeySpec(Event::F2, "f3"));
    CHECK_FALSE(eventMatchesKeySpec(Event::F1, "f13"));
    CHECK_FALSE(eventMatchesKeySpec(Event::F1, "f0"));
}

TEST_CASE("keys/whitespace around specs is ignored") {
    CHECK(eventMatchesKeySpec(Event::Escape, "  esc  "));
}

TEST_CASE("keys/invalid specs never match") {
    CHECK_FALSE(eventMatchesKeySpec(Event::Escape, ""));
    CHECK_FALSE(eventMatchesKeySpec(Event::Escape, "   "));
    CHECK_FALSE(eventMatchesKeySpec(Event::Escape, "nonsense"));
    CHECK_FALSE(eventMatchesKeySpec(Event::Escape, "fx"));
    CHECK_FALSE(eventMatchesKeySpec(Event::Escape, "f"));
    CHECK_FALSE(eventMatchesKeySpec(Event::Return, "esc"));
}

TEST_CASE("keys/shift+enter sequences are recognized") {
    CHECK(isShiftEnter(Event::Special("\x1b[13;2u")));
    CHECK(isShiftEnter(Event::Special("\x1b[27;2;13~")));
    CHECK(isShiftEnter(Event::Special("\x1b\r")));
    CHECK_FALSE(isShiftEnter(Event::Return));
    CHECK_FALSE(isShiftEnter(Event::Character("\r")));
    CHECK_FALSE(isShiftEnter(Event::Special("\x1b[13;2~")));
}

TEST_CASE("keys/eventMatchesAny checks every spec") {
    const std::vector<std::string> quit = {"q", "esc"};
    CHECK(eventMatchesAny(Event::Character('q'), quit));
    CHECK(eventMatchesAny(Event::Escape, quit));
    CHECK_FALSE(eventMatchesAny(Event::Return, quit));
    CHECK_FALSE(eventMatchesAny(Event::Escape, {}));
}
