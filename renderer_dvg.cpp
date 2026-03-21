#ifdef USE_DVG_RENDERER

// DVG (Digital Vector Generator) backend for the renderer abstraction.
// Outputs real vectors to a vector CRT via DVG hardware.

#include "renderer.h"
#include "dvg_cmd.h"
#include "debuglog.h"
#include "font.hpp"
#include "settings.hpp"

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <algorithm>

// ─── Static state ────────────────────────────────────────────────────────────

static DvgCmd g_dvg;
static int g_screenWidth = 0, g_screenHeight = 0;

static bool g_verticalDisplay = false;
static float g_camX = 0.0f, g_camY = 0.0f, g_camZ = 0.0f;
static float g_fovY = 70.0f;

static int   g_lineVertIdx = 0;
static float g_lineX0, g_lineY0;
static float g_lineR0, g_lineG0, g_lineB0, g_lineA0;

#define MAX_LOOP_VERTS 1024
static float g_loopX[MAX_LOOP_VERTS], g_loopY[MAX_LOOP_VERTS];
static int   g_loopCount = 0;
static float g_loopR, g_loopG, g_loopB, g_loopA;

static float g_stripLastX, g_stripLastY;
static bool  g_stripHasFirst = false;

static float g_curR = 1.0f, g_curG = 1.0f, g_curB = 1.0f, g_curA = 1.0f;
static bool  g_screenSpace = false;
static uint32_t g_targetFps = 40;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static bool worldToDvg(float wx, float wy, int32_t* outX, int32_t* outY)
{
    float ex = wx + g_camX;
    float ey = wy + g_camY;
    float ez = g_camZ;

    if (ez >= 0.0f) ez = -0.01f;

    float halfFovRad = (g_fovY * 0.5f) * (3.14159265f / 180.0f);
    float tanHalf = tanf(halfFovRad);
    float visHalfH = -ez * tanHalf;
    float visHalfW = visHalfH;

    float nx = ex / visHalfW;
    float ny = ey / visHalfH;

    *outX = (int32_t)((nx + 1.0f) * 0.5f * DVG_RES_MAX);
    *outY = (int32_t)((ny + 1.0f) * 0.5f * DVG_RES_MAX);

    if (g_verticalDisplay)
        *outY = 512 + (int32_t)(0.75f * *outY);

    return true;
}

static void colorToRgb(float r, float g, float b, float a,
                        uint8_t* outR, uint8_t* outG, uint8_t* outB)
{
    *outR = (uint8_t)std::min(std::max(r * a * 255.0f, 0.0f), 255.0f);
    *outG = (uint8_t)std::min(std::max(g * a * 255.0f, 0.0f), 255.0f);
    *outB = (uint8_t)std::min(std::max(b * a * 255.0f, 0.0f), 255.0f);
}

static void addWorldLine(float x0, float y0, float x1, float y1,
                          float r, float g, float b, float a)
{
    if (a < 0.1f) return;

    int32_t dx0, dy0, dx1, dy1;

    if (g_screenSpace) {
        dx0 = (int32_t)((x0 + 1.0f) * 0.5f * DVG_RES_MAX);
        dy0 = (int32_t)((y0 + 1.0f) * 0.5f * DVG_RES_MAX);
        dx1 = (int32_t)((x1 + 1.0f) * 0.5f * DVG_RES_MAX);
        dy1 = (int32_t)((y1 + 1.0f) * 0.5f * DVG_RES_MAX);

        if (g_verticalDisplay) {
            dy0 = 512 + (int32_t)(0.75f * dy0);
            dy1 = 512 + (int32_t)(0.75f * dy1);
        }
    } else {
        if (!worldToDvg(x0, y0, &dx0, &dy0)) return;
        if (!worldToDvg(x1, y1, &dx1, &dy1)) return;
    }

    uint8_t cr, cg, cb;
    colorToRgb(r, g, b, a, &cr, &cg, &cb);
    g_dvg.addVec(dx0, dy0, dx1, dy1, cr, cg, cb);
}

