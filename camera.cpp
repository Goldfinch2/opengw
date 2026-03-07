#include "camera.hpp"
#include "game.hpp"
#include "grid.hpp"
#include "players.hpp"

static const int zoomedIn = 46;  // 50
static const int zoomedOut = 72; // 66

camera::camera(const game& gameRef) : mGame(gameRef)
{
    center();
    mCurrentPos = mTargetPos;
}

void camera::center()
{
    mCurrentZoom = zoomedIn;
    mTargetZoom = zoomedIn;
    mTargetPos = Point3d((mGame.mGrid->extentX() - 1) / 2, (mGame.mGrid->extentY() - 1) / 2, mTargetZoom);
}

void camera::followPlayer()
{
    player* p = mGame.getPlayer1();
    if (p->getState() == entity::ENTITY_STATE_INACTIVE)
        return;

    Point3d playerPos = p->getPos();

    mTargetZoom = zoomedIn;

    float ax = (playerPos.x / mGame.mGrid->extentX());
    float ay = (playerPos.y / mGame.mGrid->extentY());

    const int border = -20;

    mTargetPos.x = (ax * (mGame.mGrid->extentX() + (border * 2))) - border;
    mTargetPos.y = (ay * (mGame.mGrid->extentY() + (border * 2))) - border;
}

void camera::run()
{
    mCurrentZoom += (mTargetZoom - mCurrentZoom) / 30.0f;

    mCurrentPos.x += (mTargetPos.x - mCurrentPos.x) / 30.0f;
    mCurrentPos.y += (mTargetPos.y - mCurrentPos.y) / 30.0f;

    mCurrentPos.z = mCurrentZoom;
}
