#pragma once

#include "player.hpp"

#include <memory>

class game;

class players
{
  public:
    players(const game& gameRef);
    ~players();

    void run();
    void draw();

    player* getPlayerClosestToPosition(const Point3d& point);
    player* getRandomActivePlayer();

    player* getPlayer1() { return mPlayer1.get(); }

    static std::unique_ptr<player> mPlayer1;

    const game& mGame;
};
