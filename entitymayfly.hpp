#pragma once

#include "entity.hpp"

class entityMayfly : public entity
{
  public:
    entityMayfly();

    void run() override;
    void draw() override;

    int mFlipTimer;
    int mFlipDirection;

    Point3d mTarget;
};
