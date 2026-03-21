#include "sound.hpp"
#include "debuglog.h"

constexpr std::size_t NUM_TRACKS = 200;

sound::sound()
{
    debuglog::log(true,"Initing sound\n");

    sound::mTracks.resize(NUM_TRACKS);

    SDL_AudioSpec desired {};
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = bufferCallback;
    desired.userdata = this;

    mAudioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    if (mAudioDevice == 0) {
        debuglog::log(true, "SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
    }

    for (auto& track: mTracks) {
        track.playing = false;
        track.paused = false;
        track.speed = 1;
    }
}

sound::~sound()
{
    stopSound();
    stopAllTracks();

    if (mAudioDevice) {
        SDL_CloseAudioDevice(mAudioDevice);
    }
}

void sound::bufferCallback(void* userdata, Uint8* stream, int len)
{
    auto buf = reinterpret_cast<Sint16*>(stream);
    auto thisPtr = static_cast<sound*>(userdata);

    memset(buf, 0, len);

    const std::size_t samplesPerChannel = len / sizeof(Sint16) / 2;

    if (samplesPerChannel > thisPtr->mLeftSamples.size()) {
        thisPtr->mLeftSamples.resize(samplesPerChannel);
        thisPtr->mRightSamples.resize(samplesPerChannel);
    }

    memset(thisPtr->mLeftSamples.data(), 0, samplesPerChannel * sizeof(float));
    memset(thisPtr->mRightSamples.data(), 0, samplesPerChannel * sizeof(float));

    constexpr int max_int_val = (1 << 16);

    // Fill the input buffers

    for (auto& track: thisPtr->mTracks) {
        int rIndex = 0;
        int lIndex = 0;

        if (track.playing && !track.paused) {
            const auto volume = track.vol / 2.0f / max_int_val;

            for (std::size_t s = 0; s < samplesPerChannel; s++) {
                // Stream the audio data
                const int iPos = static_cast<int>(track.pos);

                const float left = track.data[iPos] * volume;
                const float right = track.data[iPos + 1] * volume;

                thisPtr->mLeftSamples[lIndex] += left;
                thisPtr->mRightSamples[rIndex] += right;

                ++lIndex;
                ++rIndex;

                // Advance the track data position

                track.pos += 2.0 * track.speed;

                if (track.pos >= track.len) {
                    if (track.loop) {
                        track.pos -= track.len;
                    } else {
                        track.pos = 0;
                        track.playing = false;
                        break;
                    }
                }
            }
        }
    }

    // Fill the output buffer
    for (std::size_t i = 0, b = 0; i < samplesPerChannel; i++) {
        const Sint16 left = thisPtr->mLeftSamples[i] * max_int_val;
        const Sint16 right = thisPtr->mRightSamples[i] * max_int_val;
        buf[b++] = left;
        buf[b++] = right;
    }
}

void sound::loadTrack(const char* file, std::size_t track, float volume, bool loop /*=false*/)
{
    SDL_AudioSpec wave {};
    Uint8* srcData = nullptr;
    Uint32 srcLen = 0;

    // Load the sound file and convert it to 16-bit stereo at 44Hz
    if (SDL_LoadWAV(file, &wave, &srcData, &srcLen) == NULL) {
        debuglog::log(true,"Failed loading audio track %zu: %s\n", track, SDL_GetError());
        return;
    }

    debuglog::log(true,"Loaded audio track %zu\n", track);

    // Build a conversion from the source format to our target format
    SDL_AudioCVT cvt;
    int ret = SDL_BuildAudioCVT(&cvt,
                                wave.format, wave.channels, wave.freq,
                                AUDIO_S16, 2, 44100);

    if (ret < 0) {
        debuglog::log(true, "SDL_BuildAudioCVT() failed: %s\n", SDL_GetError());
        SDL_FreeWAV(srcData);
        return;
    }

    if (ret == 0) {
        // No conversion needed — source format already matches target
        mTracks[track].data.resize(srcLen / 2);
        memcpy(mTracks[track].data.data(), srcData, srcLen);
        mTracks[track].len = srcLen / 2;
    } else {
        // Conversion needed
        cvt.len = srcLen;
        cvt.buf = (Uint8*)SDL_malloc(srcLen * cvt.len_mult);
        if (!cvt.buf) {
            debuglog::log(true, "SDL_malloc() failed for audio conversion\n");
            SDL_FreeWAV(srcData);
            return;
        }
        memcpy(cvt.buf, srcData, srcLen);

        if (SDL_ConvertAudio(&cvt) != 0) {
            debuglog::log(true, "SDL_ConvertAudio() failed: %s\n", SDL_GetError());
            SDL_free(cvt.buf);
            SDL_FreeWAV(srcData);
            return;
        }

        int dstLen = cvt.len_cvt;
        mTracks[track].data.resize(dstLen / 2);
        memcpy(mTracks[track].data.data(), cvt.buf, dstLen);
        mTracks[track].len = dstLen / 2;
        SDL_free(cvt.buf);
    }

    mTracks[track].pos = 0;
    mTracks[track].loop = loop;
    mTracks[track].vol = volume;
    mTracks[track].playing = false;

    SDL_FreeWAV(srcData);
}

void sound::startSound()
{
    if (mAudioDevice) {
        SDL_PauseAudioDevice(mAudioDevice, 0);
    }
}

void sound::stopSound()
{
    if (mAudioDevice) {
        SDL_PauseAudioDevice(mAudioDevice, 1);
    }
}

void sound::playTrack(std::size_t track)
{
    mTracks[track].pos = 0;
    mTracks[track].playing = true;
}

void sound::stopTrack(std::size_t track)
{
    mTracks[track].pos = 0;
    mTracks[track].playing = false;
}

void sound::stopAllTracks()
{
    for (auto& track: mTracks) {
        track.pos = 0;
        track.playing = false;
    }
}

void sound::stopAllTracksBut(std::size_t track)
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++) {
        if (i != track) {
            mTracks[i].pos = 0;
            mTracks[i].playing = false;
        }
    }
}

void sound::pauseTrack(std::size_t track)
{
    mTracks[track].paused = true;
}

void sound::unpauseTrack(std::size_t track)
{
    mTracks[track].paused = false;
}

void sound::pauseAllTracks()
{
    for (auto& track: mTracks) {
        track.paused = true;
    }
}

void sound::pauseAllTracksBut(std::size_t track)
{
    for (std::size_t i = 0; i < NUM_TRACKS; i++) {
        if (i != track) {
            mTracks[i].paused = true;
        }
    }
}

void sound::unpauseAllTracks()
{
    for (auto& track: mTracks) {
        track.paused = false;
    }
}

void sound::setTrackSpeed(std::size_t track, double speed)
{
    mTracks[track].speed = speed;
}

bool sound::isTrackPlaying(std::size_t track)
{
    bool playing = mTracks[track].playing;
    return playing;
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
