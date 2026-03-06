#include "controls.hpp"

#include <SDL3/SDL_joystick.h>

#include <cstdio>

extern const bool* keyboardState;

// CONTROLLER VALUES
constexpr float AXIS_MAX = 32768.f;
constexpr float CLAMPVALUE = .3f;

controls::controls()
{
    printf("Initing controls\n");

    // Scan for initially connected gamepads
    scanForGamepads();
}

controls::~controls()
{
    for (auto& pad: mGamepads) {
        if (pad) {
            SDL_CloseGamepad(pad);
            pad = nullptr;
        }
    }
}

void controls::scanForGamepads()
{
    int count = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);

    if (!joysticks) {
        printf("SDL_GetJoysticks() failed: %s\n", SDL_GetError());
    }

    printf("Scanning for gamepads... Found %d joysticks\n", count);

    // Open any available gamepads
    for (int j = 0; j < count && mNumGamepads < MAX_GAMEPADS; j++) {
        if (SDL_IsGamepad(j)) {
            // Check if this device is already opened
            bool alreadyOpen = false;

            for (auto& pad: mGamepads) {
                if (pad) {
                    SDL_Joystick* joy = SDL_GetGamepadJoystick(pad);
                    if (SDL_GetJoystickID(joy) == joysticks[j]) {
                        alreadyOpen = true;
                        break;
                    }
                }
            }

            if (!alreadyOpen) {
                // Find first free slot
                for (int i = 0; i < MAX_GAMEPADS; i++) {
                    if (!mGamepads[i]) {
                        mGamepads[i] = SDL_OpenGamepad(j);
                        if (mGamepads[i]) {
                            printf("Gamepad %d: %s\n", i,
                                   SDL_GetGamepadName(mGamepads[i]));
                            mNumGamepads++;
                        }
                        break;
                    }
                }
            }
        }
    }

    SDL_free(joysticks);

    printf("Total gamepads opened: %d\n", mNumGamepads);
}

void controls::handleGamepadAdded(SDL_JoystickID instanceId)
{
    if (!SDL_IsGamepad(instanceId))
        return;

    // Find first available slot
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (!mGamepads[i]) {
            mGamepads[i] = SDL_OpenGamepad(instanceId);
            if (mGamepads[i]) {
                printf("Gamepad connected in slot %d: %s\n", i,
                       SDL_GetGamepadName(mGamepads[i]));
                mNumGamepads++;
            } else {
                printf("Failed to open gamepad %d: %s\n", instanceId, SDL_GetError());
            }
            break;
        }
    }
}

void controls::handleGamepadRemoved(SDL_JoystickID instanceId)
{
    // Find and remove the disconnected gamepad
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (mGamepads[i]) {
            SDL_Joystick* joy = SDL_GetGamepadJoystick(mGamepads[i]);
            if (SDL_GetJoystickID(joy) == instanceId) {
                printf("Gamepad disconnected from slot %d\n", i);
                SDL_CloseGamepad(mGamepads[i]);
                mGamepads[i] = nullptr;
                mNumGamepads--;
                break;
            }
        }
    }
}

Point3d controls::getLeftStick(int player)
{
    return readKeyboardLeftStick(player) + readGamepadLeftStick(player);
}

Point3d controls::getRightStick(int player)
{
    return readKeyboardRightStick(player) + readGamepadRightStick(player);
}

bool controls::getTriggerButton(int player)
{
    return readKeyboardTrigger(player) || readGamepadTrigger(player);
}

bool controls::getStartButton(int player)
{
    return readKeyboardStart(player) || readGamepadStart(player);
}

bool controls::getBackButton(int player)
{
    return readKeyboardBack(player) || readGamepadBack(player);
}

bool controls::getPauseButton(int player)
{
    return readKeyboardPause(player) || readGamepadPause(player);
}

//
// Keyboard controller
//
Point3d controls::readKeyboardLeftStick(int /*player*/)
{
    bool up = keyboardState[SDL_SCANCODE_W];
    bool down = keyboardState[SDL_SCANCODE_S];
    bool left = keyboardState[SDL_SCANCODE_A];
    bool right = keyboardState[SDL_SCANCODE_D];

    int x = 0;
    int y = 0;

    x += up ? 1 : 0;
    x -= down ? 1 : 0;

    y += left ? 1 : 0;
    y -= right ? 1 : 0;

    return Point3d(x, y, 0);
}

