#pragma once

#include "entity.hpp"

class entityWanderer : public entity
{
  public:
    entityWanderer();

    void run() override;
    void spawnTransition() override;

    float mCurrentHeading { 0.0f };
    float mAnimationIndex;
    bool mFlipped;
};
