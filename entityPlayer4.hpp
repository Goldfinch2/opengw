#pragma once

#include "player.hpp"

class entityPlayer4 : public player
{
  public:
    entityPlayer4();

    void initPlayerForGame() override;
    void spawnTransition() override;
};
