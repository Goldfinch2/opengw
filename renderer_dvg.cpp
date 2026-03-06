#ifdef USE_DVG_RENDERER

// DVG (Digital Vector Generator) backend for the renderer abstraction.
// Outputs real vectors to a vector CRT via DVG hardware connected as a
// CDC serial port.

#include "renderer.h"
#include "renderer_dvg.h"
#include "debuglog.h"
#include "dvg_serial.h"
#include "settings.hpp"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <algorithm>

// ─── Internal types ──────────────────────────────────────────────────────────

struct dvg_vec_t
{
    int32_t x0, y0, x1, y1;
    uint8_t r, g, b;
};

// ─── Static state ────────────────────────────────────────────────────────────

// Command buffer (sent to DVG each frame)
static uint8_t*  g_cmdBuf  = nullptr;
static uint32_t  g_cmdOffs = 0;

// Input vector list (accumulated during the frame)
static dvg_vec_t* g_vecList = nullptr;
static uint32_t   g_vecCnt  = 0;

// Last sent RGB (for change-detection optimisation)
static int32_t g_lastR = -1, g_lastG = -1, g_lastB = -1;

// Beam position persists across frames for blank move optimisation
static int32_t g_beamEndX = 0, g_beamEndY = 0;

// Display orientation (queried from DVG hardware)
static bool g_verticalDisplay = false;

// Screen dimensions
static int g_screenWidth  = 0;
static int g_screenHeight = 0;

// Camera / projection state
static float g_camX = 0.0f, g_camY = 0.0f, g_camZ = 0.0f;
static float g_fovY = 70.0f; // degrees, matches scene.cpp gluPerspective

// Line-vertex pair accumulator (for GL_LINES emulation)
static int   g_lineVertIdx = 0;
static float g_lineX0, g_lineY0;
static float g_lineR0, g_lineG0, g_lineB0, g_lineA0;

// Line loop accumulator
#define MAX_LOOP_VERTS 1024
static float g_loopX[MAX_LOOP_VERTS];
static float g_loopY[MAX_LOOP_VERTS];
static int   g_loopCount = 0;
static float g_loopR, g_loopG, g_loopB, g_loopA; // set by setColor before beginLineLoop

// Line strip accumulator
static float g_stripLastX, g_stripLastY;
static bool  g_stripHasFirst = false;

// Current color (set by setColor, used by lineLoop/lineStrip/triangleFan)
static float g_curR = 1.0f, g_curG = 1.0f, g_curB = 1.0f, g_curA = 1.0f;

static uint32_t g_targetFps = 40;

// ─── Helpers ─────────────────────────────────────────────────────────────────

//
// Transform world coordinates to DVG coordinates (0..4095).
// Applies the camera translation and perspective projection.
//
static bool worldToDvg(float wx, float wy, int32_t* outX, int32_t* outY)
{
    // Apply camera translation (world → eye space)
    float ex = wx + g_camX; // g_camX is already negated by translate()
    float ey = wy + g_camY;
    float ez = g_camZ;      // camera distance (negative, e.g. -46)

    // Perspective projection: project from eye space to NDC
    // FOV = 70 degrees, so half-angle = 35 degrees
    // At distance |ez|, visible half-height = |ez| * tan(35°)
    if (ez >= 0.0f) ez = -0.01f; // safety: camera must be behind objects

    float halfFovRad = (g_fovY * 0.5f) * (3.14159265f / 180.0f);
    float tanHalf = tanf(halfFovRad);
    float visHalfH = -ez * tanHalf;
    // DVG display is square — use 1:1 aspect for correct proportions
    float visHalfW = visHalfH;

    // NDC: -1..1
    float nx = ex / visHalfW;
    float ny = ey / visHalfH;

    // Clip to visible area
    if (nx < -1.2f || nx > 1.2f || ny < -1.2f || ny > 1.2f)
        return false;

    // Map NDC to DVG (0..4095)
    *outX = (int32_t)((nx + 1.0f) * 0.5f * DVG_RES_MAX);
    *outY = (int32_t)((ny + 1.0f) * 0.5f * DVG_RES_MAX);

    if (g_verticalDisplay)
    {
        int32_t tmp = *outX;
        *outX = DVG_RES_MAX - *outY;
        *outY = tmp;
        *outY = 512 + (int32_t)(0.75f * *outY);
    }

    return true;
}

