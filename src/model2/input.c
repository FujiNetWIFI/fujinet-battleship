#ifdef __TRS80M2__

/*
  Input - the P&T BIOS's key codes, read raw so BREAK reaches the game.
  Reading the keyboard is also when the screen is sent: the shared code
  polls kbhit() in every input loop and blocks in cgetc().
*/

#include <stdint.h>
#include "console.h"
#include "../misc.h"

static int pending = -1;

unsigned char kbhit(void)
{
    flush();
    if (pending < 0)
        pending = conIn();
    return pending >= 0;
}

unsigned char cgetc(void)
{
    static unsigned char k;

    while (!kbhit())
        ;
    k = (unsigned char)pending;
    pending = -1;
    return k;
}

unsigned char readJoystick()
{
    return 0;
}

#endif /* __TRS80M2__ */
