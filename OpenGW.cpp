#include "controls.hpp"
#include "debuglog.h"
#include "game.hpp"
#include "scene.hpp"
#include "settings.hpp"
#include "sincos.hpp"

#include <SDL3/SDL.h>

#include <cstdio>
#include <memory>

#if !defined(USE_DVG_RENDERER)
#include "blur.hpp"

#include <SDL3/SDL_opengl.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

// declare image buffers
#ifdef __amigaos4__
// MiniGL / Warp3D requires power of two size
static constexpr int blurBufferWidth = 256;
static constexpr int blurBufferHeight = 256;
#else
static constexpr int blurBufferWidth = 500;
static constexpr int blurBufferHeight = 250;
#endif

static std::vector<ColorRGB> blurBuffer(blurBufferWidth * blurBufferHeight);

static SDL_GLContext context;
static bool oglInited = false;
static unsigned int texOffscreen;

static void createContext(int width, int height);
static void destroyContext();
static void resizeViewport(int cx, int cy);
static void createOffscreens();
static void drawOffscreens();

#define CONTEXT_PRIMARY 0
#define CONTEXT_GLOW    1

static Uint32 lastTime;
static Uint32 fpsTime;
static int frameCount;
static int fps;
static void updateFps(Uint32 now);

#else // USE_DVG_RENDERER

#include "font.hpp"
#include "highscore.hpp"
#include "players.hpp"
#include "renderer.h"

#endif // !USE_DVG_RENDERER

// Shared globals
static SDL_Window* window;
static int mWidth, mHeight;
const bool* keyboardState;

static bool handleEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.scancode == SDL_SCANCODE_ESCAPE)
                return false;
            break;
#if !defined(USE_DVG_RENDERER)
        case SDL_EVENT_WINDOW_RESIZED:
            resizeViewport(e.window.data1, e.window.data2);
            break;
#endif
        case SDL_EVENT_GAMEPAD_ADDED:
            theGame->mControls->handleGamepadAdded(e.cdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            theGame->mControls->handleGamepadRemoved(e.cdevice.which);
            break;
        }
    }

    keyboardState = SDL_GetKeyboardState(nullptr);
    return true;
}

// ============================================================================
// SDL / OpenGL build
// ============================================================================
#if !defined(USE_DVG_RENDERER)

extern scene g_scene;

static void run()
{
    if (!oglInited)
        return;

    constexpr Uint32 logicRate = 60;
    constexpr Uint32 logicPeriod = 1000 / logicRate;

    const Uint32 targetFps = settings::get().targetFps;
    const Uint32 framePeriod = 1000 / targetFps;

    Uint32 lastLogicUpdate = SDL_GetTicks();
    Uint32 lastFrameTime = SDL_GetTicks();

    bool running = true;

    while (running) {
        const Uint32 now = SDL_GetTicks();

        if (now - lastFrameTime < framePeriod)
            continue;
        lastFrameTime = now;

        while ((now - lastLogicUpdate) > logicPeriod) {
            lastLogicUpdate += logicPeriod;
            if (!handleEvents()) {
                running = false;
            }

            g_scene.run();
        }

        drawOffscreens();

        SDL_GL_SwapWindow(window);
        updateFps(now);
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    debuglog::init();
    debuglog::log(true, "OpenGW SDL\n");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) {
        // No display server — try KMSDRM
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "kmsdrm");
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) {
            debuglog::log(true, "SDL_Init failed: %s\n", SDL_GetError());
            return 0;
        }
        debuglog::log(true, "No display server, using kmsdrm video driver\n");
    }

    // Detect display resolution
    int displayWidth = settings::get().displayWidth;
    int displayHeight = settings::get().displayHeight;
    {
        SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(displayID);
        if (dm) {
            displayWidth = dm->w;
            displayHeight = dm->h;
            debuglog::log(true, "Display mode: %dx%d\n", displayWidth, displayHeight);
        }
    }

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;

    window = SDL_CreateWindow("OpenGW",
                              displayWidth, displayHeight, flags);

    if (window) {
        SDL_HideCursor();
        srand(SDL_GetTicks());
        make_sin_cos_tables();
        g_scene.init();
        createContext(displayWidth, displayHeight);
        run();
        g_scene.shutdown();
        destroyContext();

        SDL_DestroyWindow(window);
    } else {
        debuglog::log(true, "Failed to create window: %s\n", SDL_GetError());
    }

    debuglog::log(true, "SDL_Quit\n");
    debuglog::close();
    SDL_Quit();

    return 0;
}

