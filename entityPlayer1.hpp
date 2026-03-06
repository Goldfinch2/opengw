#pragma once

#include "player.hpp"

class game;

class entityPlayer1 : public player
{
  public:
    entityPlayer1(const game& gameRef);

    void initPlayerForGame() override;
    void spawnTransition() override;

    const game& mGame;
};
