#include "md/io.h"
#include "JoypadHandler.h"

// TOP_SPEED is in pixels/frame.  Maximum value is 16.
#define TOP_SPEED 4

uint16_t joystate = 0;

void Joypad_update()
{
    joystate = g_md_pad[0];

    if (joystate & BTN_RIGHT)
    {
        fgCameraPixelX += TOP_SPEED;
    }
    else if (joystate & BTN_LEFT)
    {
        // Since we're directly manipulating the camera, make sure it doesn't go negative.
        if (fgCameraPixelX != 0)
        {
            fgCameraPixelX -= TOP_SPEED;
        }
    }

    if (joystate & BTN_UP)
    {
        // Since we're directly manipulating the camera, make sure it doesn't go negative.
        if (fgCameraPixelY != 0)
        {
            fgCameraPixelY -= TOP_SPEED;
        }
    }
    else if (joystate & BTN_DOWN)
    {
        fgCameraPixelY += TOP_SPEED;
    }
}
