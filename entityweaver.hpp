#pragma once

#include "entity.hpp"

class entityWeaver : public entity
{
  public:
    entityWeaver();

    void run() override;
    void spawnTransition() override;

    float mAnimationIndex;
};
