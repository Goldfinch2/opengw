#pragma once

#include "entity.hpp"

class game;

class entitySpinner : public entity
{
  public:
    entitySpinner(const game& constRef);

    void run() override;
    void destroyTransition() override;
    void spawnTransition() override;

    void hit(entity* aEntity) override;

    float mAnimationIndex;

  private:
    Point3d mHitSpeed;
    const game& mGame;
};
