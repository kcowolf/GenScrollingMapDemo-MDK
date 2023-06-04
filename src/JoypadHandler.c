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
        bgCameraPixelX += TOP_SPEED;
    }
    else if (joystate & BTN_LEFT)
    {
        // Since we're directly manipulating the camera, make sure it doesn't go negative.
        if (bgCameraPixelX != 0)
        {
            bgCameraPixelX -= TOP_SPEED;
        }
    }

    if (joystate & BTN_UP)
    {
        // Since we're directly manipulating the camera, make sure it doesn't go negative.
        if (bgCameraPixelY != 0)
        {
            bgCameraPixelY -= TOP_SPEED;
        }
    }
    else if (joystate & BTN_DOWN)
    {
        bgCameraPixelY += TOP_SPEED;
    }
}