//
// Cohen-Sutherland line clipping to DVG coordinate range (0..4095).
//
enum { CS_INSIDE=0, CS_LEFT=1, CS_RIGHT=2, CS_BOTTOM=4, CS_TOP=8 };

static int csCode(int32_t x, int32_t y)
{
    int code = CS_INSIDE;
    if (x < (int32_t)DVG_RES_MIN) code |= CS_LEFT;
    else if (x > (int32_t)DVG_RES_MAX) code |= CS_RIGHT;
    if (y < (int32_t)DVG_RES_MIN) code |= CS_BOTTOM;
    else if (y > (int32_t)DVG_RES_MAX) code |= CS_TOP;
    return code;
}

static bool clipLine(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1)
{
    int code0 = csCode(*x0, *y0);
    int code1 = csCode(*x1, *y1);

    while (true)
    {
        if (!(code0 | code1))
            return true;
        if (code0 & code1)
            return false;

        int code = code0 ? code0 : code1;
        int32_t x, y;

        if (code & CS_TOP) {
            x = *x0 + (int32_t)((int64_t)(*x1 - *x0) * (DVG_RES_MAX - *y0) / (*y1 - *y0));
            y = DVG_RES_MAX;
        } else if (code & CS_BOTTOM) {
            x = *x0 + (int32_t)((int64_t)(*x1 - *x0) * (DVG_RES_MIN - *y0) / (*y1 - *y0));
            y = DVG_RES_MIN;
        } else if (code & CS_RIGHT) {
            y = *y0 + (int32_t)((int64_t)(*y1 - *y0) * (DVG_RES_MAX - *x0) / (*x1 - *x0));
            x = DVG_RES_MAX;
        } else {
            y = *y0 + (int32_t)((int64_t)(*y1 - *y0) * (DVG_RES_MIN - *x0) / (*x1 - *x0));
            x = DVG_RES_MIN;
        }

        if (code == code0) {
            *x0 = x; *y0 = y;
            code0 = csCode(*x0, *y0);
        } else {
            *x1 = x; *y1 = y;
            code1 = csCode(*x1, *y1);
        }
    }
}

//
// Convert float RGBA (0..1) to uint8 RGB, pre-multiplied by alpha.
//
static void colorToRgb(float r, float g, float b, float a,
                        uint8_t* outR, uint8_t* outG, uint8_t* outB)
{
    *outR = (uint8_t)std::min(std::max(r * a * 255.0f, 0.0f), 255.0f);
    *outG = (uint8_t)std::min(std::max(g * a * 255.0f, 0.0f), 255.0f);
    *outB = (uint8_t)std::min(std::max(b * a * 255.0f, 0.0f), 255.0f);
}

//
// Append a 4-byte big-endian command word to the command buffer.
//
static void cmdAppend(uint32_t cmd)
{
    if (g_cmdOffs <= (DVG_CMD_BUF_SIZE - 4))
    {
        g_cmdBuf[g_cmdOffs++] = (uint8_t)(cmd >> 24);
        g_cmdBuf[g_cmdOffs++] = (uint8_t)(cmd >> 16);
        g_cmdBuf[g_cmdOffs++] = (uint8_t)(cmd >>  8);
        g_cmdBuf[g_cmdOffs++] = (uint8_t)(cmd >>  0);
    }
}

