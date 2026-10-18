#pragma once

#include <chrono>
#include <cstdint>
#include <string>

class profiler
{
  public:
    profiler(const std::string& name);
    ~profiler();

    void start();
    void stop();

  private:
    using clock = std::chrono::steady_clock;

    std::string name;
    clock::duration total {};
    clock::time_point started {};
    uint64_t runCounter = 0;
    clock::duration longest {};
};
