/*
  Platform specific vars.
*/
#ifndef VARS_H
#define VARS_H

// Default vars (covers most playforms)
#ifndef ESCAPE
#define ESCAPE "ESCAPE"
#define ESC "ESC"
#endif

// Include platform specific vars - this is defined in the Makefile as "../$(PLATFORM)/vars.h"
// Watcom / wine has issues with \" in the define, so hacking this for now
#if __MSDOS__
#include "../msdos/vars.h"
#else
#include PLATFORM_VARS
#endif

#endif /* VARS_H */
