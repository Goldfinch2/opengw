#include "settings.hpp"
#include "debuglog.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

static std::unique_ptr<settings> instance;

const settings& settings::get()
{
    if (!instance) {
        instance = std::make_unique<settings>();
    }

    return *instance;
}

settings::settings()
{
    mCreditsPerGame = false; // false = Freeplay

    mEnableGlow = true;

    mGridSmoothing = false;
    mParticleSmoothing = false;
    mEnemySmoothing = false;
    mPlayerSmoothing = true;
    mStarSmoothing = true;

    mAttractors = 50;
    mGridPasses = 4;

    serialPort[0] = '\0';
    sortVectors = true;
    marqueeDisplayMode = MARQUEE_MODE_FIT;

    marquee[0] = '\0';
    advmamerc[0] = '\0';
    sounds[0] = '\0';
    save[0] = '\0';

#if defined(USE_DVG_RENDERER)
    mParticles = 100;
    gridSpacing = 10;
    targetFps = 40;
    dvgRefreshRate = 40;
#else
    mParticles = 3000;
    gridSpacing = 4;
    targetFps = 60;
    dvgRefreshRate = 40;
#endif

    load();
}

// Returns true if the key was handled
static bool applyKey(settings& s, const char* key, const char* value)
{
    if (strcmp(key, "enable_glow") == 0)
        s.mEnableGlow = (strcmp(value, "true") == 0);
    else if (strcmp(key, "grid_smoothing") == 0)
        s.mGridSmoothing = (strcmp(value, "true") == 0);
    else if (strcmp(key, "particle_smoothing") == 0)
        s.mParticleSmoothing = (strcmp(value, "true") == 0);
    else if (strcmp(key, "enemy_smoothing") == 0)
        s.mEnemySmoothing = (strcmp(value, "true") == 0);
    else if (strcmp(key, "player_smoothing") == 0)
        s.mPlayerSmoothing = (strcmp(value, "true") == 0);
    else if (strcmp(key, "star_smoothing") == 0)
        s.mStarSmoothing = (strcmp(value, "true") == 0);
    else if (strcmp(key, "attractors") == 0)
        s.mAttractors = atoi(value);
    else if (strcmp(key, "particles") == 0)
        s.mParticles = atoi(value);
    else if (strcmp(key, "grid_passes") == 0)
        s.mGridPasses = atoi(value);
    else if (strcmp(key, "grid_spacing") == 0)
        s.gridSpacing = atoi(value);
    else if (strcmp(key, "credits_per_game") == 0)
        s.mCreditsPerGame = (strcmp(value, "true") == 0);
    else if (strcmp(key, "target_fps") == 0)
        s.targetFps = atoi(value);
    else if (strcmp(key, "sort_vectors") == 0)
        s.sortVectors = (strcmp(value, "true") == 0);
    else if (strcmp(key, "dvg_refresh_rate") == 0)
        s.dvgRefreshRate = atoi(value);
    else if (strcmp(key, "marquee") == 0) {
        strncpy(s.marquee, value, sizeof(s.marquee) - 1);
        s.marquee[sizeof(s.marquee) - 1] = '\0';
    }
    else if (strcmp(key, "advmamerc") == 0) {
        strncpy(s.advmamerc, value, sizeof(s.advmamerc) - 1);
        s.advmamerc[sizeof(s.advmamerc) - 1] = '\0';
    }
    else if (strcmp(key, "sounds") == 0) {
        strncpy(s.sounds, value, sizeof(s.sounds) - 1);
        s.sounds[sizeof(s.sounds) - 1] = '\0';
    }
    else if (strcmp(key, "save") == 0) {
        strncpy(s.save, value, sizeof(s.save) - 1);
        s.save[sizeof(s.save) - 1] = '\0';
    }
    else
        return false;

    return true;
}

