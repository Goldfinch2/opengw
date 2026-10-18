#pragma once

#include <vectorpie/vp.h>

// The game's drawing, on the VectorPie SDK (renderer_vp.cpp): the vector
// board or a window, the same picture.

namespace renderer
{
    // vertical: a vertically mounted monitor (y squeezed into its middle 3/4)
    void init(bool vertical);
    void shutdown();

    // The SDK's output for the frame being drawn (NULL between frames).
    void setOutput(vp_dvg* d);

    // Frame management
    void beginFrame();
    void present();

    // State
    void setColor(float r, float g, float b, float a);
    void setLineWidth(float w);
    void setPointSize(float s);
    void enableLineSmooth(bool enabled);
    void enablePointSmooth(bool enabled);
    void enableBlend();
    void disableBlend();
    void setBlendAdditive();
    void setBlendMultiplicative();

    // Transform
    void translate(float x, float y, float z = 0.0f);

    // Switch to/from 2D screen-space mode (NDC -1..1 mapped directly to display)
    void setScreenSpace(bool enabled);

    // Batched line drawing (GL_LINES equivalent)
    void beginLines();
    void endLines();
    void lineVertex(float x, float y, float r, float g, float b, float a);

    // Line loop (GL_LINE_LOOP equivalent)
    void beginLineLoop();
    void endLineLoop();
    void lineLoopVertex(float x, float y);

    // Line strip (GL_LINE_STRIP equivalent)
    void beginLineStrip();
    void endLineStrip();
    void lineStripVertex(float x, float y);

    // Batched point drawing
    void beginPoints();
    void endPoints();
    void pointVertex(float x, float y, float r, float g, float b, float a);

    // Triangle fan (filled circles: not drawn on vectors)
    void beginTriangleFan();
    void endTriangleFan();
    void triangleFanVertex(float x, float y);

    // Filled quad (its outline)
    void drawQuad(float x1, float y1, float x2, float y2,
                  float x3, float y3, float x4, float y4,
                  float r, float g, float b, float a);

    // Vertex array drawing (for grid)
    void drawGridLines(const void* vertexData, int vertexStride,
                       const void* colorData, int colorStride,
                       const unsigned short* elements, int elementCount);
}
