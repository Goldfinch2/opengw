// The game's renderer on the VectorPie SDK, as the old DVG renderer drew it:
// the world seen through a camera (translate) with a 70-degree field of view
// onto the square board, the screen-space text and scores in NDC; on a
// vertically mounted monitor, y squeezed into its middle 3/4. A line's color
// is its color times its alpha (a dim line is a darker color); a line under
// a tenth of full alpha, or black, is not drawn. Filled shapes are their
// outline (a triangle fan: nothing).
//
// The frame's lines are collected, then sent: with sort_vectors (game.ini),
// as the old renderer sorted them, fewer and shorter blank moves: connected
// chains kept whole, binned in a grid over the screen, the cells walked in a
// snake, each cell's chains nearest first.

#include "renderer.h"
#include "font.hpp"
#include "settings.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

struct Seg {
    int x0, y0, x1, y1;
    uint8_t r, g, b;
};

vp_dvg* g_out;
bool    g_vertical;
std::vector<Seg> g_segs, g_sorted;

float g_camX, g_camY, g_camZ;
const float FOV_Y = 70.0f;

int   g_lineVertIdx;
float g_lineX0, g_lineY0;

const int MAX_LOOP_VERTS = 1024;
float g_loopX[MAX_LOOP_VERTS], g_loopY[MAX_LOOP_VERTS];
int   g_loopCount;
float g_loopR, g_loopG, g_loopB, g_loopA;

float g_stripLastX, g_stripLastY;
bool  g_stripHasFirst;

float g_curR = 1, g_curG = 1, g_curB = 1, g_curA = 1;
bool  g_screenSpace;

int toBoard(float n) { return (int)((n + 1.0f) * 0.5f * VP_DVG_COORD_MAX); }

int squeezeY(int y) { return g_vertical ? 512 + (int)(0.75f * y) : y; }

void worldToBoard(float wx, float wy, int* outX, int* outY)
{
    float ex = wx + g_camX, ey = wy + g_camY, ez = g_camZ;
    if (ez >= 0.0f) ez = -0.01f;
    const float visHalf = -ez * std::tan(FOV_Y * 0.5f * 3.14159265f / 180.0f);
    *outX = toBoard(ex / visHalf);
    *outY = squeezeY(toBoard(ey / visHalf));
}

uint8_t level(float c, float a)
{
    return (uint8_t)std::min(std::max(c * a * 255.0f, 0.0f), 255.0f);
}

void addLine(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
    if (a < 0.1f) return;
    Seg s;
    s.r = level(r, a); s.g = level(g, a); s.b = level(b, a);
    if (s.r == 0 && s.g == 0 && s.b == 0) return;
    if (g_screenSpace) {
        s.x0 = toBoard(x0); s.y0 = squeezeY(toBoard(y0));
        s.x1 = toBoard(x1); s.y1 = squeezeY(toBoard(y1));
    } else {
        worldToBoard(x0, y0, &s.x0, &s.y0);
        worldToBoard(x1, y1, &s.x1, &s.y1);
    }
    g_segs.push_back(s);
}

// The old renderer's sort (dvg_utils): chains of connected lines, binned in a
// GRID x GRID grid by their middle, the cells walked in a snake, nearest
// chain first in each.
const int GRID = 8;

void sortSegs()
{
    struct Chain { size_t start, count; int x0, y0, x1, y1; int cell; bool used; };
    std::vector<Chain> chains;
    for (size_t i = 0; i < g_segs.size(); i++) {
        const Seg& s = g_segs[i];
        if (i > 0 && s.x0 == g_segs[i - 1].x1 && s.y0 == g_segs[i - 1].y1) {
            chains.back().count++;
            chains.back().x1 = s.x1;
            chains.back().y1 = s.y1;
        } else {
            chains.push_back({ i, 1, s.x0, s.y0, s.x1, s.y1, 0, false });
        }
    }
    std::vector<std::vector<size_t>> cells(GRID * GRID);
    for (size_t i = 0; i < chains.size(); i++) {
        Chain& c = chains[i];
        int col = std::clamp((c.x0 + c.x1) / 2 * GRID / (VP_DVG_COORD_MAX + 1), 0, GRID - 1);
        int row = std::clamp((c.y0 + c.y1) / 2 * GRID / (VP_DVG_COORD_MAX + 1), 0, GRID - 1);
        cells[row * GRID + col].push_back(i);
    }
    g_sorted.clear();
    int lastX = 0, lastY = 0;
    for (int row = 0; row < GRID; row++) {
        for (int k = 0; k < GRID; k++) {
            const std::vector<size_t>& bin = cells[row * GRID + ((row & 1) ? GRID - 1 - k : k)];
            for (size_t left = bin.size(); left > 0; left--) {
                size_t best = 0;
                int64_t dmin = INT64_MAX;
                for (size_t j = 0; j < bin.size() && dmin > 0; j++) {
                    const Chain& c = chains[bin[j]];
                    if (c.used) continue;
                    int64_t dx = c.x0 - lastX, dy = c.y0 - lastY, d = dx * dx + dy * dy;
                    if (d < dmin) { dmin = d; best = j; }
                }
                Chain& c = chains[bin[best]];
                c.used = true;
                g_sorted.insert(g_sorted.end(), g_segs.begin() + c.start, g_segs.begin() + c.start + c.count);
                lastX = c.x1;
                lastY = c.y1;
            }
        }
    }
    g_segs.swap(g_sorted);
}

}  // namespace

