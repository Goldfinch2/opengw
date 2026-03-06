#pragma once

#include <string>

#include <SDL3/SDL.h>

class profiler
{
  public:
    profiler(const std::string& name);
    ~profiler();

    void start();
    void stop();

  private:
    std::string name;
    Uint64 performanceCounter = 0;
    Uint64 started = 0;
    Uint64 runCounter = 0;
    Uint64 longest = 0;
};
