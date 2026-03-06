#include "settings.hpp"

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

    displayWidth = 800;
    displayHeight = 600;

    serialPort[0] = '\0';
    sortVectors = true;

#if defined(USE_DVG_RENDERER)
    mParticles = 100;
    gridSpacing = 20;
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
    if (strcmp(key, "display_width") == 0)
        s.displayWidth = atoi(value);
    else if (strcmp(key, "display_height") == 0)
        s.displayHeight = atoi(value);
    else if (strcmp(key, "enable_glow") == 0)
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
    else if (strcmp(key, "serial_port") == 0) {
        strncpy(s.serialPort, value, sizeof(s.serialPort) - 1);
        s.serialPort[sizeof(s.serialPort) - 1] = '\0';
    }
    else if (strcmp(key, "sort_vectors") == 0)
        s.sortVectors = (strcmp(value, "true") == 0);
    else if (strcmp(key, "dvg_refresh_rate") == 0)
        s.dvgRefreshRate = atoi(value);
    else
        return false;

    return true;
}

void settings::load()
{
    FILE* file = fopen("settings.txt", "r");
    if (!file) {
        printf("settings.txt not found, using defaults\n");
        return;
    }

    printf("Loading settings.txt\n");

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
                printf("settings: unknown section '%s'\n", buffer);
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
            printf("settings: unknown key '%s'\n", key);
    }

    fclose(file);
}
