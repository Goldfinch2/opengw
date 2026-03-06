#include "defines.hpp"
#include "font.hpp"
#include "game.hpp"
#include "highscore.hpp"
#include "menuSelectGameType.hpp"
#include "players.hpp"
#include "scene.hpp"
#include "settings.hpp"

#if !defined(USE_DVG_RENDERER)
#include <SDL3/SDL_opengl.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#endif

#include <cstdio>
#include <memory>

std::unique_ptr<game> theGame;

int scene::mPass;
bool scene::mEnableGridGlow = true;

scene g_scene;

#define VIRTUAL_SCREEN_WIDTH  800
#define VIRTUAL_SCREEN_HEIGHT 600

vector::pen defaultFontPen(0, 1, 0, .6, 3);

scene::scene()
{
    theGame = std::make_unique<game>();

    mAttractModeTimer = 0;
    mShowHighScores = false;

    // Create the model for the shield symbol

    int i = 0;

    mShieldSymbol.mVertexList.resize(12);
    mShieldSymbol.mVertexList[i++] = Point3d(0, 11.5);
    mShieldSymbol.mVertexList[i++] = Point3d(8.1, 8.1);
    mShieldSymbol.mVertexList[i++] = Point3d(11.5, 0);
    mShieldSymbol.mVertexList[i++] = Point3d(8.1, -8.1);
    mShieldSymbol.mVertexList[i++] = Point3d(0, -11.5);
    mShieldSymbol.mVertexList[i++] = Point3d(-8.1, -8.1);
    mShieldSymbol.mVertexList[i++] = Point3d(-11.5, 0);
    mShieldSymbol.mVertexList[i++] = Point3d(-8.1, 8.1);
    mShieldSymbol.mVertexList[i++] = Point3d(0, 4);
    mShieldSymbol.mVertexList[i++] = Point3d(0, -4);
    mShieldSymbol.mVertexList[i++] = Point3d(-4, 0);
    mShieldSymbol.mVertexList[i++] = Point3d(4, 0);

    i = 0;

    mShieldSymbol.mEdgeList.resize(10);
    mShieldSymbol.mEdgeList[i].from = 0;
    mShieldSymbol.mEdgeList[i++].to = 1;
    mShieldSymbol.mEdgeList[i].from = 1;
    mShieldSymbol.mEdgeList[i++].to = 2;
    mShieldSymbol.mEdgeList[i].from = 2;
    mShieldSymbol.mEdgeList[i++].to = 3;
    mShieldSymbol.mEdgeList[i].from = 3;
    mShieldSymbol.mEdgeList[i++].to = 4;
    mShieldSymbol.mEdgeList[i].from = 4;
    mShieldSymbol.mEdgeList[i++].to = 5;
    mShieldSymbol.mEdgeList[i].from = 5;
    mShieldSymbol.mEdgeList[i++].to = 6;
    mShieldSymbol.mEdgeList[i].from = 6;
    mShieldSymbol.mEdgeList[i++].to = 7;
    mShieldSymbol.mEdgeList[i].from = 7;
    mShieldSymbol.mEdgeList[i++].to = 0;
    mShieldSymbol.mEdgeList[i].from = 8;
    mShieldSymbol.mEdgeList[i++].to = 9;
    mShieldSymbol.mEdgeList[i].from = 10;
    mShieldSymbol.mEdgeList[i++].to = 11;
}

scene::~scene()
{
    printf("%s\n", __func__);
    theGame->quitThreads();
    theGame.reset();
}

void scene::run()
{
    theGame->run();
}

