#pragma once

#include "particle.hpp"
#include "point3d.hpp"
#include <vector>

class attractor
{
  public:
    attractor();

    void clearAll();

    Point3d evaluateParticle(particle::PARTICLE* point);

    struct Attractor
    {
        Point3d pos {};
        float strength = 0.0f;
        float radius = 0.0f;
        bool enabled = false;
        bool attractsParticles = false;
    };

    attractor::Attractor* getAttractor();

    std::vector<Attractor> mAttractors;
};
