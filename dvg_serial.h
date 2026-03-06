#ifndef DVG_SERIAL_H
#define DVG_SERIAL_H

// DVG serial port abstraction
// The DVG hardware appears as a CDC serial port and accepts 4-byte commands.
// Platform-specific implementations in dvg_serial_linux.cpp.

namespace dvg_serial
{
    bool open(const char* portName);
    void close();
    bool isOpen();
    int  write(const void* data, int count);
    int  read(void* buffer, int count);
    void flush();
}

#endif // DVG_SERIAL_H
