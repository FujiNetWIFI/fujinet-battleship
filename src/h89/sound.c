#ifdef __H89__

/**
 * @brief   Sound - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The only sound an H89 makes is the H19's bell, so it marks the moments
 * that need the player's attention and the rest stay quiet.
 */

#include "../misc.h"
#include "console.h"

static void bell(void)
{
    if (!prefs.disableSound)
        termBell();
}

void initSound() {}

void soundJoinGame()
{
    bell();
}

void soundMyTurn()
{
    bell();
}

void soundGameDone()
{
    bell();
}

void soundInvalid()
{
    bell();
}

void soundSink()
{
    bell();
}

void soundCursor() {}
void soundPlaceShip() {}
void soundTick() {}
void soundSelect() {}
void soundMiss() {}
void soundAttack() {}
void soundHit() {}
void soundStop() {}
void disableKeySounds() {}
void enableKeySounds() {}

#endif /* __H89__ */
