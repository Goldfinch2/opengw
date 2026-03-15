#pragma once

//
// Keyboard LED control via Linux evdev (/dev/input/event*).
// Self-contained — only depends on <linux/input.h> and standard headers.
// Portable to any Linux C++ project.
//
// Usage:
//     KeyboardLeds leds;
//     leds.init();                          // enumerate keyboards, save state
//     leds.set(KeyboardLeds::NUMLOCK | KeyboardLeds::SCROLLLOCK);
//     leds.shutdown();                      // restore original LED state
//

#ifdef __linux__

#include <cstdint>

class KeyboardLeds
{
public:
    // LED mask bits (match linux/input.h LED_NUML etc.)
    static constexpr unsigned NUMLOCK    = 0x01;
    static constexpr unsigned CAPSLOCK   = 0x02;
    static constexpr unsigned SCROLLLOCK = 0x04;

    KeyboardLeds() = default;
    ~KeyboardLeds();

    // Open all keyboard devices that support LEDs. Returns device count.
    int  init();

    // Restore original LED state and close devices.
    void shutdown();

    // Set LEDs on all keyboards (OR of NUMLOCK/CAPSLOCK/SCROLLLOCK).
    void set(unsigned mask);

    // Get current mask last written by set().
    unsigned get() const { return mMask; }

    // Number of keyboards with LED support found.
    int  count() const { return mCount; }

private:
    static constexpr int MAX_DEVICES = 8;
    static constexpr int MAX_SCAN    = 32; // /dev/input/event0..31

    struct Device {
        int      fd;
        unsigned savedLeds;
    };

    Device   mDevices[MAX_DEVICES] {};
    int      mCount = 0;
    unsigned mMask  = 0;
    bool     mOpen  = false;

    static bool testBit(unsigned bit, const uint8_t* bitmask);
    static bool isKeyboard(int fd, const uint8_t* evBits);
    static unsigned readLeds(int fd);
    static void writeLed(int fd, int led, bool on);
};

#else // !__linux__

// Stub for non-Linux builds (compiles but does nothing)
class KeyboardLeds
{
public:
    static constexpr unsigned NUMLOCK    = 0x01;
    static constexpr unsigned CAPSLOCK   = 0x02;
    static constexpr unsigned SCROLLLOCK = 0x04;

    int      init()          { return 0; }
    void     shutdown()      {}
    void     set(unsigned)   {}
    unsigned get() const     { return 0; }
    int      count() const   { return 0; }
};

#endif
