#pragma once

#include "inputmap.hpp"
#include "point3d.hpp"

#include <SDL2/SDL.h>

#include <memory>

class controls
{
  public:
    controls();
    ~controls();

    Point3d getLeftStick(int player);
    Point3d getRightStick(int player);
    bool getTriggerButton(int player);
    bool getStartButton(int player);
    bool getBackButton(int player);
    bool getPauseButton(int player);
    bool getExitButton();

    // Hot-plugging support
    void handleGamepadAdded(SDL_JoystickID instanceId);
    void handleGamepadRemoved(SDL_JoystickID instanceId);
    void scanForGamepads();

  private:
    // Keyboard
    Point3d readKeyboardLeftStick(int player);
    Point3d readKeyboardRightStick(int player);
    bool readKeyboardTrigger(int player);
    bool readKeyboardStart(int player);
    bool readKeyboardBack(int player);
    bool readKeyboardPause(int player);

    // Gamepads (modern API)
    Point3d readGamepadLeftStick(int player);
    Point3d readGamepadRightStick(int player);
    bool readGamepadTrigger(int player);
    bool readGamepadStart(int player);
    bool readGamepadBack(int player);
    bool readGamepadPause(int player);

    static constexpr int MAX_GAMEPADS { 4 };

    SDL_GameController* mGamepads[MAX_GAMEPADS] {};
    int mNumGamepads { 0 };

    // advmame.rc input mapping (used when configured, replaces keyboard/gamepad)
    std::unique_ptr<InputMap> mInputMap;
};