//
// Emit DVG draw-to command for one point.
//
static void cmdAddPoint(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t blank = (r == 0) && (g == 0) && (b == 0);

    if (!blank)
    {
        if (r != g_lastR || g != g_lastG || b != g_lastB)
        {
            g_lastR = r;
            g_lastG = g;
            g_lastB = b;
            cmdAppend((DVG_FLAG_RGB << 29) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff));
        }
    }
    cmdAppend((DVG_FLAG_XY << 29) | ((blank & 0x1) << 28) | ((x & 0x3fff) << 14) | (y & 0x3fff));
}

//
// Add a vector to the input list.
//
static void addVec(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   uint8_t r, uint8_t g, uint8_t b)
{
    if (r == 0 && g == 0 && b == 0 && x0 == x1 && y0 == y1)
        return;

    if (!clipLine(&x0, &y0, &x1, &y1))
        return;

    if (g_vecCnt < DVG_MAX_VECTORS)
    {
        g_vecList[g_vecCnt].x0 = x0;
        g_vecList[g_vecCnt].y0 = y0;
        g_vecList[g_vecCnt].x1 = x1;
        g_vecList[g_vecCnt].y1 = y1;
        g_vecList[g_vecCnt].r  = r;
        g_vecList[g_vecCnt].g  = g;
        g_vecList[g_vecCnt].b  = b;
        g_vecCnt++;
    }
}

//
// Add a line from world coordinates with per-vertex color.
// Uses the second vertex's color (matches GL behavior).
//
static void addWorldLine(float x0, float y0, float x1, float y1,
                          float r, float g, float b, float a)
{
    // Skip nearly-invisible lines (e.g. spawn animation fading copies)
    if (a < 0.1f) return;

    int32_t dx0, dy0, dx1, dy1;
    if (!worldToDvg(x0, y0, &dx0, &dy0)) return;
    if (!worldToDvg(x1, y1, &dx1, &dy1)) return;

    uint8_t cr, cg, cb;
    colorToRgb(r, g, b, a, &cr, &cg, &cb);
    addVec(dx0, dy0, dx1, dy1, cr, cg, cb);
}

// ─── DVG info query ──────────────────────────────────────────────────────────

static void getDvgInfo()
{
    uint32_t cmd = (DVG_FLAG_CMD << 29) | DVG_FLAG_CMD_GET_DVG_INFO;
    uint8_t cmd_buf[4];
    cmd_buf[0] = (uint8_t)(cmd >> 24);
    cmd_buf[1] = (uint8_t)(cmd >> 16);
    cmd_buf[2] = (uint8_t)(cmd >>  8);
    cmd_buf[3] = (uint8_t)(cmd >>  0);

    if (dvg_serial::write(cmd_buf, 4) != 4)
    {
        debuglog::log(true,"getDvgInfo: write failed\n");
        return;
    }

    uint32_t ack;
    int ackRead = dvg_serial::read(&ack, 4);
    if (ackRead != 4)
    {
        debuglog::log(true,"getDvgInfo: ack read failed (got %d bytes)\n", ackRead);
        return;
    }

    uint32_t jsonLen;
    int lenRead = dvg_serial::read(&jsonLen, 4);
    if (lenRead != 4)
    {
        debuglog::log(true,"getDvgInfo: length read failed (got %d bytes)\n", lenRead);
        return;
    }
    if (jsonLen == 0 || jsonLen >= DVG_MAX_JSON_SIZE)
    {
        debuglog::log(true,"getDvgInfo: bad length %u\n", jsonLen);
        return;
    }

    char jsonBuf[DVG_MAX_JSON_SIZE];
    int jsonRead = dvg_serial::read(jsonBuf, jsonLen);
    if (jsonRead != (int)jsonLen)
    {
        debuglog::log(true,"getDvgInfo: payload read failed (got %d of %u bytes)\n", jsonRead, jsonLen);
        return;
    }
    jsonBuf[jsonLen] = '\0';

    const char* p = strstr(jsonBuf, "\"vertical\"");
    if (p)
    {
        p += strlen("\"vertical\"");
        while (*p == ' ' || *p == ':') p++;
        g_verticalDisplay = (strncmp(p, "true", 4) == 0);
    }

    debuglog::log(true,"DVG JSON: %s\n", jsonBuf);
    debuglog::log(true,"DVG orientation: vertical=%d\n", g_verticalDisplay);
}

