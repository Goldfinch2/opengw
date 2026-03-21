#include "inputmap.hpp"
#include "advmame_rc.h"
#include "debuglog.h"

#include <unordered_map>

// ---------------------------------------------------------------------------
// Default bindings matching MAME auto defaults for twin-stick (robotron):
//   p1 (move) = arrows + joystick0 stick0
//   p2 (aim)  = R/F/D/G + joystick1 stick0
// ---------------------------------------------------------------------------
void InputMap::setDefaults() {
    auto addKey = [this](GameAction action, SDL_Scancode sc) {
        InputBinding b;
        b.type = InputBinding::KEYBOARD;
        b.device = 0;
        b.value = sc;
        m_bindings[static_cast<int>(action)].push_back(b);
    };

    auto addJoyAxis = [this](GameAction action, int device, int axis, InputBinding::Type type) {
        InputBinding b;
        b.type = type;
        b.device = device;
        b.value = axis;
        m_bindings[static_cast<int>(action)].push_back(b);
    };

    // Movement: arrows + joystick0 stick0
    addKey(GameAction::MOVE_UP,    SDL_SCANCODE_UP);
    addKey(GameAction::MOVE_DOWN,  SDL_SCANCODE_DOWN);
    addKey(GameAction::MOVE_LEFT,  SDL_SCANCODE_LEFT);
    addKey(GameAction::MOVE_RIGHT, SDL_SCANCODE_RIGHT);
    addJoyAxis(GameAction::MOVE_UP,    0, 1, InputBinding::JOY_AXIS_NEG);
    addJoyAxis(GameAction::MOVE_DOWN,  0, 1, InputBinding::JOY_AXIS_POS);
    addJoyAxis(GameAction::MOVE_LEFT,  0, 0, InputBinding::JOY_AXIS_NEG);
    addJoyAxis(GameAction::MOVE_RIGHT, 0, 0, InputBinding::JOY_AXIS_POS);

    // Aiming: R/F/D/G + joystick1 stick0
    addKey(GameAction::AIM_UP,     SDL_SCANCODE_R);
    addKey(GameAction::AIM_DOWN,   SDL_SCANCODE_F);
    addKey(GameAction::AIM_LEFT,   SDL_SCANCODE_D);
    addKey(GameAction::AIM_RIGHT,  SDL_SCANCODE_G);
    addJoyAxis(GameAction::AIM_UP,    1, 1, InputBinding::JOY_AXIS_NEG);
    addJoyAxis(GameAction::AIM_DOWN,  1, 1, InputBinding::JOY_AXIS_POS);
    addJoyAxis(GameAction::AIM_LEFT,  1, 0, InputBinding::JOY_AXIS_NEG);
    addJoyAxis(GameAction::AIM_RIGHT, 1, 0, InputBinding::JOY_AXIS_POS);

    // Fire: left control + joystick0 button0
    addKey(GameAction::FIRE, SDL_SCANCODE_LCTRL);
    {
        InputBinding b;
        b.type = InputBinding::JOY_BUTTON;
        b.device = 0;
        b.value = 0;
        m_bindings[static_cast<int>(GameAction::FIRE)].push_back(b);
    }

    addKey(GameAction::START_P1,   SDL_SCANCODE_1);
    addKey(GameAction::START_P2,   SDL_SCANCODE_2);
    addKey(GameAction::START_P3,   SDL_SCANCODE_3);
    addKey(GameAction::START_P4,   SDL_SCANCODE_4);

    addKey(GameAction::BACK,       SDL_SCANCODE_5);
    addKey(GameAction::PAUSE,      SDL_SCANCODE_P);
    addKey(GameAction::EXIT,       SDL_SCANCODE_ESCAPE);
}

// Convert AdvInputBinding to our InputBinding
static InputBinding convertBinding(const AdvInputBinding& ab) {
    InputBinding b;
    switch (ab.type) {
        case AdvInputBinding::KEYBOARD:     b.type = InputBinding::KEYBOARD;     break;
        case AdvInputBinding::JOY_BUTTON:   b.type = InputBinding::JOY_BUTTON;   break;
        case AdvInputBinding::JOY_AXIS_POS: b.type = InputBinding::JOY_AXIS_POS; break;
        case AdvInputBinding::JOY_AXIS_NEG: b.type = InputBinding::JOY_AXIS_NEG; break;
        case AdvInputBinding::MOUSE_BUTTON: b.type = InputBinding::MOUSE_BUTTON; break;
        default:                            b.type = InputBinding::NONE;         break;
    }
    b.device = ab.device;
    b.value  = ab.value;
    return b;
}

