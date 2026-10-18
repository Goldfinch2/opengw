#include "debuglog.h"

#include <vectorpie/vp.h>
#include <cstdio>

static FILE* g_logFile = NULL;

void debuglog::init()
{
    if (!g_logFile) {
        char path[1024];
        snprintf(path, sizeof path, "%s/opengw.log", vp_save_dir());   // the package's save/
        g_logFile = fopen(path, "w");
    }
}

void debuglog::log(bool enabled, const char* fmt, ...)
{
    if (!enabled) return;
    if (!g_logFile) init();
    if (!g_logFile) return;

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
