#pragma once

#include <cstdio>
#include <cstdarg>

namespace debuglog
{
    void init();
    void log(bool enabled, const char* fmt, ...);
    void close();
};
