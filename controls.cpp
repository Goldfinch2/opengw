#include "controls.hpp"

#include <cmath>

// Below this an analog stick counts as centered.
constexpr float CLAMPVALUE = .3f;

static vp_input* s_in;

controls::controls() {}
controls::~controls() {}

void controls::setInput(vp_input* in)
{
    s_in = in;
}

static bool held(const char* action)
{
    return s_in && vp_input_held(s_in, action);
}

// x = up (+1), y = left (+1): the digital directions at full deflection, plus
// the analog stick (which reports up and left negative).
static Point3d stick(const char* up, const char* down, const char* left, const char* right,
                     const char* axisX, const char* axisY)
{
    Point3d v(0, 0, 0);
    if (held(up))    v.x += 1;
    if (held(down))  v.x -= 1;
    if (held(left))  v.y += 1;
    if (held(right)) v.y -= 1;
    if (s_in) {
        float ax = -vp_input_axis(s_in, axisY), ay = -vp_input_axis(s_in, axisX);
        if (std::fabs(ax) >= CLAMPVALUE) v.x += ax;
        if (std::fabs(ay) >= CLAMPVALUE) v.y += ay;
    }
    return v;
}

Point3d controls::getLeftStick(int /*player*/)
{
    return stick("p1_up", "p1_down", "p1_left", "p1_right", "p1_stickx", "p1_sticky");
}

Point3d controls::getRightStick(int /*player*/)
{
    return stick("p2_up", "p2_down", "p2_left", "p2_right", "p2_stickx", "p2_sticky");
}

bool controls::getTriggerButton(int /*player*/) { return held("p1_button1"); }

bool controls::getStartButton(int player)
{
    static const char* const start[4] = { "start1", "start2", "start3", "start4" };
    return player >= 0 && player < 4 && held(start[player]);
}

bool controls::getBackButton(int /*player*/)  { return held("coin1"); }
bool controls::getPauseButton(int /*player*/) { return held("ui_pause"); }
bool controls::getExitButton()                { return false; }
