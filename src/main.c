#include "md/megadrive.h"
#include "graphics.h"
#include "JoypadHandler.h"
#include "ScrollingMap.h"

int main()
{
    megadrive_init();
    md_spr_init(SPR_MODE_DIRECT);

    ScrollingMap_init();

    // Load palettes
    md_pal_upload(0, PAL_BG, 16);
    md_pal_upload(16, PAL_FG, 16);
    md_pal_set(16, 0x0e00);  // Background color
    md_pal_upload(32, PAL_BG, 16);
    md_pal_upload(48, PAL_FG, 16);
    md_pal_set(63, 0x0eee);  // Text color

    while(1)
    {
        Joypad_update();
        ScrollingMap_update();

        // Wait for VBlank
        megadrive_finish();
        ScrollingMap_updateVDP();
    }
}