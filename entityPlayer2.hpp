#pragma once

#include "player.hpp"

class entityPlayer2 : public player
{
  public:
    entityPlayer2();

    void initPlayerForGame() override;
    void spawnTransition() override;
};
