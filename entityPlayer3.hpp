#pragma once

#include "player.hpp"

class entityPlayer3 : public player
{
  public:
    entityPlayer3();

    void initPlayerForGame() override;
    void spawnTransition() override;
};
