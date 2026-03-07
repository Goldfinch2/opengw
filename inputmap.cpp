#include "inputmap.hpp"
#include "debuglog.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// ---------------------------------------------------------------------------
// advmame key name → SDL scancode
// ---------------------------------------------------------------------------
SDL_Scancode InputMap::advmameKeyToSDL(const std::string& keyName) {
    static const std::unordered_map<std::string, SDL_Scancode> map = {
        {"up",        SDL_SCANCODE_UP},
        {"down",      SDL_SCANCODE_DOWN},
        {"left",      SDL_SCANCODE_LEFT},
        {"right",     SDL_SCANCODE_RIGHT},
        {"space",     SDL_SCANCODE_SPACE},
        {"enter",     SDL_SCANCODE_RETURN},
        {"esc",       SDL_SCANCODE_ESCAPE},
        {"tab",       SDL_SCANCODE_TAB},
        {"lcontrol",  SDL_SCANCODE_LCTRL},
        {"rcontrol",  SDL_SCANCODE_RCTRL},
        {"lalt",      SDL_SCANCODE_LALT},
        {"ralt",      SDL_SCANCODE_RALT},
        {"lshift",    SDL_SCANCODE_LSHIFT},
        {"rshift",    SDL_SCANCODE_RSHIFT},
        {"backspace", SDL_SCANCODE_BACKSPACE},
        {"capslock",  SDL_SCANCODE_CAPSLOCK},
        {"0",         SDL_SCANCODE_0},
        {"1",         SDL_SCANCODE_1},
        {"2",         SDL_SCANCODE_2},
        {"3",         SDL_SCANCODE_3},
        {"4",         SDL_SCANCODE_4},
        {"5",         SDL_SCANCODE_5},
        {"6",         SDL_SCANCODE_6},
        {"7",         SDL_SCANCODE_7},
        {"8",         SDL_SCANCODE_8},
        {"9",         SDL_SCANCODE_9},
        {"a",         SDL_SCANCODE_A},
        {"b",         SDL_SCANCODE_B},
        {"c",         SDL_SCANCODE_C},
        {"d",         SDL_SCANCODE_D},
        {"e",         SDL_SCANCODE_E},
        {"f",         SDL_SCANCODE_F},
        {"g",         SDL_SCANCODE_G},
        {"h",         SDL_SCANCODE_H},
        {"i",         SDL_SCANCODE_I},
        {"j",         SDL_SCANCODE_J},
        {"k",         SDL_SCANCODE_K},
        {"l",         SDL_SCANCODE_L},
        {"m",         SDL_SCANCODE_M},
        {"n",         SDL_SCANCODE_N},
        {"o",         SDL_SCANCODE_O},
        {"p",         SDL_SCANCODE_P},
        {"q",         SDL_SCANCODE_Q},
        {"r",         SDL_SCANCODE_R},
        {"s",         SDL_SCANCODE_S},
        {"t",         SDL_SCANCODE_T},
        {"u",         SDL_SCANCODE_U},
        {"v",         SDL_SCANCODE_V},
        {"w",         SDL_SCANCODE_W},
        {"x",         SDL_SCANCODE_X},
        {"y",         SDL_SCANCODE_Y},
        {"z",         SDL_SCANCODE_Z},
        {"f1",        SDL_SCANCODE_F1},
        {"f2",        SDL_SCANCODE_F2},
        {"f3",        SDL_SCANCODE_F3},
        {"f4",        SDL_SCANCODE_F4},
        {"f5",        SDL_SCANCODE_F5},
        {"f6",        SDL_SCANCODE_F6},
        {"f7",        SDL_SCANCODE_F7},
        {"f8",        SDL_SCANCODE_F8},
        {"f9",        SDL_SCANCODE_F9},
        {"f10",       SDL_SCANCODE_F10},
        {"f11",       SDL_SCANCODE_F11},
        {"f12",       SDL_SCANCODE_F12},
        {"insert",    SDL_SCANCODE_INSERT},
        {"home",      SDL_SCANCODE_HOME},
        {"end",       SDL_SCANCODE_END},
        {"pgup",      SDL_SCANCODE_PAGEUP},
        {"pgdn",      SDL_SCANCODE_PAGEDOWN},
        {"del",       SDL_SCANCODE_DELETE},
        {"grave",     SDL_SCANCODE_GRAVE},
        {"minus",     SDL_SCANCODE_MINUS},
        {"equals",    SDL_SCANCODE_EQUALS},
        {"openbrace", SDL_SCANCODE_LEFTBRACKET},
        {"closebrace",SDL_SCANCODE_RIGHTBRACKET},
        {"semicolon", SDL_SCANCODE_SEMICOLON},
        {"quote",     SDL_SCANCODE_APOSTROPHE},
        {"backslash", SDL_SCANCODE_BACKSLASH},
        {"comma",     SDL_SCANCODE_COMMA},
        {"period",    SDL_SCANCODE_PERIOD},
        {"slash",     SDL_SCANCODE_SLASH},
        {"numlock",   SDL_SCANCODE_NUMLOCKCLEAR},
        {"scrlock",   SDL_SCANCODE_SCROLLLOCK},
    };
    auto it = map.find(keyName);
    return (it != map.end()) ? it->second : SDL_SCANCODE_UNKNOWN;
}

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
}

