#pragma once

#include "point3d.hpp"

#include <vector>

class game;

class stars
{
  public:
    stars(const game& gameRef);
    ~stars();

    void run();
    void draw();

    struct STAR
    {
        Point3d pos {};
        float radius = 0.0f;
        float brightness = 0.0f;
        float twinkle = 0.0f;
    };

    std::vector<STAR> mStars;
    const game& mGame;
};
