#include "profiler.hpp"
#include "debuglog.h"

#include <cinttypes>

profiler::profiler(const std::string& name) : name(name)
{
}

profiler::~profiler()
{
    const double duration = std::chrono::duration<double>(total).count();
    const double average = runCounter ? 1000.0 * duration / static_cast<double>(runCounter) : 0.0;
    const long long longestMillis = std::chrono::duration_cast<std::chrono::milliseconds>(longest).count();
    debuglog::log(true,"[%s] Duration %f s, run counter %" PRIu64 ", average %f ms, longest %lld ms\n",
           name.c_str(), duration, runCounter, average, longestMillis);
}

void profiler::start()
{
    started = clock::now();
}

void profiler::stop()
{
    const auto duration = clock::now() - started;
    total += duration;
    runCounter++;
    if (duration > longest) {
        longest = duration;
    }
}
