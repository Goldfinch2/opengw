#pragma once

#include "entity.hpp"

class entityLine : public entity
{
  public:
    entityLine(void);

    void run() override;
    void spawnTransition() override;

    void addEndpoints(const Point3d& from, const Point3d& to);

    int mTimeToLive;
};