static void createContext(int width, int height)
{
    context = SDL_GL_CreateContext(window);

    if (context == nullptr) {
        debuglog::log(true, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    }

    resizeViewport(width, height);

    // Do stuff with the context here if needed...
    createOffscreens();

    if (!SDL_GL_SetSwapInterval(0)) {
        debuglog::log(true, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
    }

    if (!SDL_GL_MakeCurrent(window, context)) {
        debuglog::log(true, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
    }

    oglInited = true;
}

static void destroyContext()
{
    oglInited = false;

    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DestroyContext(context);
}

static void resizeViewport(int cx, int cy)
{
    g_scene.size(cx, cy);
    mWidth = cx;
    mHeight = cy;
}

static void createOffscreens()
{
    std::vector<char> colorBits(blurBufferWidth * blurBufferHeight * 3);

    // texture creation..
    glGenTextures(1, &texOffscreen);
    glBindTexture(GL_TEXTURE_2D, texOffscreen);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, blurBufferWidth, blurBufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, colorBits.data());
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, blurBufferWidth, blurBufferHeight, GL_RGB, GL_UNSIGNED_BYTE, colorBits.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

static void drawOffscreens()
{
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, (int*)viewport);

    if (settings::get().mEnableGlow) {
        // Draw to the blur texture
        {
            glViewport(0, 0, blurBufferWidth, blurBufferHeight);

            g_scene.draw(scene::RENDERPASS_BLUR);

            // Transfer image to the blur texture
            glBindTexture(GL_TEXTURE_2D, texOffscreen);
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, blurBufferWidth, blurBufferHeight, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    // Draw the scene normally
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    g_scene.draw(scene::RENDERPASS_PRIMARY);

    if (settings::get().mEnableGlow) {
        ////////////////////////////////////////////////
        // Do blur

        // Bind the blur texture and copy the screen bits to it
        glBindTexture(GL_TEXTURE_2D, texOffscreen);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, blurBuffer.data());

        const int blurRadius = (game::mGameMode == game::GAMEMODE_ATTRACT || game::mGameMode == game::GAMEMODE_CREDITED) ? 8 : 4;

        superFastBlur(blurBuffer.data(), blurBufferWidth, blurBufferHeight, blurRadius);
        superFastBlur(blurBuffer.data(), blurBufferWidth, blurBufferHeight, blurRadius);

        // Bind the blur result back to our texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, blurBufferWidth, blurBufferHeight, GL_RGB, GL_UNSIGNED_BYTE, blurBuffer.data());

        ////////////////////////////////////////////////
        // Draw the blur texture on top of the existing scene

        // Glowy blending effect
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        if (game::mGameMode == game::GAMEMODE_ATTRACT || game::mGameMode == game::GAMEMODE_CREDITED)
            glColor4f(1, 1, 1, 1);
        else
            glColor4f(1, 1, 1, 1);

        // Draw it on the screen
        glBegin(GL_QUADS);
        glTexCoord2d(0.0, 0.0);
        glVertex2d(-1.0, -1.0);
        glTexCoord2d(1.0, 0.0);
        glVertex2d(1.0, -1.0);
        glTexCoord2d(1.0, 1.0);
        glVertex2d(1.0, 1.0);
        glTexCoord2d(0.0, 1.0);
        glVertex2d(-1.0, 1.0);
        if (game::mGameMode == game::GAMEMODE_ATTRACT || game::mGameMode == game::GAMEMODE_CREDITED) {
            glTexCoord2d(0.0, 0.0);
            glVertex2d(-1.0, -1.0);
            glTexCoord2d(1.0, 0.0);
            glVertex2d(1.0, -1.0);
            glTexCoord2d(1.0, 1.0);
            glVertex2d(1.0, 1.0);
            glTexCoord2d(0.0, 1.0);
            glVertex2d(-1.0, 1.0);
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
}

static void updateFps(Uint32 now)
{
    ++frameCount;

    lastTime = now;

    if ((now - fpsTime) > 1000) {
        fpsTime = now;
        fps = (fps + frameCount) / 2;
        frameCount = 0;
    }
}

// ============================================================================
// DVG build
// ============================================================================
#else // USE_DVG_RENDERER

extern scene g_scene;

static void run()
{
    constexpr Uint32 logicRate = 60;
    constexpr Uint32 logicPeriod = 1000 / logicRate;

    const Uint32 targetFps = settings::get().dvgRefreshRate;
    const Uint32 framePeriod = 1000 / targetFps;

    Uint32 lastLogicUpdate = SDL_GetTicks();
    Uint32 lastFrameTime = SDL_GetTicks();

    bool running = true;

    while (running) {
        const Uint32 now = SDL_GetTicks();

        if (now - lastFrameTime < framePeriod)
            continue;
        lastFrameTime = now;

        // Run game logic at 60Hz
        while ((now - lastLogicUpdate) > logicPeriod) {
            lastLogicUpdate += logicPeriod;
            if (!handleEvents())
                running = false;

            theGame->run();
        }

        // Render
        renderer::beginFrame(mWidth, mHeight);

        scene::mPass = scene::RENDERPASS_PRIMARY;
        theGame->draw(scene::RENDERPASS_PRIMARY);

        renderer::present();
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    debuglog::init();
    debuglog::log(true, "OpenGW DVG\n");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "kmsdrm");
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) {
            debuglog::log(true, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
        debuglog::log(true, "No display server, using kmsdrm video driver\n");
    }

    // Detect display resolution (used for aspect ratio calculation)
    mWidth = settings::get().displayWidth;
    mHeight = settings::get().displayHeight;
    {
        SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(displayID);
        if (dm) {
            mWidth = dm->w;
            mHeight = dm->h;
            debuglog::log(true, "Display: %dx%d\n", mWidth, mHeight);
        }
    }

    // Create a minimal window (needed for SDL event loop and gamepad)
    window = SDL_CreateWindow("OpenGW DVG", mWidth, mHeight,
                                          SDL_WINDOW_FULLSCREEN);
    if (!window) {
        debuglog::log(true, "Failed to create window: %s\n", SDL_GetError());
        debuglog::close();
        SDL_Quit();
        return 1;
    }
    SDL_HideCursor();

    srand(SDL_GetTicks());
    make_sin_cos_tables();

    g_scene.init();

    debuglog::log(true, "Game created, gameMode=%d gameType=%d\n", game::mGameMode, game::mGameType);
    debuglog::log(true, "Player1 joined=%d state=%d\n", theGame->getPlayer1()->mJoined, theGame->getPlayer1()->getState());
    debuglog::log(true, "Player2 joined=%d state=%d\n", theGame->getPlayer2()->mJoined, theGame->getPlayer2()->getState());
    debuglog::log(true, "Player3 joined=%d state=%d\n", theGame->getPlayer3()->mJoined, theGame->getPlayer3()->getState());
    debuglog::log(true, "Player4 joined=%d state=%d\n", theGame->getPlayer4()->mJoined, theGame->getPlayer4()->getState());

    renderer::init(mWidth, mHeight);

    // Force single player, skip selection screen
    game::mCredits = 1;
    theGame->getPlayer1()->mJoined = true;
    theGame->startGame(game::GAMETYPE_SINGLEPLAYER);

    debuglog::log(true, "startGame called, gameMode=%d gameType=%d\n", game::mGameMode, game::mGameType);
    debuglog::log(true, "Player1 joined=%d state=%d\n", theGame->getPlayer1()->mJoined, theGame->getPlayer1()->getState());
    debuglog::log(true, "Player2 joined=%d state=%d\n", theGame->getPlayer2()->mJoined, theGame->getPlayer2()->getState());
    debuglog::log(true, "Player3 joined=%d state=%d\n", theGame->getPlayer3()->mJoined, theGame->getPlayer3()->getState());
    debuglog::log(true, "Player4 joined=%d state=%d\n", theGame->getPlayer4()->mJoined, theGame->getPlayer4()->getState());
    debuglog::log(true, "numPlayers=%d\n", theGame->numPlayers());

    run();

    g_scene.shutdown();
    renderer::shutdown();

    debuglog::log(true, "Done\n");
    debuglog::close();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#endif // !USE_DVG_RENDERER