// ─── Serial I/O ──────────────────────────────────────────────────────────────

static int serialWrite(uint8_t* buf, int size)
{
    int total = size;
    while (size > 0)
    {
        int chunk = std::min(size, 512);
        int written = dvg_serial::write(buf, chunk);
        if (written != chunk)
            return -1;
        buf  += chunk;
        size -= chunk;
    }
    return total;
}

// ─── Vector sorting (chain-based grid sort from bzone2) ─────────────────────

#define GRID_COLS 8
#define GRID_ROWS 8
#define GRID_NUM_CELLS (GRID_COLS * GRID_ROWS)

struct dvg_chain_t
{
    uint16_t start;
    uint16_t count;
    int32_t  x0, y0;
    int32_t  x1, y1;
};

static dvg_chain_t g_chains[DVG_MAX_VECTORS];
static uint16_t    g_chainCnt;
static dvg_vec_t   g_sortTemp[DVG_MAX_VECTORS];
static uint16_t    g_cellCnt[GRID_NUM_CELLS];
static uint16_t    g_cellOffset[GRID_NUM_CELLS];
static uint16_t    g_cellBins[DVG_MAX_VECTORS];
static uint8_t     g_sortUsed[DVG_MAX_VECTORS];

static void sortVectors()
{
    if (g_vecCnt <= 1)
        return;

    // Pass 1: detect connected chains
    g_chainCnt = 0;
    g_chains[0].start = 0;
    g_chains[0].count = 1;
    g_chains[0].x0 = g_vecList[0].x0;
    g_chains[0].y0 = g_vecList[0].y0;

    for (uint32_t i = 1; i < g_vecCnt; i++)
    {
        if (g_vecList[i].x0 == g_vecList[i - 1].x1 &&
            g_vecList[i].y0 == g_vecList[i - 1].y1)
        {
            g_chains[g_chainCnt].count++;
        }
        else
        {
            g_chains[g_chainCnt].x1 = g_vecList[i - 1].x1;
            g_chains[g_chainCnt].y1 = g_vecList[i - 1].y1;
            g_chainCnt++;
            g_chains[g_chainCnt].start = (uint16_t)i;
            g_chains[g_chainCnt].count = 1;
            g_chains[g_chainCnt].x0 = g_vecList[i].x0;
            g_chains[g_chainCnt].y0 = g_vecList[i].y0;
        }
    }
    g_chains[g_chainCnt].x1 = g_vecList[g_vecCnt - 1].x1;
    g_chains[g_chainCnt].y1 = g_vecList[g_vecCnt - 1].y1;
    g_chainCnt++;

    // Pass 2: assign each chain to a grid cell
    memset(g_cellCnt, 0, sizeof(g_cellCnt));
    for (uint32_t i = 0; i < g_chainCnt; i++)
    {
        int32_t mx = (g_chains[i].x0 + g_chains[i].x1) / 2;
        int32_t my = (g_chains[i].y0 + g_chains[i].y1) / 2;
        int col = mx * GRID_COLS / (DVG_RES_MAX + 1);
        int row = my * GRID_ROWS / (DVG_RES_MAX + 1);
        if (col < 0) col = 0;
        if (col >= GRID_COLS) col = GRID_COLS - 1;
        if (row < 0) row = 0;
        if (row >= GRID_ROWS) row = GRID_ROWS - 1;
        g_sortUsed[i] = (uint8_t)(row * GRID_COLS + col);
        g_cellCnt[row * GRID_COLS + col]++;
    }

    g_cellOffset[0] = 0;
    for (int i = 1; i < GRID_NUM_CELLS; i++)
        g_cellOffset[i] = g_cellOffset[i - 1] + g_cellCnt[i - 1];

    memset(g_cellCnt, 0, sizeof(g_cellCnt));
    for (uint32_t i = 0; i < g_chainCnt; i++)
    {
        uint8_t c = g_sortUsed[i];
        g_cellBins[g_cellOffset[c] + g_cellCnt[c]++] = (uint16_t)i;
    }

    memset(g_sortUsed, 0, g_chainCnt);

    // Pass 3: traverse grid in snake pattern, nearest-neighbor on chains
    uint32_t outCnt = 0;
    int32_t lastX = 0, lastY = 0;

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int i = 0; i < GRID_COLS; i++)
        {
            int col = (row & 1) ? (GRID_COLS - 1 - i) : i;
            int cell = row * GRID_COLS + col;
            uint16_t cnt = g_cellCnt[cell];
            if (cnt == 0) continue;
            uint16_t* bin = &g_cellBins[g_cellOffset[cell]];

            uint16_t remaining = cnt;
            while (remaining > 0)
            {
                int64_t dmin = INT64_MAX;
                uint16_t bestIdx = 0;

                for (uint16_t j = 0; j < cnt; j++)
                {
                    if (g_sortUsed[bin[j]]) continue;
                    const dvg_chain_t& ch = g_chains[bin[j]];
                    int64_t dx0 = (int64_t)(ch.x0 - lastX);
                    int64_t dy0 = (int64_t)(ch.y0 - lastY);
                    int64_t d0 = dx0 * dx0 + dy0 * dy0;
                    if (d0 < dmin) { bestIdx = j; dmin = d0; }
                    if (dmin == 0) break;
                }

                g_sortUsed[bin[bestIdx]] = true;
                remaining--;
                const dvg_chain_t& ch = g_chains[bin[bestIdx]];

                for (uint32_t k = ch.start; k < (uint32_t)(ch.start + ch.count); k++)
                {
                    g_sortTemp[outCnt] = g_vecList[k];
                    outCnt++;
                }
                lastX = ch.x1;
                lastY = ch.y1;
            }
        }
    }

    memcpy(g_vecList, g_sortTemp, outCnt * sizeof(dvg_vec_t));
    g_vecCnt = outCnt;
}

