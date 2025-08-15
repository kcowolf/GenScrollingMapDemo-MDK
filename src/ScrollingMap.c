#include <stdint.h>
#include "md/megadrive.h"
#include "graphics.h"
#include "MathUtil.h"
#include "ScrollingMap.h"

// TODO -- Background should probably wrap -- at least horizontally if not vertically.

// NOTE: While not a direct port from the original, the structure and techniques used here were inspired from
// sikthehedgehog's Dragon's Castle.
//
// https://github.com/sikthehedgehog/dragon/blob/master/src-68k/stage.68k

// NOTE: Width of background map must be ((width of foreground map / 2) + 160).
// NOTE: Height of background map must be ((height of foreground map / 2) + 112).
#define PLANE_FG VDP_BG_A
#define PLANE_BG VDP_BG_B

// The maximum coordinates (towards the bottom right) where the camera can be without showing anything beyond the map edges.
uint32_t fgCameraLimitPixelX;
uint32_t fgCameraLimitPixelY;

// The coordinates of the camera (top-left pixel on the screen).  These will be clamped to be within the map boundaries (0,0)-(cameraLimitPixelX,cameraLimitPixelY)
uint32_t fgCameraPixelX;
uint32_t fgCameraPixelY;

// Coordinates of the tile containing fgCameraPixelX and fgCameraPixelY.
uint16_t fgCameraTileX;
uint16_t fgCameraTileY;

// Coordinates of the tile at the top left of the background.  Background scrolls at half the rate of the foreground.
uint16_t bgCameraTileX;
uint16_t bgCameraTileY;

// Buffers used for copying map data to VRAM.
uint16_t fgRowBuffer1[VDP_PLANE_TILE_WIDTH];
uint16_t fgRowBuffer2[VDP_PLANE_TILE_WIDTH];
uint16_t fgColumnBuffer1[VDP_PLANE_TILE_HEIGHT];
uint16_t fgColumnBuffer2[VDP_PLANE_TILE_HEIGHT];

uint16_t bgRowBuffer1[VDP_PLANE_TILE_WIDTH];
uint16_t bgRowBuffer2[VDP_PLANE_TILE_WIDTH];
uint16_t bgColumnBuffer1[VDP_PLANE_TILE_HEIGHT];
uint16_t bgColumnBuffer2[VDP_PLANE_TILE_HEIGHT];

uint16_t fgRowOffsets[ROW_OFFSET_COUNT];
uint16_t bgRowOffsets[ROW_OFFSET_COUNT];

void updateVDP();
void redrawForegroundRow(uint16_t rowToUpdate);
void redrawBackgroundRow(uint16_t rowToUpdate);
void redrawForegroundColumn(uint16_t columnToUpdate);
void redrawBackgroundColumn(uint16_t columnToUpdate);
void redrawForegroundScreen();
void redrawBackgroundScreen();
void updateCamera();

void ScrollingMap_init()
{
    md_vdp_set_plane_size(VDP_PLANESIZE_64x32);

    // TODO -- Initialize the camera's position based on the player's starting position.
    fgCameraPixelX = 0;
    fgCameraPixelY = 0;
    fgCameraLimitPixelX = TILEMAP_FG_PIXEL_WIDTH - SCREEN_PIXEL_WIDTH;
    fgCameraLimitPixelY = TILEMAP_FG_PIXEL_HEIGHT - SCREEN_PIXEL_HEIGHT;

    // Load tiles
    md_dma_transfer_vram(TILESET_FG_START_IDX << 5, &TILESET_FG, TILESET_FG_TILE_COUNT << 4, 2);
    md_dma_transfer_vram(TILESET_BG_START_IDX << 5, &TILESET_BG, TILESET_BG_TILE_COUNT << 4, 2);

    // Calculate row offsets so we don't need to multiply later.
    uint16_t i;

    uint16_t rowOffset = 0;
    for (i = 0; i < TILEMAP_FG_TILE_HEIGHT; i += 2)
    {
        fgRowOffsets[i >> 1] = rowOffset;
        rowOffset += (TILEMAP_FG_TILE_WIDTH << 1);
    }

    rowOffset = 0;
    for (i = 0; i < TILEMAP_BG_TILE_HEIGHT; i += 2)
    {
        bgRowOffsets[i >> 1] = rowOffset;
        rowOffset += (TILEMAP_BG_TILE_WIDTH << 1);
    }

    updateCamera();
    updateVDP();
    redrawForegroundScreen();
    redrawBackgroundScreen();
}

