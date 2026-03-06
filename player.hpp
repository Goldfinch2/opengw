#pragma once

#include "entity.hpp"
#include "entityplayermissile.hpp"

#include <vector>

class player : public entity
{
  public:
    player();
    ~player();

    virtual void initPlayerForGame();
    virtual void deinitPlayerForGame();

    void run() override;
    void spawn() override;
    void spawnTransition() override;
    void destroyTransition() override;
    void destroy() override;
    void indicating() override;

    void draw() override;

    std::vector<entityPlayerMissile> missiles;

    int mScore;
    int mMultiplier;

    int mPlayerAssignment;

    bool mJoined;

    void addKillAtLocation(int points, Point3d pos);

    void runMissiles();

    bool shields() { return mSheildTimer > 0; }

    int getNumLives();
    int getNumBombs();

    void addLife();
    void takeLife();
    void addBomb();
    void takeBomb();
    void switchWeapons();

    void addPointsNoMultiplier(int points);

    vector::pen getExhaustPen() const { return mExhaustPen; }
    vector::pen getMissilesPen() const { return mMissilesPen; }
    vector::pen getFontPen() const { return mFontPen; }

  protected:
    void firePattern1(const Point3d& fireAngle, const Point3d& playerSpeed);
    void firePattern2(const Point3d& fireAngle, const Point3d& playerSpeed);
    void firePattern3(const Point3d& fireAngle, const Point3d& playerSpeed);

    int mCurrentWeapon;

    int mWeaponCounter;
    int mBombCounter;
    int mLifeCounter;

    bool mDrawSheild;

    int mFiringTimer;
    int mSheildTimer;
    int mBombInterimTimer = 0;

    int mKillCounter;
    int mLevelAdvanceCounter;

    int mNumLives;
    int mNumBombs;

    float mExhaustSpreadIndex;

    vector::pen mExhaustPen;
    vector::pen mMissilesPen;
    vector::pen mFontPen;
};

#define PLAYER_SHEILD_TIME 250
