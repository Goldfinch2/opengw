#pragma once

#include "entityspinner.hpp"

class game;

class entityTinySpinner : public entitySpinner
{
  public:
    entityTinySpinner(const game& gameRef);

    void run() override;
    void spawnTransition() override;
    void spawn() override;
    void destroyTransition() override;
    void destroy() override;

    void draw() override;

    entity* hitTest(const Point3d& pos, float radius) override;

    void setInitialSpeed(const Point3d& speed) { mInitialSpeed = speed; }

  private:
    Point3d mVirtualPos { 0, 0, 0 };
    Point3d mInitialSpeed;

    const game& mGame;
};