Point3d controls::readKeyboardRightStick(int /*player*/)
{
    bool up = keyboardState[SDL_SCANCODE_UP];
    bool down = keyboardState[SDL_SCANCODE_DOWN];
    bool left = keyboardState[SDL_SCANCODE_LEFT];
    bool right = keyboardState[SDL_SCANCODE_RIGHT];

    int x = 0;
    int y = 0;

    x += up ? 1 : 0;
    x -= down ? 1 : 0;

    y += left ? 1 : 0;
    y -= right ? 1 : 0;

    return Point3d(x, y, 0);
}

bool controls::readKeyboardTrigger(int /*player*/)
{
    return keyboardState[SDL_SCANCODE_SPACE];
}

bool controls::readKeyboardStart(int player)
{
    switch (player) {
    case 0:
        return keyboardState[SDL_SCANCODE_1];
    case 1:
        return keyboardState[SDL_SCANCODE_2];
    case 2:
        return keyboardState[SDL_SCANCODE_3];
    case 3:
        return keyboardState[SDL_SCANCODE_4];
    }
    return false;
}

bool controls::readKeyboardBack(int /*player*/)
{
    return keyboardState[SDL_SCANCODE_BACKSPACE];
}

bool controls::readKeyboardPause(int /*player*/)
{
    return keyboardState[SDL_SCANCODE_P];
}

//
// Modern SDL GameController API
//

Point3d controls::readGamepadLeftStick(int player)
{
    if (!mGamepads[player])
        return Point3d(0, 0, 0);

    Point3d vector;
    // Note: Y axis is inverted for standard game controls
    vector.x = -(SDL_GetGamepadAxis(mGamepads[player], SDL_GAMEPAD_AXIS_LEFTY)) / AXIS_MAX;
    vector.y = -(SDL_GetGamepadAxis(mGamepads[player], SDL_GAMEPAD_AXIS_LEFTX)) / AXIS_MAX;
    vector.z = 0;

    // Apply deadzone
    if (fabs(vector.x) < CLAMPVALUE) {
        vector.x = 0;
    }
    if (fabs(vector.y) < CLAMPVALUE) {
        vector.y = 0;
    }

    return vector;
}

Point3d controls::readGamepadRightStick(int player)
{
    if (!mGamepads[player])
        return Point3d(0, 0, 0);

    Point3d vector;
    vector.x = -(SDL_GetGamepadAxis(mGamepads[player], SDL_GAMEPAD_AXIS_RIGHTY)) / AXIS_MAX;
    vector.y = -(SDL_GetGamepadAxis(mGamepads[player], SDL_GAMEPAD_AXIS_RIGHTX)) / AXIS_MAX;
    vector.z = 0;

    // Apply deadzone
    if (fabs(vector.x) < CLAMPVALUE) {
        vector.x = 0;
    }
    if (fabs(vector.y) < CLAMPVALUE) {
        vector.y = 0;
    }

    return vector;
}

bool controls::readGamepadTrigger(int player)
{
    if (!mGamepads[player])
        return false;

    // Right trigger
    float triggerVal = (SDL_GetGamepadAxis(mGamepads[player], SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) / AXIS_MAX;

    if (triggerVal > CLAMPVALUE) {
        return true;
    }

    return false;
}

bool controls::readGamepadStart(int player)
{
    if (!mGamepads[player])
        return false;

    return SDL_GetGamepadButton(mGamepads[player], SDL_GAMEPAD_BUTTON_SOUTH);
}

bool controls::readGamepadBack(int player)
{
    if (!mGamepads[player])
        return false;

    return SDL_GetGamepadButton(mGamepads[player], SDL_GAMEPAD_BUTTON_EAST);
}

bool controls::readGamepadPause(int player)
{
    if (!mGamepads[player])
        return false;

    return SDL_GetGamepadButton(mGamepads[player], SDL_GAMEPAD_BUTTON_START);
}