// ---------------------------------------------------------------------------
// Parse one input_map[ACTION] line from advmame.rc
// ---------------------------------------------------------------------------
void InputMap::parseLine(const std::string& line) {
    size_t start = line.find('[');
    size_t end   = line.find(']');
    if (start == std::string::npos || end == std::string::npos) return;

    std::string actionName = line.substr(start + 1, end - start - 1);

    // Map advmame action names to game actions
    // Movement: p1 directions
    // Aiming:   p2 directions (second joystick on the cabinet)
    GameAction action = GameAction::COUNT;
    if      (actionName == "p1_up")      action = GameAction::MOVE_UP;
    else if (actionName == "p1_down")    action = GameAction::MOVE_DOWN;
    else if (actionName == "p1_left")    action = GameAction::MOVE_LEFT;
    else if (actionName == "p1_right")   action = GameAction::MOVE_RIGHT;
    else if (actionName == "p2_up")      action = GameAction::AIM_UP;
    else if (actionName == "p2_down")    action = GameAction::AIM_DOWN;
    else if (actionName == "p2_left")    action = GameAction::AIM_LEFT;
    else if (actionName == "p2_right")   action = GameAction::AIM_RIGHT;
    else if (actionName == "p1_button1") action = GameAction::FIRE;
    else if (actionName == "start1")     action = GameAction::START_P1;
    else if (actionName == "start2")     action = GameAction::START_P2;
    else if (actionName == "start3")     action = GameAction::START_P3;
    else if (actionName == "start4")     action = GameAction::START_P4;
    else if (actionName == "coin1")      action = GameAction::BACK;
    else if (actionName == "ui_pause")   action = GameAction::PAUSE;
    else return;

    int idx = static_cast<int>(action);

    std::string bindings = trim(line.substr(end + 1));
    if (bindings == "auto") return;

    // Clear defaults for this action since we have explicit bindings
    m_bindings[idx].clear();

    // Split on " or "
    std::vector<std::string> parts;
    size_t pos = 0;
    while (pos < bindings.size()) {
        size_t orPos = bindings.find(" or ", pos);
        if (orPos == std::string::npos) {
            parts.push_back(trim(bindings.substr(pos)));
            break;
        }
        parts.push_back(trim(bindings.substr(pos, orPos - pos)));
        pos = orPos + 4;
    }

    for (const auto& part : parts) {
        if (part.find("not") == 0) continue;

        InputBinding binding;

        if (part.find("keyboard[") == 0) {
            size_t b1 = part.find('[');
            size_t b2 = part.find(']');
            if (b1 == std::string::npos || b2 == std::string::npos) continue;
            std::string inner = part.substr(b1 + 1, b2 - b1 - 1);
            size_t comma = inner.find(',');
            if (comma == std::string::npos) continue;

            binding.type = InputBinding::KEYBOARD;
            binding.device = std::stoi(inner.substr(0, comma));
            std::string keyName = trim(inner.substr(comma + 1));

            SDL_Scancode sc = advmameKeyToSDL(keyName);
            if (sc == SDL_SCANCODE_UNKNOWN) continue;
            binding.value = sc;
        }
        else if (part.find("joystick_button[") == 0) {
            size_t b1 = part.find('[');
            size_t b2 = part.find(']');
            if (b1 == std::string::npos || b2 == std::string::npos) continue;
            std::string inner = part.substr(b1 + 1, b2 - b1 - 1);
            size_t comma = inner.find(',');
            if (comma == std::string::npos) continue;

            binding.type = InputBinding::JOY_BUTTON;
            binding.device = std::stoi(inner.substr(0, comma));
            std::string btnName = trim(inner.substr(comma + 1));

            static const std::unordered_map<std::string, int> btnMap = {
                {"trigger", 0}, {"top", 1}, {"thumb", 2}, {"thumb2", 3},
                {"top2", 4}, {"pinkie", 5}, {"base", 6}, {"base2", 7},
                {"base3", 8}, {"base4", 9}, {"base5", 10}, {"base6", 11},
                {"a", 0}, {"b", 1}, {"c", 2}, {"x", 3}, {"y", 4}, {"z", 5},
                {"tl", 6}, {"tr", 7}, {"tl2", 8}, {"tr2", 9},
                {"select", 10}, {"start", 11}, {"mode", 12},
            };
            auto it = btnMap.find(btnName);
            binding.value = (it != btnMap.end()) ? it->second : 0;
        }
        else if (part.find("joystick_digital[") == 0) {
            size_t b1 = part.find('[');
            size_t b2 = part.find(']');
            if (b1 == std::string::npos || b2 == std::string::npos) continue;
            std::string inner = part.substr(b1 + 1, b2 - b1 - 1);

            std::istringstream ss(inner);
            std::string devStr, stickStr, axisStr, dirStr;
            std::getline(ss, devStr, ',');
            std::getline(ss, stickStr, ',');
            std::getline(ss, axisStr, ',');
            std::getline(ss, dirStr, ',');

            binding.device = std::stoi(trim(devStr));
            axisStr = trim(axisStr);
            dirStr = trim(dirStr);

            // Map axis name to SDL axis index: stick*2 + (y?1:0)
            int stick = std::stoi(trim(stickStr));
            int axis = stick * 2;
            if (axisStr == "y") axis += 1;

            binding.value = axis;
            if (dirStr == "up" || dirStr == "left")
                binding.type = InputBinding::JOY_AXIS_NEG;
            else
                binding.type = InputBinding::JOY_AXIS_POS;
        }
        else if (part.find("mouse_button[") == 0) {
            size_t b1 = part.find('[');
            size_t b2 = part.find(']');
            if (b1 == std::string::npos || b2 == std::string::npos) continue;
            std::string inner = part.substr(b1 + 1, b2 - b1 - 1);
            size_t comma = inner.find(',');
            if (comma == std::string::npos) continue;

            binding.type = InputBinding::MOUSE_BUTTON;
            binding.device = std::stoi(inner.substr(0, comma));
            std::string btnName = trim(inner.substr(comma + 1));

            static const std::unordered_map<std::string, int> mbtnMap = {
                {"left", SDL_BUTTON_LEFT}, {"right", SDL_BUTTON_RIGHT},
                {"middle", SDL_BUTTON_MIDDLE}, {"side", SDL_BUTTON_X1},
                {"extra", SDL_BUTTON_X2},
            };
            auto it = mbtnMap.find(btnName);
            binding.value = (it != mbtnMap.end()) ? it->second : SDL_BUTTON_LEFT;
        }
        else {
            continue;
        }

        m_bindings[idx].push_back(binding);
    }
}