void ScrollingMap_update()
{
    uint16_t oldFGCameraTileX = fgCameraTileX;
    uint16_t oldFGCameraTileY = fgCameraTileY;
    uint16_t oldBGCameraTileX = bgCameraTileX;
    uint16_t oldBGCameraTileY = bgCameraTileY;

    updateCamera();

    // Foreground
    if (fgCameraTileX < oldFGCameraTileX)
    {
        // Moved left.
        redrawForegroundColumn(fgCameraTileX);
    }
    else if (fgCameraTileX > oldFGCameraTileX)
    {
        // Moved right.
        redrawForegroundColumn(fgCameraTileX + SCREEN_TILE_WIDTH);
    }

    if (fgCameraTileY < oldFGCameraTileY)
    {
        // Moved up.
        redrawForegroundRow(fgCameraTileY);
    }
    else if (fgCameraTileY > oldFGCameraTileY)
    {
        // Moved down.
        redrawForegroundRow(fgCameraTileY + SCREEN_TILE_HEIGHT);
    }

    // Background
    if (bgCameraTileX < oldBGCameraTileX)
    {
        // Moved left.
        redrawBackgroundColumn(bgCameraTileX);
    }
    else if (bgCameraTileX > oldBGCameraTileX)
    {
        // Moved right.
        redrawBackgroundColumn(bgCameraTileX + SCREEN_TILE_WIDTH);
    }

    if (bgCameraTileY < oldBGCameraTileY)
    {
        // Moved up.
        redrawBackgroundRow(bgCameraTileY);
    }
    else if (bgCameraTileY > oldBGCameraTileY)
    {
        // Moved down.
        redrawBackgroundRow(bgCameraTileY + SCREEN_TILE_HEIGHT);
    }

    updateVDP();
}

void updateVDP()
{
    static int16_t hScrollDataA;
    static int16_t vScrollDataA;

    static int16_t hScrollDataB;
    static int16_t vScrollDataB;

    hScrollDataA = -(fgCameraPixelX);
    vScrollDataA = (fgCameraPixelY);
    hScrollDataB = -(fgCameraPixelX >> 1);
    vScrollDataB = (fgCameraPixelY) >> 1;

    // Foreground
    md_dma_transfer_vram(VRAM_HSCR_BASE_DEFAULT, &hScrollDataA, 1, 2);
    md_dma_transfer_vsram(0, &vScrollDataA, 1, 2);

    // Background, scrolls at half the rate.
    md_dma_transfer_vram(VRAM_HSCR_BASE_DEFAULT + 2, &hScrollDataB, 1, 2);
    md_dma_transfer_vsram(2, &vScrollDataB, 1, 2);
}

void redrawForegroundRow(uint16_t rowToUpdate)
{
    // Calculate where in the tilemap the new row's tiles are located.
    const uint16_t* mapDataAddr = TILEMAP_FG + fgRowOffsets[rowToUpdate >> 1] + fgCameraTileX;

    uint16_t rowBufferIdx = fgCameraTileX;

    // Copy the tiles into the buffer.
    uint16_t i;
    for (i = VDP_PLANE_TILE_WIDTH; i != 0; i--)
    {
        rowBufferIdx &= 0x3F;  // rowBufferIdx MOD 64 (VDP_PLANE_TILE_WIDTH)
        fgRowBuffer1[rowBufferIdx] = *mapDataAddr;
        fgRowBuffer2[rowBufferIdx] = *(mapDataAddr + TILEMAP_FG_TILE_WIDTH);
        rowBufferIdx++;
        mapDataAddr++;
    }

    // Queue copying the buffer into VRAM.
    md_dma_transfer_vram(VRAM_SCRA_BASE_DEFAULT + ((((rowToUpdate & VDP_PLANE_TILE_HEIGHT_MINUS_ONE) << 6)) << 1), &fgRowBuffer1, VDP_PLANE_TILE_WIDTH, 2);
    md_dma_transfer_vram(VRAM_SCRA_BASE_DEFAULT + (((((rowToUpdate + 1) & VDP_PLANE_TILE_HEIGHT_MINUS_ONE) << 6)) << 1), &fgRowBuffer2, VDP_PLANE_TILE_WIDTH, 2);
}

