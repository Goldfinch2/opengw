#pragma once

#include "entity.hpp"

#include <memory>

class entityRepulsorShieldLine;
class game;

class entityRepulsor : public entity
{
  public:
    entityRepulsor(const game& gameRef);

    void run() override;
    void spawnTransition() override;
    void destroyTransition() override;
    void indicateTransition() override;
    void draw() override;

    void repelEntity(entity* e);

    typedef enum
    {
        State_Thinking = 0,
        State_Aiming,
        State_Charging
    } State;

    bool mShieldsEnabled;

  protected:
    State mAIState;
    int mTimer;
    int mLoopSoundId;

    std::unique_ptr<entityRepulsorShieldLine> mShield1;
    std::unique_ptr<entityRepulsorShieldLine> mShield2;
    std::unique_ptr<entityRepulsorShieldLine> mShield3;

    const game& mGame;
};
