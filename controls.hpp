#pragma once

#include "point3d.hpp"

#include <vectorpie/vp.h>

// The cabinet's controls through the VectorPie SDK (advmame.rc actions, the
// ones the game's own input map used): move on the p1 directions and the p1
// stick, aim on the p2 directions and the p2 stick, fire p1_button1, start1
// to start4, coin1 (credit), ui_pause. ui_cancel (back to the menu) is the
// SDK's own.
class controls
{
  public:
    controls();
    ~controls();

    // the SDK's input, read before each game step (OpenGW.cpp)
    static void setInput(vp_input* in);

    Point3d getLeftStick(int player);
    Point3d getRightStick(int player);
    bool getTriggerButton(int player);
    bool getStartButton(int player);
    bool getBackButton(int player);
    bool getPauseButton(int player);
    bool getExitButton();       // the SDK handles ui_cancel: always false
};
