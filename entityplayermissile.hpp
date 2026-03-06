#pragma once

#include "entity.hpp"

class entityPlayerMissile : public entity
{
  public:
    entityPlayerMissile();

    void run() override;
    void spawnTransition() override;
    void destroyTransition() override;

    void draw() override;

    int mType; // TODO: entity class also has mType
    int mPlayerSource;
    float mVelocity { 0.0f };

  private:
    Point3d mLastPos;
};
