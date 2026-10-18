// OpenGW.cpp: the entry point. OpenGW on the VectorPie SDK: vp_run owns the
// loop. The game steps 60 times a second (scene::run, after the controls are
// read) and draws at the output's rate (scene::draw through renderer_vp.cpp),
// on the vector board (frame_hz in game.ini's [dvg] for the USB-DVG,
// [beamforge] for BeamForge) or in a window. The exit button returns to the
// VectorPie menu; the SDK closes everything.

#include "controls.hpp"
#include "debuglog.h"
#include "game.hpp"
#include "renderer.h"
#include "scene.hpp"
#include "settings.hpp"
#include "sincos.hpp"
#include "sound.hpp"

#include <vectorpie/vp.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

extern scene g_scene;

// VP_VERTICAL=1: a vertically mounted monitor (the SDK shapes its window so)
static bool vertical()
{
    const char* v = std::getenv("VP_VERTICAL");
    return v && v[0] == '1';
}

static int init(vp_game* g)
{
    debuglog::log(true, "== opengw boot ==\n");
    // The settings' section: [sdl] in a window, [dvg] or [beamforge] on a board.
    vp_dvg* out = vp_game_output(g);
    settings::setOutput(vp_dvg_get_output(out) != VP_DVG_OUTPUT_BOARD ? settings::WINDOW
                        : vp_dvg_hardware(out) == VP_HW_BEAMFORGE ? settings::BEAMFORGE
                        : settings::DVG);
    std::srand((unsigned int)std::time(nullptr));
    make_sin_cos_tables();
    renderer::init(vertical());
    sound::setAudio(vp_game_audio(g));
    controls::setInput(vp_game_input(g));
    g_scene.init();
    // The board is square: aspect 1:1, as the DVG build had it.
    g_scene.size(1.0f, 1.0f);
    return VP_OK;
}

static void update(vp_game* g, float /*dt*/)
{
    controls::setInput(vp_game_input(g));
    g_scene.run();
}

static void draw(vp_game* /*g*/, vp_dvg* d)
{
    renderer::setOutput(d);
    renderer::beginFrame();
    scene::mPass = scene::RENDERPASS_PRIMARY;
    g_scene.draw(scene::RENDERPASS_PRIMARY);
    renderer::present();
    renderer::setOutput(nullptr);
}

static void game_shutdown(vp_game* /*g*/)
{
    g_scene.shutdown();
    renderer::shutdown();
    debuglog::close();
}

int main(int argc, char** argv)
{
    vp_game_desc desc;
    std::memset(&desc, 0, sizeof desc);
    desc.title          = "Geometry Wars";
    desc.init           = init;
    desc.update         = update;
    desc.draw           = draw;
    desc.shutdown       = game_shutdown;
    desc.update_hz      = 60;
    desc.frame_hz       = 60;
    desc.frame_hz_board = 40;          // game.ini's [dvg] / [beamforge] frame_hz override it
    return vp_run(&desc, argc, argv);
}
