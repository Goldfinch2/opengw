#pragma once

#include "entity.hpp"

#include <vector>

class entitySnakeSegment;

class entitySnake : public entity
{
  public:
    entitySnake();

    void runTransition() override;
    void run() override;
    void spawnTransition() override;
    void spawn() override;
    void destroyTransition() override;
    void destroy() override;
    void indicateTransition() override;

    entity* hitTest(const Point3d& pos, float radius) override;

    void draw() override;

  private:
    Point3d mTarget;
    std::vector<entitySnakeSegment> mSegments;

    void updateTarget();
};
