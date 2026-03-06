#include "sound.hpp"

#include <cstdio>

constexpr std::size_t NUM_TRACKS = 200;

sound::sound()
{
    printf("Initing sound\n");

    sound::mTracks.resize(NUM_TRACKS);

    const SDL_AudioSpec spec { SDL_AUDIO_S16, 2, 44100 };
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &spec, sdl3AudioCallback, this);

    if (!stream) {
        fprintf(stderr, "SDL_OpenAudioDeviceStream() failed: %s\n", SDL_GetError());
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

    if (stream) {
        SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(stream));
    }
}

void sound::sdl3AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int /*total_amount*/)
{
    if (additional_amount > 0) {
        Uint8* data = SDL_stack_alloc(Uint8, additional_amount);
        if (data) {
            bufferCallback(userdata, data, additional_amount);
            if (!SDL_PutAudioStreamData(stream, data, additional_amount)) {
                fprintf(stderr, "SDL_PutAudioStreamData() failed: %s\n", SDL_GetError());
            }
            SDL_stack_free(data);
        }
    } else {
        fprintf(stderr, "SDL_stack_alloc() failed: %s\n", SDL_GetError());
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
    if (!SDL_LoadWAV(file, &wave, &srcData, &srcLen)) {
        printf("Failed loading audio track %zu: %s\n", track, SDL_GetError());
        return;
    }

    printf("Loaded audio track %zu\n", track);

    Uint8* dstData = nullptr;
    int dstLen = 0;
    const SDL_AudioSpec srcSpec { wave.format, wave.channels, wave.freq };
    const SDL_AudioSpec dstSpec { SDL_AUDIO_S16, 2, 44100 };

    if (SDL_ConvertAudioSamples(&srcSpec, srcData, srcLen, &dstSpec, &dstData, &dstLen)) {
        mTracks[track].data.resize(dstLen / 2);
        memcpy(mTracks[track].data.data(), dstData, dstLen);
        mTracks[track].len = dstLen / 2;
        mTracks[track].pos = 0;
        mTracks[track].loop = loop;
        mTracks[track].vol = volume;
        mTracks[track].playing = false;
    } else {
        fprintf(stderr, "SDL_ConvertAudioSamples() failed: %s\n", SDL_GetError());
    }


    SDL_free(srcData);
    SDL_free(dstData);
}

void sound::startSound()
{
    if (stream) {
        if (!SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream))) {
            fprintf(stderr, "SDL_ResumeAudioDevice() failed: %s\n", SDL_GetError());
        }
    }
}

void sound::stopSound()
{
    if (stream) {
        if (!SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(stream))) {
            fprintf(stderr, "SDL_PauseAudioDevice() failed: %s\n", SDL_GetError());
        }
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
