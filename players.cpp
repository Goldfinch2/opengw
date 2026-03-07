#include "players.hpp"
#include "debuglog.h"
#include "enemies.hpp"
#include "entityPlayer1.hpp"
#include "game.hpp"

#include <cstdio>

std::unique_ptr<player> players::mPlayer1;

players::players(const game& gameRef) : mGame(gameRef)
{
    mPlayer1 = std::make_unique<entityPlayer1>(gameRef);
    mPlayer1->setState(entity::ENTITY_STATE_INACTIVE);
    mPlayer1->mJoined = false;
}

players::~players()
{
}

void players::run()
{
    player* currentPlayer = mPlayer1.get();

    currentPlayer->runMissiles();

    switch (currentPlayer->getState()) {
    case entity::ENTITY_STATE_INACTIVE:
        return;
    case entity::ENTITY_STATE_SPAWN_TRANSITION:
        currentPlayer->spawnTransition();
        break;
    case entity::ENTITY_STATE_SPAWNING:
        currentPlayer->spawn();
        break;
    case entity::ENTITY_STATE_RUN_TRANSITION:
        currentPlayer->runTransition();
        break;
    case entity::ENTITY_STATE_RUNNING:
        currentPlayer->run();

        // Hit test the player against the enemies
        if (1) // Change to 0 to cheat for testing :-)
        {
            entity* enemy = theGame->mEnemies->hitTestEnemiesAtPosition(currentPlayer->getPos(), currentPlayer->getRadius() * .75);
            if (enemy) {
                enemy = enemy->getParent();

                if (currentPlayer->shields()) {
                    // We hit an enemy with shields still on
                    enemy->hit(currentPlayer);
                } else {
                    // We hit an enemy

                    // Destroy the player
                    currentPlayer->setState(entity::ENTITY_STATE_DESTROY_TRANSITION);

                    // Set the enemy we hit to "indicate"
                    enemy->setState(entity::ENTITY_STATE_INDICATE_TRANSITION);
                    enemy->incGenId();

                    // Destroy all the other enemies
                    for (int i = 0; i < NUM_ENEMIES; i++) {
                        if ((theGame->mEnemies->mEnemies[i]->getState() != entity::ENTITY_STATE_INACTIVE) && (theGame->mEnemies->mEnemies[i] != enemy)) {
                            theGame->mEnemies->mEnemies[i]->hit(nullptr);
                            theGame->mEnemies->mEnemies[i]->incGenId();
                        }
                    }
                }
            }
        }

        break;
    case entity::ENTITY_STATE_DESTROY_TRANSITION:
        currentPlayer->destroyTransition();
        break;
    case entity::ENTITY_STATE_DESTROYED:
        currentPlayer->destroy();
        break;
    default:
        debuglog::log(true,"Unhandled player state\n");
        break;
    }
}

void players::draw()
{
    mPlayer1->draw();
}

player* players::getPlayerClosestToPosition(const Point3d& point)
{
    (void)point;
    return mPlayer1.get();
}

player* players::getRandomActivePlayer()
{
    if (mPlayer1->mJoined && mPlayer1->getState() == entity::ENTITY_STATE_RUNNING)
        return mPlayer1.get();
    return nullptr;
}
