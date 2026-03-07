#pragma once

#include <string>

class settings
{
  public:
    settings();
    ~settings() = default;

    const static settings& get();

    bool mCreditsPerGame;

    bool mEnableGlow;

    bool mGridSmoothing;
    bool mParticleSmoothing;
    bool mEnemySmoothing;
    bool mPlayerSmoothing;
    bool mStarSmoothing;

    int mAttractors;
    int mParticles;
    int mGridPasses;
    int gridSpacing;

    int targetFps;

    char serialPort[128];
    bool sortVectors;
    int dvgRefreshRate;

    char marquee[256];
    char advmamerc[256];
    char sounds[256];
    char save[256];

    std::string marqueeFile() const;
    std::string soundFile(const char* filename) const;
    std::string saveFile(const char* filename) const;

  private:
    void load();
};
