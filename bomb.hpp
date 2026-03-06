#pragma once

#include "point3d.hpp"
#include "vector.hpp"

#include <vector>

class bomb
{
  public:
    struct RING
    {
        Point3d pos {};
        float radius = 0.0f;
        float thickness = 0.0f;
        float speed = 0.0f;
        int timeToLive = 0;
        float fadeStep = 0.0f;
        vector::pen pen {};
    };

    bomb(void);
    ~bomb(void);

    void startBomb(Point3d pos, float radius, float thickness, float speed, int timeToLive, vector::pen pen);

    void run();
    void draw();

    bool isBombing();

    std::vector<RING> mRings;
};