bool InputMap::load(const char* advmameRcPath) {
    setDefaults();

    if (!advmameRcPath || !advmameRcPath[0]) {
        debuglog::log(true, "InputMap: no advmame.rc configured, using default bindings\n");
        return false;
    }

    AdvmameRC rc;
    if (!advmameLoad(advmameRcPath, rc)) {
        debuglog::log(true, "InputMap: cannot open '%s', using default bindings\n", advmameRcPath);
        return false;
    }

    static const struct { const char* rcName; GameAction action; } actionMap[] = {
        {"p1_up",        GameAction::MOVE_UP},
        {"p1_down",      GameAction::MOVE_DOWN},
        {"p1_left",      GameAction::MOVE_LEFT},
        {"p1_right",     GameAction::MOVE_RIGHT},
        {"p2_up",        GameAction::AIM_UP},
        {"p2_down",      GameAction::AIM_DOWN},
        {"p2_left",      GameAction::AIM_LEFT},
        {"p2_right",     GameAction::AIM_RIGHT},
        {"p1_button1",   GameAction::FIRE},
        {"start1",       GameAction::START_P1},
        {"start2",       GameAction::START_P2},
        {"start3",       GameAction::START_P3},
        {"start4",       GameAction::START_P4},
        {"coin1",        GameAction::BACK},
        {"ui_pause",     GameAction::PAUSE},
        {"ui_cancel",    GameAction::EXIT},
    };

    for (const auto& am : actionMap) {
        auto it = rc.inputActions.find(am.rcName);
        if (it == rc.inputActions.end()) continue;

        int idx = static_cast<int>(am.action);
        m_bindings[idx].clear();
        for (const auto& ab : it->second.bindings)
            m_bindings[idx].push_back(convertBinding(ab));
    }

    debuglog::log(true, "InputMap: loaded bindings from '%s'\n", advmameRcPath);
    return true;
}

// ---------------------------------------------------------------------------
// Joystick management
// ---------------------------------------------------------------------------
void InputMap::openJoysticks() {
    int count = SDL_NumJoysticks();

    for (int i = 0; i < count && m_numJoysticks < MAX_JOYSTICKS; i++) {
        m_joysticks[m_numJoysticks] = SDL_JoystickOpen(i);
        if (m_joysticks[m_numJoysticks]) {
            debuglog::log(true, "InputMap: opened joystick %d: %s\n",
                          m_numJoysticks,
                          SDL_JoystickName(m_joysticks[m_numJoysticks]));
            m_numJoysticks++;
        }
    }

    debuglog::log(true, "InputMap: %d joystick(s) opened\n", m_numJoysticks);
}

void InputMap::closeJoysticks() {
    for (int i = 0; i < m_numJoysticks; i++) {
        if (m_joysticks[i]) {
            SDL_JoystickClose(m_joysticks[i]);
            m_joysticks[i] = nullptr;
        }
    }
    m_numJoysticks = 0;
}

InputMap::~InputMap() {
    closeJoysticks();
}

// ---------------------------------------------------------------------------
// State-based query: is the given action currently active?
// ---------------------------------------------------------------------------
static constexpr int JOY_DEADZONE = 16000;

bool InputMap::isActive(GameAction action) const {
    int idx = static_cast<int>(action);
    if (idx < 0 || idx >= ACTION_COUNT) return false;

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    for (const auto& b : m_bindings[idx]) {
        switch (b.type) {
        case InputBinding::KEYBOARD:
            if (keys && keys[b.value])
                return true;
            break;

        case InputBinding::JOY_BUTTON:
            if (b.device < m_numJoysticks && m_joysticks[b.device]) {
                if (SDL_JoystickGetButton(m_joysticks[b.device], b.value))
                    return true;
            }
            break;

        case InputBinding::JOY_AXIS_POS:
            if (b.device < m_numJoysticks && m_joysticks[b.device]) {
                Sint16 val = SDL_JoystickGetAxis(m_joysticks[b.device], b.value);
                if (val > JOY_DEADZONE)
                    return true;
            }
            break;

        case InputBinding::JOY_AXIS_NEG:
            if (b.device < m_numJoysticks && m_joysticks[b.device]) {
                Sint16 val = SDL_JoystickGetAxis(m_joysticks[b.device], b.value);
                if (val < -JOY_DEADZONE)
                    return true;
            }
            break;

        case InputBinding::MOUSE_BUTTON: {
            int mx, my;
            Uint32 state = SDL_GetMouseState(&mx, &my);
            if (state & SDL_BUTTON(b.value))
                return true;
            break;
        }

        default:
            break;
        }
    }

    return false;
}
