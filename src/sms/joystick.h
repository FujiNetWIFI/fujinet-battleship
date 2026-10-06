#ifdef BUILD_SMS

/**
 * @brief Sega Master System joypad definitions
 * @author Thomas Cherryhomes
 * @license gpl v.3
 */

#ifndef _JOYSTICK_H
#define _JOYSTICK_H

/*
  Port $DC, player 1, inverted so a set bit is a pressed button. The reader
  (src/sms/input.c) defines the layout; only the four directions ever reach
  readCommonInput(), the buttons are reported as keys.
*/

#define JOY_UP_MASK             0x01
#define JOY_DOWN_MASK           0x02
#define JOY_LEFT_MASK           0x04
#define JOY_RIGHT_MASK          0x08
#define JOY_BTN_1_MASK          0x10
#define JOY_BTN_2_MASK          0x20

#define JOY_DIRS_MASK           0x0F

#define JOY_UP(v)               ((v) & JOY_UP_MASK)
#define JOY_DOWN(v)             ((v) & JOY_DOWN_MASK)
#define JOY_LEFT(v)             ((v) & JOY_LEFT_MASK)
#define JOY_RIGHT(v)            ((v) & JOY_RIGHT_MASK)
#define JOY_BTN_1(v)            ((v) & JOY_BTN_1_MASK)
#define JOY_BTN_2(v)            ((v) & JOY_BTN_2_MASK)

#endif /* _JOYSTICK_H */

#endif /* BUILD_SMS */
