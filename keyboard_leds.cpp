#ifdef __linux__

#include "keyboard_leds.h"
#include "debuglog.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

bool KeyboardLeds::testBit(unsigned bit, const uint8_t* bitmask)
{
    return (bitmask[bit / 8] & (1 << (bit % 8))) != 0;
}

bool KeyboardLeds::isKeyboard(int fd, const uint8_t* evBits)
{
    if (!testBit(EV_KEY, evBits))
        return false;

    // Check for a few common key codes to confirm it's a real keyboard
    uint8_t keyBits[(KEY_MAX + 7) / 8] {};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0)
        return false;

    return testBit(KEY_A, keyBits) && testBit(KEY_Z, keyBits);
}

unsigned KeyboardLeds::readLeds(int fd)
{
    uint8_t ledBits[(LED_MAX + 7) / 8] {};
    unsigned mask = 0;

    if (ioctl(fd, EVIOCGLED(sizeof(ledBits)), ledBits) >= 0)
    {
        if (testBit(LED_NUML, ledBits))    mask |= NUMLOCK;
        if (testBit(LED_CAPSL, ledBits))   mask |= CAPSLOCK;
        if (testBit(LED_SCROLLL, ledBits)) mask |= SCROLLLOCK;
    }
    return mask;
}

void KeyboardLeds::writeLed(int fd, int led, bool on)
{
    struct input_event ev {};
    ev.type  = EV_LED;
    ev.code  = led;
    ev.value = on ? 1 : 0;
    write(fd, &ev, sizeof(ev));
}

// ─── Public interface ────────────────────────────────────────────────────────

KeyboardLeds::~KeyboardLeds()
{
    if (mOpen)
        shutdown();
}

int KeyboardLeds::init()
{
    if (mOpen)
        return mCount;

    mCount = 0;
    char path[64];

    for (int i = 0; i < MAX_SCAN && mCount < MAX_DEVICES; i++)
    {
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        // Need write access to control LEDs
        int fd = open(path, O_RDWR | O_NONBLOCK);
        if (fd < 0)
            fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        uint8_t evBits[(EV_MAX + 7) / 8] {};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0)
        {
            close(fd);
            continue;
        }

        if (!isKeyboard(fd, evBits) || !testBit(EV_LED, evBits))
        {
            close(fd);
            continue;
        }

        mDevices[mCount].fd = fd;
        mDevices[mCount].savedLeds = readLeds(fd);
        mCount++;

        debuglog::log(true, "KeyboardLeds: opened %s (saved state 0x%x)\n",
                      path, mDevices[mCount - 1].savedLeds);
    }

    mMask = 0;
    mOpen = true;

    debuglog::log(true, "KeyboardLeds: %d device(s) with LED support\n", mCount);
    return mCount;
}

void KeyboardLeds::shutdown()
{
    if (!mOpen)
        return;

    // Restore original LED state
    for (int i = 0; i < mCount; i++)
    {
        unsigned saved = mDevices[i].savedLeds;
        writeLed(mDevices[i].fd, LED_NUML,    saved & NUMLOCK);
        writeLed(mDevices[i].fd, LED_CAPSL,   saved & CAPSLOCK);
        writeLed(mDevices[i].fd, LED_SCROLLL, saved & SCROLLLOCK);
        close(mDevices[i].fd);
    }

    debuglog::log(true, "KeyboardLeds: shutdown, restored %d device(s)\n", mCount);

    mCount = 0;
    mMask  = 0;
    mOpen  = false;
}

void KeyboardLeds::set(unsigned mask)
{
    mMask = mask;

    for (int i = 0; i < mCount; i++)
    {
        writeLed(mDevices[i].fd, LED_NUML,    mask & NUMLOCK);
        writeLed(mDevices[i].fd, LED_CAPSL,   mask & CAPSLOCK);
        writeLed(mDevices[i].fd, LED_SCROLLL, mask & SCROLLLOCK);
    }
}

#endif // __linux__
