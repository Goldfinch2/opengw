#include "settings.hpp"
#include "debuglog.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

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

// Helper: read from build-specific section first, fall back to global
static std::string cfgGet(const CfgFile& cfg, const std::string& sec, const std::string& key, const std::string& def) {
    if (cfg.hasKey(sec, key)) return cfg.getString(sec, key, def);
    return cfg.getString("", key, def);
}
static int cfgGetI(const CfgFile& cfg, const std::string& sec, const std::string& key, int def) {
    if (cfg.hasKey(sec, key)) return cfg.getInt(sec, key, def);
    return cfg.getInt("", key, def);
}
static bool cfgGetB(const CfgFile& cfg, const std::string& sec, const std::string& key, bool def) {
    if (cfg.hasKey(sec, key)) return cfg.getBool(sec, key, def);
    return cfg.getBool("", key, def);
}

static void cfgCopyStr(const CfgFile& cfg, const std::string& sec, const std::string& key, char* dst, size_t dstSize) {
    std::string val = cfgGet(cfg, sec, key, "");
    if (!val.empty()) snprintf(dst, dstSize, "%s", val.c_str());
}

void settings::load()
{
    CfgFile cfg;
    if (!cfg.load("opengw.cfg")) {
        debuglog::log(true, "opengw.cfg not found, using defaults\n");
        return;
    }

    debuglog::log(true, "Loading opengw.cfg\n");

#if defined(USE_DVG_RENDERER)
    const std::string sec = "dvg";
#else
    const std::string sec = "sdl";
#endif

    mEnableGlow        = cfgGetB(cfg, sec, "enable_glow",        mEnableGlow);
    mGridSmoothing     = cfgGetB(cfg, sec, "grid_smoothing",     mGridSmoothing);
    mParticleSmoothing = cfgGetB(cfg, sec, "particle_smoothing", mParticleSmoothing);
    mEnemySmoothing    = cfgGetB(cfg, sec, "enemy_smoothing",    mEnemySmoothing);
    mPlayerSmoothing   = cfgGetB(cfg, sec, "player_smoothing",   mPlayerSmoothing);
    mStarSmoothing     = cfgGetB(cfg, sec, "star_smoothing",     mStarSmoothing);
    mAttractors        = cfgGetI(cfg, sec, "attractors",         mAttractors);
    mParticles         = cfgGetI(cfg, sec, "particles",          mParticles);
    mGridPasses        = cfgGetI(cfg, sec, "grid_passes",        mGridPasses);
    gridSpacing        = cfgGetI(cfg, sec, "grid_spacing",       gridSpacing);
    mCreditsPerGame    = cfgGetB(cfg, sec, "credits_per_game",   mCreditsPerGame);
    targetFps          = cfgGetI(cfg, sec, "target_fps",         targetFps);
    sortVectors        = cfgGetB(cfg, sec, "sort_vectors",       sortVectors);
    dvgRefreshRate     = cfgGetI(cfg, sec, "dvg_refresh_rate",   dvgRefreshRate);

    cfgCopyStr(cfg, sec, "marquee",   marquee,   sizeof(marquee));
    cfgCopyStr(cfg, sec, "advmamerc", advmamerc, sizeof(advmamerc));
    cfgCopyStr(cfg, sec, "sounds",    sounds,    sizeof(sounds));
    cfgCopyStr(cfg, sec, "save",      save,      sizeof(save));

#if defined(USE_DVG_RENDERER)
    // Read serial port and display mode from advmame.rc
    {
        const char* rcPath = advmamerc[0] ? advmamerc : "/home/pi/.advance/advmame.rc";
        AdvmameRC rc;
        if (advmameLoad(rcPath, rc)) {
            if (rc.config.isDvgEnabled()) {
                strncpy(serialPort, rc.config.rendererPort.c_str(), sizeof(serialPort) - 1);
                serialPort[sizeof(serialPort) - 1] = '\0';
                debuglog::log(true, "Serial port from %s: %s\n", rcPath, serialPort);
            }
            if (rc.config.marqueeDisplayMode == "stretch")
                marqueeDisplayMode = MARQUEE_MODE_STRETCH;
            else if (rc.config.marqueeDisplayMode == "zoom")
                marqueeDisplayMode = MARQUEE_MODE_ZOOM;
            else
                marqueeDisplayMode = MARQUEE_MODE_FIT;
            debuglog::log(true, "Marquee display mode: %d\n", marqueeDisplayMode);
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