// ---------------------------------------------------------------------------
// Load advmame.rc
// ---------------------------------------------------------------------------
bool InputMap::load(const char* advmameRcPath) {
    setDefaults();

    if (!advmameRcPath || !advmameRcPath[0]) {
        debuglog::log(true, "InputMap: no advmame.rc configured, using default bindings\n");
        return false;
    }

    std::ifstream file(advmameRcPath);
    if (!file.is_open()) {
        debuglog::log(true, "InputMap: cannot open '%s', using default bindings\n", advmameRcPath);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.find("input_map[") == 0) {
            parseLine(line);
        }
    }

    debuglog::log(true, "InputMap: loaded bindings from '%s'\n", advmameRcPath);
    return true;
}

// ---------------------------------------------------------------------------
// Joystick management
// ---------------------------------------------------------------------------
void InputMap::openJoysticks() {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    if (!ids) return;

    for (int i = 0; i < count && m_numJoysticks < MAX_JOYSTICKS; i++) {
        m_joysticks[m_numJoysticks] = SDL_OpenJoystick(ids[i]);
        if (m_joysticks[m_numJoysticks]) {
            debuglog::log(true, "InputMap: opened joystick %d: %s\n",
                          m_numJoysticks,
                          SDL_GetJoystickName(m_joysticks[m_numJoysticks]));
            m_numJoysticks++;
        }
    }

    SDL_free(ids);
    debuglog::log(true, "InputMap: %d joystick(s) opened\n", m_numJoysticks);
}

void InputMap::closeJoysticks() {
    for (int i = 0; i < m_numJoysticks; i++) {
        if (m_joysticks[i]) {
            SDL_CloseJoystick(m_joysticks[i]);
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

    const bool* keys = SDL_GetKeyboardState(nullptr);

    for (const auto& b : m_bindings[idx]) {
        switch (b.type) {
        case InputBinding::KEYBOARD:
            if (keys && keys[b.value])
                return true;
            break;

        case InputBinding::JOY_BUTTON:
            if (b.device < m_numJoysticks && m_joysticks[b.device]) {
                if (SDL_GetJoystickButton(m_joysticks[b.device], b.value))
                    return true;
            }
            break;

        case InputBinding::JOY_AXIS_POS:
            if (b.device < m_numJoysticks && m_joysticks[b.device]) {
                Sint16 val = SDL_GetJoystickAxis(m_joysticks[b.device], b.value);
                if (val > JOY_DEADZONE)
                    return true;
            }
            break;

        case InputBinding::JOY_AXIS_NEG:
            if (b.device < m_numJoysticks && m_joysticks[b.device]) {
                Sint16 val = SDL_GetJoystickAxis(m_joysticks[b.device], b.value);
                if (val < -JOY_DEADZONE)
                    return true;
            }
            break;

        case InputBinding::MOUSE_BUTTON: {
            SDL_MouseButtonFlags state = SDL_GetMouseState(nullptr, nullptr);
            if (state & SDL_BUTTON_MASK(b.value))
                return true;
            break;
        }

        default:
            break;
        }
    }

    return false;
}
