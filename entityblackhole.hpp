#pragma once

#include "entity.hpp"

class game;

class entityBlackHole : public entity
{
  public:
    entityBlackHole(const game& gameRef);

    void runTransition() override;
    void run() override;
    void spawnTransition() override;
    void destroyTransition() override;
    void indicateTransition() override;

    void hit(entity* aEntity) override;
    entity* hitTest(const Point3d& pos, float radius) override;

    void draw() override;

    void feed(int points);

    float getRadius() const;

    bool mActivated;
    float mAnimationIndex;
    float mAnimationSpeed;
    float mAnimationDepth;

    float mAttractorStrengthIndex;

    float mBalance;
    float mBalanceRate;

    float mStrength;

    int mFeedCount { 0 };

    int mPoints { 0 };

  private:
    void drawRing();

    float mHumSpeed { 0.0f };
    float mHumSpeedTarget { 0.0f };
    int mHumLoopSoundId;
    float mGridPullIndex;

    const game& mGame;
};
