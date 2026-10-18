#pragma once

#include <vectorpie/vp.h>

#include <cstddef>

// The game's sounds: numbered tracks, each played on its own voice (playing a
// track again restarts it), through the VectorPie SDK's audio.
class sound
{
  public:
    sound();
    ~sound();

    static void setAudio(vp_audio* audio);     // NULL: silent

    void loadTrack(const char* file, std::size_t track, float volume, bool loop = false);
    void playTrack(std::size_t track);
    void stopTrack(std::size_t track);
    void stopAllTracks();
    void stopAllTracksBut(std::size_t track);
    int playTrackGroup(std::size_t trackFirst, std::size_t trackLast);
    void pauseTrack(std::size_t track);
    void unpauseTrack(std::size_t track);
    void pauseAllTracks();
    void unpauseAllTracks();
    void pauseAllTracksBut(std::size_t track);
    bool isTrackPlaying(std::size_t track);

    void startSound();
    void stopSound();

    void setTrackSpeed(std::size_t track, double speed);
};
