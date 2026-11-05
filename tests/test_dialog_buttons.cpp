#include "test_framework.hpp"

#include "dialog_buttons.hpp"

#include <ftxui/component/event.hpp>

using namespace claudex;

namespace {

ftxui::Mouse makeMouse(ftxui::Mouse::Button button,
                       ftxui::Mouse::Motion motion, int x, int y) {
    ftxui::Mouse mouse;
    mouse.button = button;
    mouse.motion = motion;
    mouse.x = x;
    mouse.y = y;
    return mouse;
}

ftxui::Event mouseEvent(ftxui::Mouse::Button button,
                        ftxui::Mouse::Motion motion, int x, int y) {
    return ftxui::Event::Mouse(
        "", makeMouse(button, motion, x, y));
}

}  // namespace

TEST_CASE("dialog_buttons/mouseRowAt finds the containing row") {
    std::vector<ftxui::Box> boxes(3, kEmptyHitBox);
    boxes[1] = ftxui::Box{0, 10, 2, 2};

    CHECK_EQ(mouseRowAt(boxes, makeMouse(ftxui::Mouse::None,
                                         ftxui::Mouse::Moved, 5, 2)),
             1);
    CHECK_EQ(mouseRowAt(boxes, makeMouse(ftxui::Mouse::None,
                                         ftxui::Mouse::Moved, 5, 3)),
             -1);
    CHECK_EQ(mouseRowAt({}, makeMouse(ftxui::Mouse::None,
                                      ftxui::Mouse::Moved, 0, 0)),
             -1);
}

TEST_CASE("dialog_buttons/mouseInside ignores empty boxes") {
    const ftxui::Mouse mouse =
        makeMouse(ftxui::Mouse::None, ftxui::Mouse::Moved, 0, 0);
    CHECK_FALSE(mouseInside(kEmptyHitBox, mouse));
    CHECK(mouseInside(ftxui::Box{0, 0, 0, 0}, mouse));
}

TEST_CASE("dialog_buttons/hover tracks the row under the pointer") {
    DialogButtonBar bar;
    bar.set({"One", "Two"}, nullptr);
    bar.boxes[0] = ftxui::Box{0, 4, 0, 0};
    bar.boxes[1] = ftxui::Box{6, 10, 0, 0};

    ftxui::Event hover = mouseEvent(ftxui::Mouse::None,
                                    ftxui::Mouse::Moved, 7, 0);
    CHECK(bar.handle(hover));
    CHECK_EQ(bar.hovered, 1);
    // Moving inside the same row does not redraw.
    CHECK_FALSE(bar.handle(hover));

    ftxui::Event leave = mouseEvent(ftxui::Mouse::None,
                                    ftxui::Mouse::Moved, 5, 0);
    CHECK(bar.handle(leave));
    CHECK_EQ(bar.hovered, -1);
}

TEST_CASE("dialog_buttons/left click runs the matching callback") {
    DialogButtonBar bar;
    int clicked = -1;
    bar.set({"One", "Two"}, [&](int index) { clicked = index; });
    bar.boxes[0] = ftxui::Box{0, 4, 0, 0};
    bar.boxes[1] = ftxui::Box{6, 10, 0, 0};

    ftxui::Event press =
        mouseEvent(ftxui::Mouse::Left, ftxui::Mouse::Pressed, 1, 0);
    CHECK(bar.handle(press));
    CHECK_EQ(clicked, 0);

    clicked = -1;
    ftxui::Event miss =
        mouseEvent(ftxui::Mouse::Left, ftxui::Mouse::Pressed, 5, 0);
    CHECK_FALSE(bar.handle(miss));
    CHECK_EQ(clicked, -1);
}

TEST_CASE("dialog_buttons/ignores keyboard events") {
    DialogButtonBar bar;
    bar.set({"One"}, nullptr);
    bar.boxes[0] = ftxui::Box{0, 4, 0, 0};

    ftxui::Event key = ftxui::Event::Return;
    CHECK_FALSE(bar.handle(key));
}