//
// Build frame command buffer and send to DVG.
//
static void buildFrameCommands()
{
    g_cmdOffs = 0;
    g_lastR = g_lastG = g_lastB = -1;

    int32_t beamX = g_beamEndX, beamY = g_beamEndY;

    for (uint32_t i = 0; i < g_vecCnt; i++)
    {
        int32_t x0 = g_vecList[i].x0, y0 = g_vecList[i].y0;
        int32_t x1 = g_vecList[i].x1, y1 = g_vecList[i].y1;

        if (beamX != x0 || beamY != y0)
            cmdAddPoint(x0, y0, 0, 0, 0);

        cmdAddPoint(x1, y1, g_vecList[i].r, g_vecList[i].g, g_vecList[i].b);
        beamX = x1;
        beamY = y1;
    }
    g_beamEndX = beamX;
    g_beamEndY = beamY;

    cmdAppend(DVG_FLAG_COMPLETE << 29);
}

static void serialSend()
{
    const settings& s = settings::get();

    if (s.sortVectors)
        sortVectors();

    buildFrameCommands();
    serialWrite(g_cmdBuf, g_cmdOffs);
}

// ─── Renderer interface ──────────────────────────────────────────────────────

namespace renderer
{

void init(int width, int height)
{
    g_screenWidth  = width;
    g_screenHeight = height;

    g_cmdBuf  = (uint8_t*)malloc(DVG_CMD_BUF_SIZE);
    g_vecList = (dvg_vec_t*)malloc(DVG_MAX_VECTORS * sizeof(dvg_vec_t));

    const settings& s = settings::get();

    if (!s.serialPort[0])
    {
        debuglog::log(true,"DVG renderer: no serial_port configured in settings.txt\n");
        debuglog::log(true,"DVG renderer: running without hardware (vector count will be logged)\n");
        return;
    }

    if (!dvg_serial::open(s.serialPort))
    {
        debuglog::log(true,"DVG renderer: failed to open %s\n", s.serialPort);
        return;
    }

    getDvgInfo();

    int cfgFps = s.dvgRefreshRate;
    if (cfgFps < 5)  cfgFps = 5;
    if (cfgFps > 60) cfgFps = 60;
    g_targetFps = (uint32_t)cfgFps;

    debuglog::log(true,"DVG renderer initialized — %u fps\n", g_targetFps);
}

void shutdown()
{
    if (dvg_serial::isOpen())
    {
        dvg_serial::flush();
        SDL_Delay(100);

        g_cmdOffs = 0;
        for (int i = 0; i < 16; i++)
            cmdAppend(DVG_FLAG_COMPLETE << 29);
        cmdAppend(DVG_FLAG_EXIT << 29);
        serialWrite(g_cmdBuf, g_cmdOffs);
        dvg_serial::flush();

        SDL_Delay(50);
        dvg_serial::close();
    }

    free(g_cmdBuf);  g_cmdBuf  = nullptr;
    free(g_vecList); g_vecList = nullptr;

    debuglog::log(true,"DVG renderer shutdown\n");
}

void beginFrame(int width, int height)
{
    g_screenWidth  = width;
    g_screenHeight = height;
    g_cmdOffs = 0;
    g_vecCnt  = 0;
    g_lastR = g_lastG = g_lastB = -1;
    g_camX = g_camY = g_camZ = 0.0f;
}

void present()
{
    static uint32_t frameCount = 0;
    if (++frameCount % 120 == 0)
        debuglog::log(true,"DVG: %u vectors this frame\n", g_vecCnt);

    if (dvg_serial::isOpen() && g_vecCnt > 0)
        serialSend();

    g_cmdOffs = 0;
    g_vecCnt  = 0;
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

// ── Lines ────────────────────────────────────────────────────────────────

void beginLines()
{
    g_lineVertIdx = 0;
}

void endLines()
{
}

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

void beginLineStrip()
{
    g_stripHasFirst = false;
}

void endLineStrip()
{
}

void lineStripVertex(float x, float y)
{
    if (g_stripHasFirst)
    {
        addWorldLine(g_stripLastX, g_stripLastY, x, y,
                     g_curR, g_curG, g_curB, g_curA);
    }
    g_stripLastX = x;
    g_stripLastY = y;
    g_stripHasFirst = true;
}

// ── Points ───────────────────────────────────────────────────────────────

void beginPoints()
{
}

void endPoints()
{
}

void pointVertex(float x, float y, float r, float g, float b, float a)
{
    int32_t dvgX, dvgY;
    if (!worldToDvg(x, y, &dvgX, &dvgY)) return;

    uint8_t cr, cg, cb;
    colorToRgb(r, g, b, a, &cr, &cg, &cb);

    // Zero-length vector = dot on CRT
    addVec(dvgX, dvgY, dvgX, dvgY, cr, cg, cb);
}

// ── Triangle fan (black hole fill — skip on vector display) ──────────────

void beginTriangleFan() { }
void endTriangleFan()   { }
void triangleFanVertex(float, float) { }

// ── Filled quad (wireframe outline on vector display) ────────────────────

void drawQuad(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              float r, float g, float b, float a)
{
    uint8_t cr, cg, cb;
    colorToRgb(r, g, b, a, &cr, &cg, &cb);
    if (cr == 0 && cg == 0 && cb == 0)
        return;

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

        // Use average color of the two endpoints
        float r = (c0[0] + c1[0]) * 0.5f;
        float g = (c0[1] + c1[1]) * 0.5f;
        float b = (c0[2] + c1[2]) * 0.5f;
        float a = (c0[3] + c1[3]) * 0.5f;

        addWorldLine(v0[0], v0[1], v1[0], v1[1], r, g, b, a);
    }
}

} // namespace renderer

#endif // USE_DVG_RENDERER
