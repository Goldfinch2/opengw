#include "settings.hpp"
#include "debuglog.h"

#include <vectorpie/vp.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

static std::unique_ptr<settings> instance;
static settings::Output s_output = settings::DVG;

void settings::setOutput(Output output)
{
    s_output = output;
}

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

    mGridSmoothing = false;
    mParticleSmoothing = false;
    mEnemySmoothing = false;
    mPlayerSmoothing = true;
    mStarSmoothing = true;

    mAttractors = 50;
    mGridPasses = 4;
    sortVectors = true;

    // the board's (a window takes [sdl]'s from game.ini)
    mParticles = 100;
    gridSpacing = 10;

    load();
}

// game.ini: section -> key -> value ("" is the global section: the game's
// keys come after the package's). "key = value" lines; a comment (';' or
// '#') at the start of a line or after a space.
typedef std::map<std::string, std::map<std::string, std::string>> Cfg;

static bool readCfg(Cfg& cfg)
{
    char path[1024];
    FILE* f = std::fopen(vp_asset_path("game.ini", path, sizeof path), "r");
    if (!f) return false;
    std::string section;
    char line[512];
    while (std::fgets(line, sizeof line, f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        for (char* c = p; *c; c++)
            if ((*c == ';' || *c == '#') && (c == p || c[-1] == ' ' || c[-1] == '\t')) { *c = '\0'; break; }
        char* e = p + std::strlen(p);
        while (e > p && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ' || e[-1] == '\t')) *--e = '\0';
        if (*p == '\0') continue;
        if (*p == '[') {
            char* close = std::strchr(p, ']');
            if (close) section.assign(p + 1, close);
            continue;
        }
        char* eq = std::strchr(p, '=');
        if (!eq) continue;
        char* k = eq;
        while (k > p && (k[-1] == ' ' || k[-1] == '\t')) k--;
        char* v = eq + 1;
        while (*v == ' ' || *v == '\t') v++;
        cfg[section][std::string(p, k)] = v;
    }
    std::fclose(f);
    return true;
}

// The sections to look in, first to last: the output's, then the global.
typedef std::vector<std::string> Sections;

static const std::string* get(const Cfg& cfg, const Sections& secs, const char* key)
{
    for (const std::string& s : secs) {
        auto it = cfg.find(s);
        if (it == cfg.end()) continue;
        auto k = it->second.find(key);
        if (k != it->second.end()) return &k->second;
    }
    return nullptr;
}

static void getI(const Cfg& cfg, const Sections& sec, const char* key, int& out)
{
    if (const std::string* v = get(cfg, sec, key)) out = std::atoi(v->c_str());
}

static void getB(const Cfg& cfg, const Sections& sec, const char* key, bool& out)
{
    if (const std::string* v = get(cfg, sec, key))
        out = *v == "true" || *v == "1" || *v == "yes" || *v == "on";
}

void settings::load()
{
    Cfg cfg;
    const Sections sec = s_output == WINDOW ? Sections { "sdl", "" }
                       : s_output == DVG    ? Sections { "dvg", "" }
                                            : Sections { "beamforge", "dvg", "" };
    if (!readCfg(cfg)) {
        debuglog::log(true, "game.ini not found, using defaults\n");
        return;
    }

    getB(cfg, sec, "grid_smoothing",     mGridSmoothing);
    getB(cfg, sec, "particle_smoothing", mParticleSmoothing);
    getB(cfg, sec, "enemy_smoothing",    mEnemySmoothing);
    getB(cfg, sec, "player_smoothing",   mPlayerSmoothing);
    getB(cfg, sec, "star_smoothing",     mStarSmoothing);
    getI(cfg, sec, "attractors",         mAttractors);
    getI(cfg, sec, "particles",          mParticles);
    getI(cfg, sec, "grid_passes",        mGridPasses);
    getI(cfg, sec, "grid_spacing",       gridSpacing);
    getB(cfg, sec, "credits_per_game",   mCreditsPerGame);
    getB(cfg, sec, "sort_vectors",       sortVectors);
    debuglog::log(true, "settings: game.ini, section [%s]\n", sec[0].c_str());
}

std::string settings::soundFile(const char* filename) const
{
    return std::string("sounds/") + filename;
}

std::string settings::saveFile(const char* filename) const
{
    return std::string(vp_save_dir()) + "/" + filename;
}
