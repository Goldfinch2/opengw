#include "debuglog.h"

static FILE* g_logFile = NULL;

void debuglog::init()
{
    if (!g_logFile)
        g_logFile = fopen("opengw.log", "w");
}

void debuglog::log(bool enabled, const char* fmt, ...)
{
    if (!enabled || !g_logFile) return;

    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);
    fflush(g_logFile);
}

void debuglog::close()
{
    if (g_logFile)
    {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}