void redrawBackgroundRow(uint16_t rowToUpdate)
{
    // Calculate where in the tilemap the new row's tiles are located.
    const uint16_t* mapDataAddr = TILEMAP_BG + bgRowOffsets[rowToUpdate >> 1] + bgCameraTileX;

    uint16_t rowBufferIdx = bgCameraTileX;

    // Copy the tiles into the buffer.
    uint16_t i;
    for (i = VDP_PLANE_TILE_WIDTH; i != 0; i--)
    {
        rowBufferIdx &= 0x3F;  // rowBufferIdx MOD 64 (VDP_PLANE_TILE_WIDTH)
        bgRowBuffer1[rowBufferIdx] = *mapDataAddr;
        bgRowBuffer2[rowBufferIdx] = *(mapDataAddr + TILEMAP_BG_TILE_WIDTH);
        rowBufferIdx++;
        mapDataAddr++;
    }

    // Queue copying the buffer into VRAM.
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + ((((rowToUpdate & VDP_PLANE_TILE_HEIGHT_MINUS_ONE) << 6)) << 1), &bgRowBuffer1, VDP_PLANE_TILE_WIDTH, 2);
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + (((((rowToUpdate + 1) & VDP_PLANE_TILE_HEIGHT_MINUS_ONE) << 6)) << 1), &bgRowBuffer2, VDP_PLANE_TILE_WIDTH, 2);
}

