#include "hyperspace.hpp"
#include "camera.hpp"
#include "game.hpp"
#include "mathutils.hpp"

#include "renderer.h"

extern Point3d cameraPos;

const float size = 200;

hyperspace::hyperspace(void)
{
    for (int i = 0; i < NUM_HS_STARS; i++) {
        mStars[i].lastPos.x = mStars[i].pos.x = (mathutils::frandFrom0To1() * size) - (size / 2);
        mStars[i].lastPos.y = mStars[i].pos.y = (mathutils::frandFrom0To1() * size) - (size / 2);
        mStars[i].lastPos.z = mStars[i].pos.z = (mathutils::frandFrom0To1() * size) - (size / 2);
        mStars[i].color = vector::pen(mathutils::frandFrom0To1() + .5, mathutils::frandFrom0To1() + .5, mathutils::frandFrom0To1() + .5, 1, 1);
    }

    mCurrentBrightness = mTargetBrightness = 0;
}

hyperspace::~hyperspace(void)
{
}

void hyperspace::run()
{
    for (int i = 0; i < NUM_HS_STARS; i++) {
        mStars[i].lastPos.z = mStars[i].pos.z;
        mStars[i].pos.z += 2 * mCurrentBrightness;
        if (mStars[i].pos.z > 100) {
            mStars[i].lastPos.x = mStars[i].pos.x = (mathutils::frandFrom0To1() * size) - (size / 2);
            mStars[i].lastPos.y = mStars[i].pos.y = (mathutils::frandFrom0To1() * size) - (size / 2);
            mStars[i].lastPos.z = mStars[i].pos.z = (mathutils::frandFrom0To1() * size) - (size / 2);
            mStars[i].color = vector::pen(mathutils::frandFrom0To1() + .5, mathutils::frandFrom0To1() + .5, mathutils::frandFrom0To1() + .5, 1, 1);
        }
    }

    if (mCurrentBrightness < mTargetBrightness) {
        mCurrentBrightness += .005;
    } else if (mCurrentBrightness > mTargetBrightness) {
        mCurrentBrightness *= .99;
    }
}

void hyperspace::draw()
{

    renderer::enablePointSmooth(true);
    renderer::setPointSize(4);
    renderer::setColor(1, 1, 1, 1);

    renderer::beginLines();

    for (int i = 0; i < NUM_HS_STARS; i++) {
        float a = .5 * mCurrentBrightness;

        renderer::lineVertex(mStars[i].lastPos.x + theGame->mCamera->mCurrentPos.x, mStars[i].lastPos.y + theGame->mCamera->mCurrentPos.y, 1, 1, 1, a);
        renderer::lineVertex(mStars[i].pos.x + theGame->mCamera->mCurrentPos.x, mStars[i].pos.y + theGame->mCamera->mCurrentPos.y, 1, 1, 1, a);
    }

    renderer::endLines();
}
