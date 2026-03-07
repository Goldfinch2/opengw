#pragma once

// Renderer abstraction: compile-time switch between backends
// Default: OpenGL 1.x
// Define USE_DVG_RENDERER for DVG hardware vector output

namespace renderer
{
    // Initialization / shutdown
    void init(int width, int height);
    void shutdown();

    // Frame management
    void beginFrame(int width, int height);
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

    // Triangle fan (for filled circles - black holes)
    void beginTriangleFan();
    void endTriangleFan();
    void triangleFanVertex(float x, float y);

    // Filled quad
    void drawQuad(float x1, float y1, float x2, float y2,
                  float x3, float y3, float x4, float y4,
                  float r, float g, float b, float a);

    // Vertex array drawing (for grid)
    void drawGridLines(const void* vertexData, int vertexStride,
                       const void* colorData, int colorStride,
                       const unsigned short* elements, int elementCount);
}
