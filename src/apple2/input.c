/*
  Apple II Joystick
*/

#include <apple2.h>
#include <joystick.h>

static unsigned char installedDriver = 0, canReadJoystick=0;

unsigned char readJoystick() {

  if (!installedDriver) {
    installedDriver=1;

    if (joy_install(joy_static_stddrv) == JOY_ERR_OK) {
      canReadJoystick = joy_read(JOY_1) == 0;
    }
  }
  
  return canReadJoystick ? joy_read(JOY_1) : 0;
}