#include "sound.hpp"
#include "debuglog.h"

#include <cstdio>
#include <cstring>

constexpr std::size_t NUM_TRACKS = 200;

// The old mixer played a track at half its volume, of full scale; the SDK's
// volume is 0..1 of full scale. One number turns the whole game up or down.
constexpr float GAME_VOLUME = 0.5f;

// The SDK has no pause: a paused voice plays on silent at this pitch, all
// but frozen, and takes its volume and speed back when unpaused.
constexpr float PAUSED_PITCH = 1e-6f;

namespace {

struct Track {
    vp_sound* snd = nullptr;
    bool      loop = false;
    bool      paused = false;
    float     vol = 0;          // loadTrack's
    double    speed = 1;        // setTrackSpeed
    vp_voice  voice = 0;
};

vp_audio* s_audio;
Track     s_track[NUM_TRACKS];

Track* at(std::size_t i) { return i < NUM_TRACKS ? &s_track[i] : nullptr; }

bool playing(const Track* t) { return s_audio && t->voice && vp_voice_playing(s_audio, t->voice); }

}  // namespace

sound::sound() {}

sound::~sound()
{
    stopAllTracks();
}

void sound::setAudio(vp_audio* audio) { s_audio = audio; }

void sound::loadTrack(const char* file, std::size_t track, float volume, bool loop /*=false*/)
{
    Track* t = at(track);
    if (!t || !s_audio) return;
    t->loop = loop;
    t->vol = volume;
    char path[1024];
    if (vp_sound_load(s_audio, vp_asset_path(file, path, sizeof path), &t->snd) != VP_OK) {
        t->snd = nullptr;
        debuglog::log(true, "sound: %s\n", vp_last_error());
    }
}

void sound::startSound() {}
void sound::stopSound() { stopAllTracks(); }

void sound::playTrack(std::size_t track)
{
    Track* t = at(track);
    if (!t || !s_audio) return;
    if (t->voice) vp_voice_stop(s_audio, t->voice, 0);
    t->paused = false;
    t->voice = t->snd ? vp_sound_play(s_audio, t->snd, t->vol * GAME_VOLUME, (float)t->speed, t->loop) : 0;
}

void sound::stopTrack(std::size_t track)
{
    Track* t = at(track);
    if (!t || !s_audio) return;
    if (t->voice) vp_voice_stop(s_audio, t->voice, 0);
    t->voice = 0;
    t->paused = false;
}

void sound::stopAllTracks()
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++) stopTrack(i);
}

void sound::stopAllTracksBut(std::size_t track)
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++)
        if (i != track) stopTrack(i);
}

void sound::pauseTrack(std::size_t track)
{
    Track* t = at(track);
    if (!t || t->paused || !playing(t)) return;
    t->paused = true;
    vp_voice_set_volume(s_audio, t->voice, 0, 0);
    vp_voice_set_pitch(s_audio, t->voice, PAUSED_PITCH);
}

void sound::unpauseTrack(std::size_t track)
{
    Track* t = at(track);
    if (!t || !t->paused) return;
    t->paused = false;
    if (!playing(t)) return;
    vp_voice_set_pitch(s_audio, t->voice, (float)t->speed);
    vp_voice_set_volume(s_audio, t->voice, t->vol * GAME_VOLUME, 0);
}

void sound::pauseAllTracks()
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++) pauseTrack(i);
}

void sound::pauseAllTracksBut(std::size_t track)
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++)
        if (i != track) pauseTrack(i);
}

void sound::unpauseAllTracks()
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++) unpauseTrack(i);
}

// Applies to the track as it plays, as in the old mixer.
void sound::setTrackSpeed(std::size_t track, double speed)
{
    Track* t = at(track);
    if (!t) return;
    t->speed = speed;
    if (!t->paused && playing(t) && speed > 0) vp_voice_set_pitch(s_audio, t->voice, (float)speed);
}

bool sound::isTrackPlaying(std::size_t track)
{
    Track* t = at(track);
    return t && playing(t);
}

int sound::playTrackGroup(std::size_t trackFirst, std::size_t trackLast)
{
    for (std::size_t i = trackFirst; i <= trackLast; i++) {
        if (!isTrackPlaying(i)) {
            playTrack(i);
            return i;
        }
    }
    // Just retrigger the first one
    playTrack(trackFirst);
    return trackFirst;
}
