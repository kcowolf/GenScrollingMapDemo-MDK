#ifndef JOYPADHANDLER_H
#define JOYPADHANDLER_H

#include <stdint.h>

void Joypad_update();

// TODO -- Normally these wouldn't be exposed here.  JoypadHandler would manipulate the player's
// position and the camera's position would be calculated based on that.  However, for this demo
// I let the JoypadHandler manipulate the camera coordinates directly.
extern uint32_t bgCameraPixelX;
extern uint32_t bgCameraPixelY;

#endif // JOYPADHANDLER_H