void settings::load()
{
    FILE* file = fopen("opengw.cfg", "r");
    if (!file) {
        debuglog::log(true,"opengw.cfg not found, using defaults\n");
        return;
    }

    debuglog::log(true, "Loading opengw.cfg\n");

    // Section tracking: 0 = global (applies to all), 1 = sdl, 2 = dvg
    enum Section { SECTION_GLOBAL, SECTION_SDL, SECTION_DVG };
    Section section = SECTION_GLOBAL;

#if defined(USE_DVG_RENDERER)
    const Section mySection = SECTION_DVG;
#else
    const Section mySection = SECTION_SDL;
#endif

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        int len = (int)strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[--len] = 0;
        if (len > 0 && buffer[len - 1] == '\r')
            buffer[--len] = 0;

        if (strlen(buffer) == 0 || buffer[0] == '#')
            continue;

        // Section headers
        if (buffer[0] == '[') {
            if (strcmp(buffer, "[sdl]") == 0)
                section = SECTION_SDL;
            else if (strcmp(buffer, "[dvg]") == 0)
                section = SECTION_DVG;
            else
                debuglog::log(true,"settings: unknown section '%s'\n", buffer);
            continue;
        }

        // Skip keys that belong to the other build's section
        if (section != SECTION_GLOBAL && section != mySection)
            continue;

        char* key = strtok(buffer, " ");
        if (!key) continue;

        char* value = strtok(nullptr, " ");
        if (!value) continue;

        if (!applyKey(*this, key, value))
            debuglog::log(true,"settings: unknown key '%s'\n", key);
    }

    fclose(file);

#if defined(USE_DVG_RENDERER)
    // Read serial port from advmame.rc (vector_aux_renderer_port)
    {
        const char* rcPath = advmamerc[0] ? advmamerc : "/home/pi/.advance/advmame.rc";
        FILE* rc = fopen(rcPath, "r");
        if (rc)
        {
            char rcLine[1024];
            while (fgets(rcLine, sizeof(rcLine), rc))
            {
                int rcLen = (int)strlen(rcLine);
                while (rcLen > 0 && (rcLine[rcLen - 1] == '\n' || rcLine[rcLen - 1] == '\r'))
                    rcLine[--rcLen] = 0;

                if (strncmp(rcLine, "vector_aux_renderer_port", 24) == 0 &&
                    (rcLine[24] == ' ' || rcLine[24] == 0))
                {
                    char* val = rcLine + 24;
                    while (*val == ' ') val++;
                    if (*val && strcmp(val, "auto") != 0)
                    {
                        strncpy(serialPort, val, sizeof(serialPort) - 1);
                        serialPort[sizeof(serialPort) - 1] = '\0';
                        debuglog::log(true, "Serial port from %s: %s\n", rcPath, serialPort);
                    }
                }

                if (strncmp(rcLine, "vector_aux_marquee_display_mode", 31) == 0 &&
                    (rcLine[31] == ' ' || rcLine[31] == 0))
                {
                    char* val = rcLine + 31;
                    while (*val == ' ') val++;
                    if (strcmp(val, "stretch") == 0)
                        marqueeDisplayMode = MARQUEE_MODE_STRETCH;
                    else if (strcmp(val, "zoom") == 0)
                        marqueeDisplayMode = MARQUEE_MODE_ZOOM;
                    else
                        marqueeDisplayMode = MARQUEE_MODE_FIT;
                    debuglog::log(true, "Marquee display mode from %s: %s (%d)\n",
                                  rcPath, val, marqueeDisplayMode);
                }
            }
            fclose(rc);
        }
        else
        {
            debuglog::log(true, "Could not open %s for serial port\n", rcPath);
        }
    }
#endif
}

std::string settings::marqueeFile() const
{
    std::string dir = marquee[0] ? marquee : "artwork";
    std::string primary = dir + "/opengw.png";

    FILE* f = fopen(primary.c_str(), "r");
    if (f) {
        fclose(f);
        return primary;
    }

    return dir + "/default.png";
}

std::string settings::soundFile(const char* filename) const
{
    if (sounds[0])
        return std::string(sounds) + "/" + filename;
    return std::string("sounds/") + filename;
}

std::string settings::saveFile(const char* filename) const
{
    if (save[0])
        return std::string(save) + "/" + filename;
    return std::string(filename);
}
