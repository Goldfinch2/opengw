#pragma once

#include <string>

// The game's settings: game.ini next to the executable (the package's
// manifest), "key = value" lines after the package's; a section overrides
// them for the output: [sdl] in a window, [dvg] on the USB-DVG, [beamforge]
// on BeamForge (its missing keys taken from [dvg]). The frame rate on a
// board is its section's frame_hz, read by the SDK.
class settings
{
  public:
    settings();
    ~settings() = default;

    // The output, before the first get().
    enum Output { WINDOW, DVG, BEAMFORGE };
    static void setOutput(Output output);

    const static settings& get();

    bool mCreditsPerGame;

    bool mGridSmoothing;
    bool mParticleSmoothing;
    bool mEnemySmoothing;
    bool mPlayerSmoothing;
    bool mStarSmoothing;

    int mAttractors;
    int mParticles;
    int mGridPasses;
    int gridSpacing;

    bool sortVectors;

    // sounds/<file>, through the SDK's asset path
    std::string soundFile(const char* filename) const;
    // the package's save folder
    std::string saveFile(const char* filename) const;

  private:
    void load();
};
