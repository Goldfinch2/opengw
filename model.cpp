#include "model.hpp"
#include "renderer.h"

model::model()
{
    mMatrix.Identity();

    mIsLineLoop = false;
}

model::~model()
{
}

void model::draw(const vector::pen& pen)
{
    if (mIsLineLoop) {
        renderer::setColor(pen.r, pen.g, pen.b, pen.a);
        renderer::setLineWidth(pen.lineRadius * .3);

        renderer::beginLineStrip();

        Point3d from = mVertexList[mEdgeList[0].from];
        mMatrix.TransformVertex(from, &from);
        renderer::lineStripVertex(from.x, from.y);

        for (std::size_t i = 1; i < mEdgeList.size(); i++) {
            Point3d to = mVertexList[mEdgeList[i].to];

            mMatrix.TransformVertex(to, &to);

            renderer::lineStripVertex(to.x, to.y);
        }

        renderer::endLineStrip();
    } else {
        renderer::setColor(pen.r, pen.g, pen.b, pen.a);
        renderer::setLineWidth(pen.lineRadius * .3);

        renderer::beginLines();

        for (std::size_t i = 0; i < mEdgeList.size(); i++) {
            Point3d from = mVertexList[mEdgeList[i].from];
            Point3d to = mVertexList[mEdgeList[i].to];

            mMatrix.TransformVertex(from, &from);
            mMatrix.TransformVertex(to, &to);

            renderer::lineVertex(from.x, from.y, pen.r, pen.g, pen.b, pen.a);
            renderer::lineVertex(to.x, to.y, pen.r, pen.g, pen.b, pen.a);
        }

        renderer::endLines();
    }
}

void model::emit(const vector::pen& pen)
{
    for (std::size_t i = 0; i < mEdgeList.size(); i++) {
        Point3d from = mVertexList[mEdgeList[i].from];
        Point3d to = mVertexList[mEdgeList[i].to];

        mMatrix.TransformVertex(from, &from);
        mMatrix.TransformVertex(to, &to);

        renderer::lineVertex(from.x, from.y, pen.r, pen.g, pen.b, pen.a);
        renderer::lineVertex(to.x, to.y, pen.r, pen.g, pen.b, pen.a);
    }
}