static void getDvgInfo()
{
    char jsonBuf[DVG_MAX_JSON_SIZE + 1];
    if (!g_dvg.queryDvgInfo(jsonBuf, sizeof(jsonBuf)))
    {
        debuglog::log(true, "getDvgInfo: query failed\n");
        return;
    }

    const char* p = strstr(jsonBuf, "\"vertical\"");
    if (p)
    {
        p += strlen("\"vertical\"");
        while (*p == ' ' || *p == ':') p++;
        g_verticalDisplay = (strncmp(p, "true", 4) == 0);
    }

    debuglog::log(true, "DVG JSON: %s\n", jsonBuf);
    debuglog::log(true, "DVG orientation: vertical=%d\n", g_verticalDisplay);
}

static void serialSend()
{
    const settings& s = settings::get();

    if (s.sortVectors)
        g_dvg.sortVectors();

    g_dvg.buildFrameCommands();
    g_dvg.sendFrame();
}

// ─── Renderer interface ──────────────────────────────────────────────────────

namespace renderer
{

void init(int width, int height)
{
    g_screenWidth  = width;
    g_screenHeight = height;

    if (!g_dvg.init())
    {
        debuglog::log(true, "DVG renderer: allocation failed\n");
        return;
    }

    const settings& s = settings::get();

    if (!s.serialPort[0])
    {
        debuglog::log(true, "DVG renderer: no serial_port configured\n");
        debuglog::log(true, "DVG renderer: running without hardware\n");
        return;
    }

    if (!g_dvg.openSerial(s.serialPort))
    {
        debuglog::log(true, "DVG renderer: failed to open %s\n", s.serialPort);
        return;
    }

    getDvgInfo();

    int cfgFps = s.dvgRefreshRate;
    if (cfgFps < 5)  cfgFps = 5;
    if (cfgFps > 60) cfgFps = 60;
    g_targetFps = (uint32_t)cfgFps;

    debuglog::log(true, "DVG renderer initialized — %u fps\n", g_targetFps);
}

void shutdown()
{
    if (g_dvg.isSerialOpen())
    {
        SDL_Delay(100);
        g_dvg.sendExitSequence();
        SDL_Delay(50);
        g_dvg.closeSerial();
    }

    g_dvg.shutdown();
    debuglog::log(true, "DVG renderer shutdown\n");
}

void beginFrame(int width, int height)
{
    g_screenWidth  = width;
    g_screenHeight = height;
    g_dvg.beginFrame();
    g_camX = g_camY = g_camZ = 0.0f;
}

void present()
{
    static uint32_t frameCount = 0;
    if (++frameCount % 120 == 0)
        debuglog::log(true, "DVG: %u vectors this frame\n", g_dvg.vecCount());

    if (g_dvg.isSerialOpen() && g_dvg.vecCount() > 0)
        serialSend();

    g_dvg.beginFrame();
}

// ── State ────────────────────────────────────────────────────────────────

void setColor(float r, float g, float b, float a)
{
    g_curR = r; g_curG = g; g_curB = b; g_curA = a;
}

void setLineWidth(float)  { }
void setPointSize(float)  { }
void enableLineSmooth(bool)  { }
void enablePointSmooth(bool) { }
void enableBlend()    { }
void disableBlend()   { }
void setBlendAdditive()      { }
void setBlendMultiplicative() { }

// ── Transform ────────────────────────────────────────────────────────────

void translate(float x, float y, float z)
{
    g_camX += x;
    g_camY += y;
    g_camZ += z;
}

void setScreenSpace(bool enabled)
{
    g_screenSpace = enabled;
    font::AlphanumericsPrintScale = enabled ? 2.0f : 1.0f;
}

// ── Lines ────────────────────────────────────────────────────────────────

void beginLines() { g_lineVertIdx = 0; }
void endLines() { }

void lineVertex(float x, float y, float r, float g, float b, float a)
{
    if (g_lineVertIdx == 0)
    {
        g_lineX0 = x;  g_lineY0 = y;
        g_lineR0 = r;  g_lineG0 = g;  g_lineB0 = b;  g_lineA0 = a;
        g_lineVertIdx = 1;
    }
    else
    {
        addWorldLine(g_lineX0, g_lineY0, x, y, r, g, b, a);
        g_lineVertIdx = 0;
    }
}

// ── Line loop ────────────────────────────────────────────────────────────

void beginLineLoop()
{
    g_loopCount = 0;
    g_loopR = g_curR; g_loopG = g_curG; g_loopB = g_curB; g_loopA = g_curA;
}

void endLineLoop()
{
    if (g_loopCount < 2) return;
    for (int i = 0; i < g_loopCount; i++)
    {
        int j = (i + 1) % g_loopCount;
        addWorldLine(g_loopX[i], g_loopY[i], g_loopX[j], g_loopY[j],
                     g_loopR, g_loopG, g_loopB, g_loopA);
    }
}

void lineLoopVertex(float x, float y)
{
    if (g_loopCount < MAX_LOOP_VERTS)
    {
        g_loopX[g_loopCount] = x;
        g_loopY[g_loopCount] = y;
        g_loopCount++;
    }
}

// ── Line strip ───────────────────────────────────────────────────────────

void beginLineStrip() { g_stripHasFirst = false; }
void endLineStrip() { }

void lineStripVertex(float x, float y)
{
    if (g_stripHasFirst)
        addWorldLine(g_stripLastX, g_stripLastY, x, y, g_curR, g_curG, g_curB, g_curA);
    g_stripLastX = x;
    g_stripLastY = y;
    g_stripHasFirst = true;
}

// ── Points ───────────────────────────────────────────────────────────────

void beginPoints() { }
void endPoints() { }

void pointVertex(float x, float y, float r, float g, float b, float a)
{
    int32_t dvgX, dvgY;
    if (!worldToDvg(x, y, &dvgX, &dvgY)) return;

    uint8_t cr, cg, cb;
    colorToRgb(r, g, b, a, &cr, &cg, &cb);
    g_dvg.addVec(dvgX, dvgY, dvgX, dvgY, cr, cg, cb);
}

// ── Triangle fan (skip on vector display) ────────────────────────────────

void beginTriangleFan() { }
void endTriangleFan()   { }
void triangleFanVertex(float, float) { }

// ── Filled quad (wireframe on vector display) ────────────────────────────

void drawQuad(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              float r, float g, float b, float a)
{
    uint8_t cr, cg, cb;
    colorToRgb(r, g, b, a, &cr, &cg, &cb);
    if (cr == 0 && cg == 0 && cb == 0) return;

    addWorldLine(x1, y1, x2, y2, r, g, b, a);
    addWorldLine(x2, y2, x3, y3, r, g, b, a);
    addWorldLine(x3, y3, x4, y4, r, g, b, a);
    addWorldLine(x4, y4, x1, y1, r, g, b, a);
}

// ── Vertex array grid drawing ────────────────────────────────────────────

void drawGridLines(const void* vertexData, int vertexStride,
                   const void* colorData, int colorStride,
                   const unsigned short* elements, int elementCount)
{
    const uint8_t* verts  = (const uint8_t*)vertexData;
    const uint8_t* colors = (const uint8_t*)colorData;

    for (int i = 0; i < elementCount; i += 2)
    {
        int idx0 = elements[i];
        int idx1 = elements[i + 1];

        const float* v0 = (const float*)(verts + idx0 * vertexStride);
        const float* v1 = (const float*)(verts + idx1 * vertexStride);
        const float* c0 = (const float*)(colors + idx0 * colorStride);
        const float* c1 = (const float*)(colors + idx1 * colorStride);

        float r = (c0[0] + c1[0]) * 0.5f;
        float g = (c0[1] + c1[1]) * 0.5f;
        float b = (c0[2] + c1[2]) * 0.5f;
        float a = (c0[3] + c1[3]) * 0.5f;

        addWorldLine(v0[0], v0[1], v1[0], v1[1], r, g, b, a);
    }
}

} // namespace renderer

#endif // USE_DVG_RENDERER
