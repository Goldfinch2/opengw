#ifdef USE_DVG_RENDERER
#ifndef _WIN32

#include "dvg_serial.h"
#include "debuglog.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

static int g_serialFd = -1;

namespace dvg_serial
{
    bool open(const char* portName)
    {
        if (g_serialFd >= 0)
            close();

        g_serialFd = ::open(portName, O_RDWR | O_NOCTTY);
        if (g_serialFd < 0)
        {
            debuglog::log(true,"dvg_serial: failed to open %s (%s)\n", portName, strerror(errno));
            return false;
        }

        struct termios tios;
        memset(&tios, 0, sizeof(tios));
        if (tcgetattr(g_serialFd, &tios) < 0)
        {
            debuglog::log(true,"dvg_serial: tcgetattr failed (%s)\n", strerror(errno));
            ::close(g_serialFd);
            g_serialFd = -1;
            return false;
        }

        cfmakeraw(&tios);
        cfsetispeed(&tios, B2000000);
        cfsetospeed(&tios, B2000000);

        tios.c_cflag |= (CLOCAL | CREAD);
        tios.c_cflag &= ~CRTSCTS;

        if (tcsetattr(g_serialFd, TCSANOW, &tios) < 0)
        {
            debuglog::log(true,"dvg_serial: tcsetattr failed (%s)\n", strerror(errno));
            ::close(g_serialFd);
            g_serialFd = -1;
            return false;
        }

        debuglog::log(true,"dvg_serial: opened %s\n", portName);
        return true;
    }

    void close()
    {
        if (g_serialFd >= 0)
        {
            tcdrain(g_serialFd);
            ::close(g_serialFd);
            g_serialFd = -1;
            debuglog::log(true,"dvg_serial: closed\n");
        }
    }

    bool isOpen()
    {
        return g_serialFd >= 0;
    }

    int write(const void* data, int count)
    {
        if (g_serialFd < 0)
            return -1;

        ssize_t result = ::write(g_serialFd, data, count);
        if (result < 0)
            return -1;

        return (int)result;
    }

    int read(void* buffer, int count)
    {
        if (g_serialFd < 0)
            return -1;

        ssize_t result = ::read(g_serialFd, buffer, count);
        if (result < 0)
            return -1;

        return (int)result;
    }

    void flush()
    {
        if (g_serialFd >= 0)
            tcdrain(g_serialFd);
    }
}

#endif // !_WIN32
#endif // USE_DVG_RENDERER