namespace renderer
{

void init(bool vertical) { g_vertical = vertical; }

void shutdown()
{
    g_out = nullptr;
    g_segs.clear();
}

void setOutput(vp_dvg* d) { g_out = d; }

void beginFrame()
{
    g_segs.clear();
    g_camX = g_camY = g_camZ = 0.0f;
}

void present()
{
    if (!g_out) return;
    if (settings::get().sortVectors && g_segs.size() > 1) sortSegs();
    int beamX = -1, beamY = -1;
    int r = -1, g = -1, b = -1;
    for (const Seg& s : g_segs) {
        if (s.r != r || s.g != g || s.b != b) {
            vp_color(g_out, s.r, s.g, s.b);
            r = s.r; g = s.g; b = s.b;
        }
        if (s.x0 != beamX || s.y0 != beamY) vp_moveto(g_out, s.x0, s.y0);
        vp_lineto(g_out, s.x1, s.y1);
        beamX = s.x1;
        beamY = s.y1;
    }
    g_segs.clear();
}

// ── State ────────────────────────────────────────────────────────────────

void setColor(float r, float g, float b, float a)
{
    g_curR = r; g_curG = g; g_curB = b; g_curA = a;
}

void setLineWidth(float) {}
void setPointSize(float) {}
void enableLineSmooth(bool) {}
void enablePointSmooth(bool) {}
void enableBlend() {}
void disableBlend() {}
void setBlendAdditive() {}
void setBlendMultiplicative() {}

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
void endLines() {}

void lineVertex(float x, float y, float r, float g, float b, float a)
{
    if (g_lineVertIdx == 0) {
        g_lineX0 = x;
        g_lineY0 = y;
        g_lineVertIdx = 1;
        return;
    }
    addLine(g_lineX0, g_lineY0, x, y, r, g, b, a);
    g_lineVertIdx = 0;
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
    for (int i = 0; i < g_loopCount; i++) {
        int j = (i + 1) % g_loopCount;
        addLine(g_loopX[i], g_loopY[i], g_loopX[j], g_loopY[j], g_loopR, g_loopG, g_loopB, g_loopA);
    }
}

void lineLoopVertex(float x, float y)
{
    if (g_loopCount < MAX_LOOP_VERTS) {
        g_loopX[g_loopCount] = x;
        g_loopY[g_loopCount] = y;
        g_loopCount++;
    }
}

// ── Line strip ───────────────────────────────────────────────────────────

void beginLineStrip() { g_stripHasFirst = false; }
void endLineStrip() {}

void lineStripVertex(float x, float y)
{
    if (g_stripHasFirst)
        addLine(g_stripLastX, g_stripLastY, x, y, g_curR, g_curG, g_curB, g_curA);
    g_stripLastX = x;
    g_stripLastY = y;
    g_stripHasFirst = true;
}

// ── Points: a dot, one board unit long ──────────────────────────────────

void beginPoints() {}
void endPoints() {}

void pointVertex(float x, float y, float r, float g, float b, float a)
{
    Seg s;
    s.r = level(r, a); s.g = level(g, a); s.b = level(b, a);
    if (s.r == 0 && s.g == 0 && s.b == 0) return;
    if (g_screenSpace) { s.x0 = toBoard(x); s.y0 = squeezeY(toBoard(y)); }
    else worldToBoard(x, y, &s.x0, &s.y0);
    s.x1 = s.x0 + 1;
    s.y1 = s.y0;
    g_segs.push_back(s);
}

// ── Triangle fan: not drawn on vectors ──────────────────────────────────

void beginTriangleFan() {}
void endTriangleFan() {}
void triangleFanVertex(float, float) {}

// ── Filled quad: its outline ────────────────────────────────────────────

void drawQuad(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              float r, float g, float b, float a)
{
    addLine(x1, y1, x2, y2, r, g, b, a);
    addLine(x2, y2, x3, y3, r, g, b, a);
    addLine(x3, y3, x4, y4, r, g, b, a);
    addLine(x4, y4, x1, y1, r, g, b, a);
}

// ── The grid's vertex arrays: each line in its ends' average color ─────

void drawGridLines(const void* vertexData, int vertexStride,
                   const void* colorData, int colorStride,
                   const unsigned short* elements, int elementCount)
{
    const uint8_t* verts = (const uint8_t*)vertexData;
    const uint8_t* colors = (const uint8_t*)colorData;
    for (int i = 0; i + 1 < elementCount; i += 2) {
        const float* v0 = (const float*)(verts + elements[i] * vertexStride);
        const float* v1 = (const float*)(verts + elements[i + 1] * vertexStride);
        const float* c0 = (const float*)(colors + elements[i] * colorStride);
        const float* c1 = (const float*)(colors + elements[i + 1] * colorStride);
        addLine(v0[0], v0[1], v1[0], v1[1], (c0[0] + c1[0]) * 0.5f, (c0[1] + c1[1]) * 0.5f,
                (c0[2] + c1[2]) * 0.5f, (c0[3] + c1[3]) * 0.5f);
    }
}

}  // namespace renderer
