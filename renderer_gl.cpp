#if !defined(USE_DVG_RENDERER)

#include "renderer.h"

#include <SDL3/SDL_opengl.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

namespace renderer
{

void init(int /*width*/, int /*height*/)
{
}

void shutdown()
{
}

void beginFrame(int width, int height)
{
    (void)width;
    (void)height;
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void present()
{
}

void setColor(float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
}

void setLineWidth(float w)
{
    glLineWidth(w);
}

void setPointSize(float s)
{
    glPointSize(s);
}

void enableLineSmooth(bool enabled)
{
    if (enabled) {
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
        glDisable(GL_LINE_SMOOTH);
    }
}

void enablePointSmooth(bool enabled)
{
    if (enabled) {
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
        glDisable(GL_POINT_SMOOTH);
    }
}

void enableBlend()
{
    glEnable(GL_BLEND);
}

void disableBlend()
{
    glDisable(GL_BLEND);
}

void setBlendAdditive()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void setBlendMultiplicative()
{
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
}

void translate(float x, float y, float z)
{
    glTranslatef(x, y, z);
}

void setScreenSpace(bool /*enabled*/)
{
    // GL build uses glEnable2D/glDisable2D for 2D mode
}

// Lines
void beginLines()
{
    glBegin(GL_LINES);
}

void endLines()
{
    glEnd();
}

void lineVertex(float x, float y, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glVertex3f(x, y, 0.0f);
}

// Line loop
void beginLineLoop()
{
    glBegin(GL_LINE_LOOP);
}

void endLineLoop()
{
    glEnd();
}

void lineLoopVertex(float x, float y)
{
    glVertex3f(x, y, 0.0f);
}

// Line strip
void beginLineStrip()
{
    glBegin(GL_LINE_STRIP);
}

void endLineStrip()
{
    glEnd();
}

void lineStripVertex(float x, float y)
{
    glVertex3f(x, y, 0.0f);
}

// Points
void beginPoints()
{
    glBegin(GL_POINTS);
}

void endPoints()
{
    glEnd();
}

void pointVertex(float x, float y, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glVertex3f(x, y, 0.0f);
}

// Triangle fan
void beginTriangleFan()
{
    glBegin(GL_TRIANGLE_FAN);
}

void endTriangleFan()
{
    glEnd();
}

void triangleFanVertex(float x, float y)
{
    glVertex3f(x, y, 0.0f);
}

// Filled quad
void drawQuad(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex3f(x1, y1, 0.0f);
    glVertex3f(x2, y2, 0.0f);
    glVertex3f(x3, y3, 0.0f);
    glVertex3f(x4, y4, 0.0f);
    glEnd();
}

// Vertex array grid drawing
void drawGridLines(const void* vertexData, int vertexStride,
                   const void* colorData, int colorStride,
                   const unsigned short* elements, int elementCount)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_FLOAT, vertexStride, vertexData);
    glColorPointer(4, GL_FLOAT, colorStride, colorData);
    glDrawElements(GL_LINES, elementCount, GL_UNSIGNED_SHORT, elements);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

} // namespace renderer

#endif // !USE_DVG_RENDERER
