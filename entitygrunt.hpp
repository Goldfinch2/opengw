#pragma once

#include "entity.hpp"

class game;

class entityGrunt : public entity
{
  public:
    entityGrunt(const game& gameRef);

    void run() override;
    void spawnTransition() override;

    float mAnimationIndex;

    const game& mGame;
};
