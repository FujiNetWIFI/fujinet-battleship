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
#include PLATFORM_VARS

#endif /* VARS_H */
