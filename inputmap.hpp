#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>

struct InputBinding {
    enum Type { NONE, KEYBOARD, JOY_BUTTON, JOY_AXIS_POS, JOY_AXIS_NEG, MOUSE_BUTTON };
    Type type   = NONE;
    int  device = 0;
    int  value  = 0;
};

enum class GameAction {
    MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT,
    AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT,
    FIRE,
    START_P1, START_P2, START_P3, START_P4,
    BACK,
    PAUSE,
    EXIT,
    COUNT
};

class InputMap {
  public:
    ~InputMap();

    bool load(const char* advmameRcPath);

    bool isActive(GameAction action) const;

    void openJoysticks();
    void closeJoysticks();

  private:
    static constexpr int ACTION_COUNT = static_cast<int>(GameAction::COUNT);
    std::vector<InputBinding> m_bindings[ACTION_COUNT];

    static constexpr int MAX_JOYSTICKS = 8;
    SDL_Joystick* m_joysticks[MAX_JOYSTICKS] {};
    int m_numJoysticks = 0;

    void setDefaults();
};