void scene::draw(int pass)
{
    mPass = pass;

    // *******************************************

    static float colorTimer = 0;
    colorTimer += .08;

    vector::pen rainbowPen;
    rainbowPen.r = get_sin(colorTimer + ((2 * PI) / 1)) + .5;
    rainbowPen.g = get_sin(colorTimer + ((2 * PI) / 2)) + .5;
    rainbowPen.b = get_sin(colorTimer + ((2 * PI) / 3)) + .5;
    rainbowPen.a = 1;
    rainbowPen.lineRadius = 3;

#if !defined(USE_DVG_RENDERER)
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(70, mWidth / mHeight, 0.01, 1000);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Glowy blending effect
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        theGame->draw(pass);

        glDisable(GL_BLEND);

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }

    if ((game::mGameMode == game::GAMEMODE_HIGHSCORES_CHECK) || (game::mGameMode == game::GAMEMODE_HIGHSCORES)) {
        float top = .8;
        float bottom = -.8;

        glEnable(GL_BLEND);

        glColor4f(0, 0, 0, .5);
        glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2d(-1.0, bottom);
        glVertex2d(1.0, bottom);
        glVertex2d(1.0, top);
        glVertex2d(-1.0, top);
        glEnd();

        glColor4f(0, 1, 0, .1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBegin(GL_QUADS);
        glVertex2d(-1.0, bottom);
        glVertex2d(1.0, bottom);
        glVertex2d(1.0, top);
        glVertex2d(-1.0, top);
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        if (pass == RENDERPASS_PRIMARY) {
            glLineWidth(6);

            glColor4f(0, 1, 0, .6);

            glBegin(GL_LINES);
            glVertex2d(1.0, top + .005);
            glVertex2d(-1.0, top + .005);
            glVertex2d(-1.0, bottom - .005);
            glVertex2d(1.0, bottom - .005);
            glEnd();
        }
    } else if ((game::mGameMode == game::GAMEMODE_ATTRACT) || (game::mGameMode == game::GAMEMODE_CREDITED)
               /*|| (game::mGameMode == game::GAMEMODE_HIGHSCORES)*/) {
        float top = .65;
        float bottom = -.4;

        glEnable(GL_BLEND);

        glColor4f(0, .1, 0, .9);
        glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2d(-1.0, bottom);
        glVertex2d(1.0, bottom);
        glVertex2d(1.0, top);
        glVertex2d(-1.0, top);
        glEnd();

        glColor4f(0, 1, 0, .1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        if (pass == RENDERPASS_PRIMARY) {

            glLineWidth(6);

            glColor4f(.1, 1, .1, .4);

            if (!mTextureMarquee.mLoaded)
                mTextureMarquee.load("images/marquee.png");

            // Draw the marquee
            if (!mShowHighScores) {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                glEnable(GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

                mTextureMarquee.bind();

                glColor4f(1, 1, 1, 1);

                float x = 0;
                float y = 0;
                float width = 2;
                float height = 2 / 0.5625;

                glBegin(GL_QUADS);
                glTexCoord2d(0, 1);
                glVertex2d(x - (width / 2), y - (height / 2));
                glTexCoord2d(1, 1);
                glVertex2d(x + (width / 2), y - (height / 2));
                glTexCoord2d(1, 0);
                glVertex2d(x + (width / 2), y + (height / 2));
                glTexCoord2d(0, 0);
                glVertex2d(x - (width / 2), y + (height / 2));
                glEnd();

                mTextureMarquee.unbind();

                glDisable(GL_TEXTURE_2D);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
        }
    }
#else // USE_DVG_RENDERER
    theGame->draw(pass);
#endif

    if (pass == RENDERPASS_PRIMARY) {
#if !defined(USE_DVG_RENDERER)
        glEnable2D();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_MULTISAMPLE);
#endif

        if ((game::mGameMode != game::GAMEMODE_HIGHSCORES) && (game::mGameMode != game::GAMEMODE_CHOOSE_GAMETYPE))
            drawScores();

        if (game::mGameMode == game::GAMEMODE_HIGHSCORES) {
            drawCredits();
            theGame->mHighscore->drawEnterScore();
        } else if (game::mGameMode == game::GAMEMODE_GAMEOVER) {
            drawCredits();
            drawScores();
        } else if ((game::mGameMode == game::GAMEMODE_ATTRACT) || (game::mGameMode == game::GAMEMODE_CREDITED)) {
            if (++mAttractModeTimer > (mShowHighScores ? 200 : 1000)) {
                mAttractModeTimer = 0;
            }

            if (mShowHighScores) {
                theGame->mHighscore->drawTable();
            } else {
                if (game::mGameMode == game::GAMEMODE_CREDITED) {
                    drawCredits();
                }
            }
        } else if (game::mGameMode == game::GAMEMODE_CHOOSE_GAMETYPE) {
            vector::pen pen = defaultFontPen;
            font::AlphanumericsPrint(font::ALIGN_CENTER, .025, 0, 0, &pen, "Choose Game Type And Players");

            pen.a = .1;
            pen.lineRadius = 8;
            font::AlphanumericsPrint(font::ALIGN_CENTER, .025, 0, 0, &pen, "Choose Game Type And Players");

            pen.r = 1;
            pen.g = 1;
            pen.b = 1;
            pen.a = 1;
            pen.lineRadius = defaultFontPen.lineRadius;
            if (menuSelectGameType::selection == 0) {
                font::AlphanumericsPrint(font::ALIGN_CENTER, .025, 0, .9, &pen, "Single Player");
            } else {
                font::AlphanumericsPrint(font::ALIGN_CENTER, .025, 0, -.9, &pen, "Multiplayer Co-op");
            }

            pen.a = .1;
            pen.lineRadius = 8;
            if (menuSelectGameType::selection == 0) {
                font::AlphanumericsPrint(font::ALIGN_CENTER, .025, 0, .9, &pen, "Single Player");
            } else {
                font::AlphanumericsPrint(font::ALIGN_CENTER, .025, 0, -.9, &pen, "Multiplayer Co-op");
            }
        } else // RUNNING
        {
            drawNumLives();
            drawNumBombs();
        }

#if !defined(USE_DVG_RENDERER)
        glDisable(GL_MULTISAMPLE);
        glDisable(GL_LINE_SMOOTH);

        glDisable2D();
#endif
    }
}

void scene::drawCredits()
{
    if (settings::get().mCreditsPerGame == 0) {
    } else {
        vector::pen pen = defaultFontPen;
        font::AlphanumericsPrint(font::ALIGN_CENTER, .015, 0, -.9, &pen, "Credits %d", game::mCredits);

        pen.a = .2;
        pen.lineRadius = 8;
        font::AlphanumericsPrint(font::ALIGN_CENTER, .015, 0, -.9, &pen, "Credits %d", game::mCredits);
    }
}

void scene::drawNumLives()
{
    int overflow = 0;
    int num = theGame->getPlayer1()->getNumLives();
    if (num > 5) {
        overflow = num;
        num = 1;
    }

    for (int i = 0; i < num; i++) {
        vector::pen pen(defaultFontPen, .75, 10);
        float scale = .017;

        const auto& model = theGame->getPlayer1()->getModel();
        model->Identity();
        model->Scale(Point3d(scale, scale * mAspect, 0));
        model->Rotate(0);
        model->Translate(Point3d(-.12 + (.04 * i), .9, 0));
        model->draw(pen);
    }

    if (overflow > 0) {
        vector::pen pen = defaultFontPen;
        font::AlphanumericsPrint(font::ALIGN_CENTER, .016, -.12 - .06, .9, &pen, "x%d", overflow);

        pen.a = .1;
        pen.lineRadius = 8;
        font::AlphanumericsPrint(font::ALIGN_CENTER, .016, -.12 - .06, .9, &pen, "x%d", overflow);
    }
}

void scene::drawNumBombs()
{
    if (theGame->mGameType == game::GAMETYPE_SINGLEPLAYER) {
        int overflow = 0;
        int num = theGame->getPlayer1()->getNumBombs();
        if (num > 5) {
            overflow = num;
            num = 1;
        }

        for (int i = 0; i < num; i++) {
            vector::pen pen(defaultFontPen, .75, 10);
            float scale = .0014;

            mShieldSymbol.Identity();
            mShieldSymbol.Scale(Point3d(scale, scale * mAspect, 0));
            mShieldSymbol.Rotate(0);
            mShieldSymbol.Translate(Point3d(.12 + (.04 * i), .9, 0));
            mShieldSymbol.draw(pen);
        }

        if (overflow > 0) {
            vector::pen pen = defaultFontPen;
            font::AlphanumericsPrint(font::ALIGN_CENTER, .016, .12 + .06, .9, &pen, "x%d", overflow);

            pen.a = .1;
            pen.lineRadius = 8;
            font::AlphanumericsPrint(font::ALIGN_CENTER, .016, .12 + .06, .9, &pen, "x%d", overflow);
        }
    }
}

void scene::drawScores()
{
    bool gameover = (theGame->mGameMode != game::GAMEMODE_PLAYING);

    if (theGame->mGameType == game::GAMETYPE_SINGLEPLAYER) {
        char format[512];
        snprintf(format, sizeof(format), "%d", theGame->getPlayer1()->mScore);
        char* s = font::formatStringWithCommas(format);

        vector::pen pen = defaultFontPen;
        if (gameover)
            font::AlphanumericsPrint(font::ALIGN_LEFT, .016, -.94, .9, &pen, "Score");
        else
            font::AlphanumericsPrint(font::ALIGN_LEFT, .016, -.94, .9, &pen, "Score x%d", theGame->getPlayer1()->mMultiplier);
        font::AlphanumericsPrint(font::ALIGN_LEFT, .02, -.94, .82, &pen, s);

        pen.a = .1;
        pen.lineRadius = 8;
        if (gameover)
            font::AlphanumericsPrint(font::ALIGN_LEFT, .016, -.94, .9, &pen, "Score");
        else
            font::AlphanumericsPrint(font::ALIGN_LEFT, .016, -.94, .9, &pen, "Score x%d", theGame->getPlayer1()->mMultiplier);
        font::AlphanumericsPrint(font::ALIGN_LEFT, .02, -.94, .82, &pen, s);
    } else {
        for (int i = 0; i < 4; i++) {
            Point3d pos;
            player* player;
            switch (i) {
            case 0:
                player = theGame->getPlayer1();
                pos = Point3d(-.7, .9, 0);
                break;
            case 1:
                player = theGame->getPlayer2();
                pos = Point3d(.3, .9, 0);
                break;
            case 2:
                player = theGame->getPlayer3();
                pos = Point3d(-.7, -.9, 0);
                break;
            case 3:
                player = theGame->getPlayer4();
                pos = Point3d(.3, -.9, 0);
                break;
            }

            if (player->mJoined) {
                char format[512];
                snprintf(format, sizeof(format), "%d", player->mScore);
                char* s = font::formatStringWithCommas(format);

                vector::pen pen(player->getFontPen(), .75, 3);
                if (gameover || (player->getState() != entity::ENTITY_STATE_RUNNING))
                    font::AlphanumericsPrint(font::ALIGN_LEFT, .02, pos.x, pos.y, &pen, "%s", s);
                else
                    font::AlphanumericsPrint(font::ALIGN_LEFT, .02, pos.x, pos.y, &pen, "%s x%d", s, player->mMultiplier);

                pen.a = .1;
                pen.lineRadius = 8;
                if (gameover || (player->getState() != entity::ENTITY_STATE_RUNNING))
                    font::AlphanumericsPrint(font::ALIGN_LEFT, .02, pos.x, pos.y, &pen, "%s", s);
                else
                    font::AlphanumericsPrint(font::ALIGN_LEFT, .02, pos.x, pos.y, &pen, "%s x%d", s, player->mMultiplier);
            } else {
                vector::pen pen(player->getFontPen(), .75, 3);
                font::AlphanumericsPrint(font::ALIGN_LEFT, .02, pos.x, pos.y, &pen, "Press Start");

                pen.a = .1;
                pen.lineRadius = 8;
                font::AlphanumericsPrint(font::ALIGN_LEFT, .02, pos.x, pos.y, &pen, "Press Start");
            }
        }
    }
}

#if !defined(USE_DVG_RENDERER)
void scene::glEnable2D()
{
    int vPort[4];
    vPort[0] = VIRTUAL_SCREEN_HEIGHT;
    vPort[1] = 0;
    vPort[2] = 0;
    vPort[3] = VIRTUAL_SCREEN_WIDTH;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(vPort[0], vPort[0] + vPort[2], vPort[1] + vPort[3], vPort[1], -1, 1);

    glViewport(0, 0, mWidth, mHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void scene::glDisable2D()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
#endif // !USE_DVG_RENDERER

void scene::showHighScores()
{
    if (settings::get().mCreditsPerGame == 0 || game::mCredits) {
        game::mGameMode = game::GAMEMODE_CREDITED;
    } else {
        game::mGameMode = game::GAMEMODE_ATTRACT;
    }
    mAttractModeTimer = -200;
    mShowHighScores = true;
}