void redrawForegroundColumn(uint16_t columnToUpdate)
{
    // Calculate where in the tilemap the new row's tiles are located.
    const uint16_t* mapDataAddr = TILEMAP_FG + fgRowOffsets[fgCameraTileY >> 1] + columnToUpdate;

    uint16_t columnBufferIdx = fgCameraTileY;

    // Copy the tiles into the buffer.
    uint16_t i;
    for (i = VDP_PLANE_TILE_HEIGHT; i != 0; i--)
    {
        columnBufferIdx &= 0x1F;  // columnBufferIdx MOD 32 (VDP_PLANE_TILE_HEIGHT)
        fgColumnBuffer1[columnBufferIdx] = *mapDataAddr;
        fgColumnBuffer2[columnBufferIdx] = *(mapDataAddr + 1);
        columnBufferIdx++;
        mapDataAddr += TILEMAP_FG_TILE_WIDTH;
    }

    // Queue copying the buffer into VRAM.
    md_dma_transfer_vram(VRAM_SCRA_BASE_DEFAULT + ((columnToUpdate & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &fgColumnBuffer1, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
    md_dma_transfer_vram(VRAM_SCRA_BASE_DEFAULT + (((columnToUpdate + 1) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &fgColumnBuffer2, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
}

void redrawBackgroundColumn(uint16_t columnToUpdate)
{
    // Calculate where in the tilemap the new row's tiles are located.
    const uint16_t* mapDataAddr = TILEMAP_BG + bgRowOffsets[bgCameraTileY >> 1] + columnToUpdate;

    uint16_t columnBufferIdx = bgCameraTileY;

    // Copy the tiles into the buffer.
    uint16_t i;
    for (i = VDP_PLANE_TILE_HEIGHT; i != 0; i--)
    {
        columnBufferIdx &= 0x1F;  // columnBufferIdx MOD 32 (VDP_PLANE_TILE_HEIGHT)
        bgColumnBuffer1[columnBufferIdx] = *mapDataAddr;
        bgColumnBuffer2[columnBufferIdx] = *(mapDataAddr + 1);
        columnBufferIdx++;
        mapDataAddr += TILEMAP_BG_TILE_WIDTH;
    }

    // Queue copying the buffer into VRAM.
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + ((columnToUpdate & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &bgColumnBuffer1, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + (((columnToUpdate + 1) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &bgColumnBuffer2, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
}

// Redraw the whole screen.  Normally this would be done with the screen blacked out.
void redrawForegroundScreen()
{
    uint16_t currentCol = SCREEN_TILE_WIDTH_PLUS_TWO;
    do
    {
        currentCol -= 2;

        // Calculate where in the tilemap the new row's tiles are located.
        const uint16_t* mapDataAddr = TILEMAP_FG + fgRowOffsets[fgCameraTileY >> 1] + fgCameraTileX + currentCol;

        uint16_t columnBufferIdx = fgCameraTileY;

        // Copy the tiles into the buffer.
        uint16_t i;
        for (i = VDP_PLANE_TILE_HEIGHT; i != 0; i--)
        {
            columnBufferIdx &= 0x1F;  // columnBufferIdx MOD 32 (VDP_PLANE_TILE_HEIGHT)
            fgColumnBuffer1[columnBufferIdx] = *mapDataAddr;
            fgColumnBuffer2[columnBufferIdx] = *(mapDataAddr + 1);
            columnBufferIdx++;
            mapDataAddr += TILEMAP_FG_TILE_WIDTH;
        }

        // Since we're redrawing the whole screen, do the DMA immediately instead of queuing it up.
        md_dma_transfer_vram(VRAM_SCRA_BASE_DEFAULT + (((fgCameraTileX + currentCol) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &fgColumnBuffer1, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
        md_dma_transfer_vram(VRAM_SCRA_BASE_DEFAULT + (((fgCameraTileX + currentCol + 1) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &fgColumnBuffer2, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
        md_dma_process();
    }
    while (currentCol != 0);
}

void redrawBackgroundScreen()
{
    uint16_t currentCol = SCREEN_TILE_WIDTH_PLUS_TWO;
    do
    {
        currentCol -= 2;

        // Calculate where in the tilemap the new row's tiles are located.
        const uint16_t* mapDataAddr = TILEMAP_BG + bgRowOffsets[bgCameraTileY >> 1] + bgCameraTileX + currentCol;

        uint16_t columnBufferIdx = bgCameraTileY;

        // Copy the tiles into the buffer.
        uint16_t i;
        for (i = VDP_PLANE_TILE_HEIGHT; i != 0; i--)
        {
            columnBufferIdx &= 0x1F;  // columnBufferIdx MOD 32 (VDP_PLANE_TILE_HEIGHT)
            bgColumnBuffer1[columnBufferIdx] = *mapDataAddr;
            bgColumnBuffer2[columnBufferIdx] = *(mapDataAddr + 1);
            columnBufferIdx++;
            mapDataAddr += TILEMAP_BG_TILE_WIDTH;
        }

        // Since we're redrawing the whole screen, do the DMA immediately instead of queuing it up.
        md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + (((bgCameraTileX + currentCol) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &bgColumnBuffer1, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
        md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + (((bgCameraTileX + currentCol + 1) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &bgColumnBuffer2, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
        md_dma_process();
    }
    while (currentCol != 0);
}

void updateCamera()
{
    // TODO -- Calculate the camera's position based on the player's current position.

    // TODO -- Make sure the camera's position doesn't go below zero (for this demo we do this in JoypadHandler).

    if (fgCameraPixelX > fgCameraLimitPixelX)
    {
        fgCameraPixelX = fgCameraLimitPixelX;
    }

    if (fgCameraPixelY > fgCameraLimitPixelY)
    {
        fgCameraPixelY = fgCameraLimitPixelY;
    }

    // tileX and tileY only change when the camera moves 16 pixels horizontally.  & 0xFFFE means these will always be even numbers.
    fgCameraTileX = PIXEL_TO_TILE(fgCameraPixelX) & 0xFFFE;
    fgCameraTileY = PIXEL_TO_TILE(fgCameraPixelY) & 0xFFFE;

    bgCameraTileX = PIXEL_TO_TILE(fgCameraPixelX >> 1) & 0xFFFE;
    bgCameraTileY = PIXEL_TO_TILE(fgCameraPixelY >> 1) & 0xFFFE;
}
