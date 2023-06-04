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

// NOTE: Assumes background will each only use one palette.  Sonic 2's foregrounds can use at least 2.

#define PLANE_BG VDP_BG_B

// The maximum coordinates (towards the bottom right) where the camera can be without showing anything beyond the map edges.
uint32_t bgCameraLimitPixelX;
uint32_t bgCameraLimitPixelY;

// The coordinates of the camera (top-left pixel on the screen).  These will be clamped to be within the map boundaries (0,0)-(cameraLimitPixelX,cameraLimitPixelY)
uint32_t bgCameraPixelX;
uint32_t bgCameraPixelY;

// Coordinates of the tile containing bgCameraPixelX and bgCameraPixelY.
uint16_t bgCameraTileX;
uint16_t bgCameraTileY;

// Buffers used for copying map data to VRAM.
uint16_t bgRowBuffer1[VDP_PLANE_TILE_WIDTH];
uint16_t bgRowBuffer2[VDP_PLANE_TILE_WIDTH];
uint16_t bgColumnBuffer1[VDP_PLANE_TILE_HEIGHT];
uint16_t bgColumnBuffer2[VDP_PLANE_TILE_HEIGHT];

uint16_t mapTileWidth;
uint16_t mapTileHeight;
const uint16_t* tileMap;

uint16_t bgTilesetStartIdx;

uint16_t bgRowOffsets[ROW_OFFSET_COUNT];

void redrawBackgroundRow(uint16_t rowToUpdate);
void redrawBackgroundColumn(uint16_t columnToUpdate);
void redrawBackgroundScreen();
void updateCamera();

void ScrollingMap_init()
{
    md_vdp_set_plane_size(VDP_PLANESIZE_64x32);

    mapTileWidth = TILEMAP_FG_TILE_WIDTH;
    mapTileHeight = TILEMAP_FG_TILE_HEIGHT;
    tileMap = TILEMAP_FG;

    // TODO -- Initialize the camera's position based on the player's starting position.
    bgCameraPixelX = 0;
    bgCameraPixelY = 0;
    bgCameraLimitPixelX = TILE_TO_PIXEL(mapTileWidth) - SCREEN_PIXEL_WIDTH;
    bgCameraLimitPixelY = TILE_TO_PIXEL(mapTileHeight) - SCREEN_PIXEL_HEIGHT;

    // Load tiles
    bgTilesetStartIdx = MAP_TILE_START_IDX;

    md_dma_transfer_vram(bgTilesetStartIdx << 5, &TILESET_FG, TILESET_FG_TILE_COUNT << 4, 2);

    // Calculate row offsets so we don't need to multiply later.
    uint16_t rowOffset = 0;
    uint16_t i;
    for (i = 0; i < mapTileHeight; i += 2)
    {
        bgRowOffsets[i >> 1] = rowOffset;
        rowOffset += (mapTileWidth << 1);
    }

    updateCamera();
    ScrollingMap_updateVDP();
    redrawBackgroundScreen();
}

void ScrollingMap_update()
{
    uint16_t oldBGCameraTileX = bgCameraTileX;
    uint16_t oldBGCameraTileY = bgCameraTileY;

    updateCamera();

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
}

void setHorizontalScroll(VdpPlane vdpPlane, int16_t value)
{
    if (vdpPlane == VDP_PLANE_B)
    {
        md_dma_transfer_vram(VRAM_HSCR_BASE_DEFAULT + 2, &value, 1, 2);
    }
    else
    {
        md_dma_transfer_vram(VRAM_HSCR_BASE_DEFAULT, &value, 1, 2);
    }
}

void setVerticalScroll(VdpPlane vdpPlane, int16_t value)
{
    if (vdpPlane == VDP_PLANE_B)
    {
        md_dma_transfer_vsram(2, &value, 1, 2);
    }
    else
    {
        md_dma_transfer_vsram(0, &value, 1, 2);
    }
}

void ScrollingMap_updateVDP()
{
    // Background
    setHorizontalScroll(VDP_PLANE_B, -(bgCameraPixelX));
    setVerticalScroll(VDP_PLANE_B, (bgCameraPixelY));
}

void redrawBackgroundRow(uint16_t rowToUpdate)
{
    // Calculate where in the tilemap the new row's tiles are located.
    const uint16_t* mapDataAddr = tileMap + bgRowOffsets[rowToUpdate >> 1] + bgCameraTileX;

    uint16_t rowBufferIdx = bgCameraTileX;
    uint16_t baseTile = VDP_ATTR(bgTilesetStartIdx, 0, 0, 1, 0);

    // Copy the tiles into the buffer.
    uint16_t i;
    for (i = VDP_PLANE_TILE_WIDTH; i != 0; i--)
    {
        rowBufferIdx &= 0x3F;  // rowBufferIdx MOD 64 (VDP_PLANE_TILE_WIDTH)
        // TODO -- Need to determine which is better -- rowBuffer[rowBufferIdx] or *(rowBuffer + rowBufferIdx).
        bgRowBuffer1[rowBufferIdx] = baseTile + *mapDataAddr;
        bgRowBuffer2[rowBufferIdx] = baseTile + *(mapDataAddr + mapTileWidth);
        rowBufferIdx++;
        mapDataAddr++;
    }

    // Queue copying the buffer into VRAM.
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + ((((rowToUpdate & VDP_PLANE_TILE_HEIGHT_MINUS_ONE) << 6)) << 1), &bgRowBuffer1, VDP_PLANE_TILE_WIDTH, 2);
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + (((((rowToUpdate + 1) & VDP_PLANE_TILE_HEIGHT_MINUS_ONE) << 6)) << 1), &bgRowBuffer2, VDP_PLANE_TILE_WIDTH, 2);
}

void redrawBackgroundColumn(uint16_t columnToUpdate)
{
    // Calculate where in the tilemap the new row's tiles are located.
    const uint16_t* mapDataAddr = tileMap + bgRowOffsets[bgCameraTileY >> 1] + columnToUpdate;

    uint16_t columnBufferIdx = bgCameraTileY;
    uint16_t baseTile = VDP_ATTR(bgTilesetStartIdx, 0, 0, 1, 0);

    // Copy the tiles into the buffer.
    uint16_t i;
    for (i = VDP_PLANE_TILE_HEIGHT; i != 0; i--)
    {
        columnBufferIdx &= 0x1F;  // columnBufferIdx MOD 32 (VDP_PLANE_TILE_HEIGHT)
        bgColumnBuffer1[columnBufferIdx] = baseTile + *mapDataAddr;
        bgColumnBuffer2[columnBufferIdx] = baseTile + *(mapDataAddr + 1);
        columnBufferIdx++;
        mapDataAddr += mapTileWidth;
    }

    // Queue copying the buffer into VRAM.
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + ((columnToUpdate & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &bgColumnBuffer1, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
    md_dma_transfer_vram(VRAM_SCRB_BASE_DEFAULT + (((columnToUpdate + 1) & VDP_PLANE_TILE_WIDTH_MINUS_ONE) << 1), &bgColumnBuffer2, VDP_PLANE_TILE_HEIGHT, VDP_PLANE_TILE_WIDTH_TIMES_TWO);
}

// Redraw the whole screen.  Normally this would be done with the screen blacked out.

void redrawBackgroundScreen()
{
    uint16_t currentCol = SCREEN_TILE_WIDTH_PLUS_TWO;
    do
    {
        currentCol -= 2;

        // Calculate where in the tilemap the new row's tiles are located.
        const uint16_t* mapDataAddr = tileMap + bgRowOffsets[bgCameraTileY >> 1] + bgCameraTileX + currentCol;

        uint16_t columnBufferIdx = bgCameraTileY;
        uint16_t baseTile = VDP_ATTR(bgTilesetStartIdx, 0, 0, 1, 0);

        // Copy the tiles into the buffer.
        uint16_t i;
        for (i = VDP_PLANE_TILE_HEIGHT; i != 0; i--)
        {
            columnBufferIdx &= 0x1F;  // columnBufferIdx MOD 32 (VDP_PLANE_TILE_HEIGHT)
            bgColumnBuffer1[columnBufferIdx] = baseTile + *mapDataAddr;
            bgColumnBuffer2[columnBufferIdx] = baseTile + *(mapDataAddr + 1);
            columnBufferIdx++;
            mapDataAddr += mapTileWidth;
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

    if (bgCameraPixelX > bgCameraLimitPixelX)
    {
        bgCameraPixelX = bgCameraLimitPixelX;
    }

    if (bgCameraPixelY > bgCameraLimitPixelY)
    {
        bgCameraPixelY = bgCameraLimitPixelY;
    }

    // bgCameraTileX only changes when the camera moves 16 pixels horizontally.  & 0xFFFE means it will always be an even number.
    bgCameraTileX = PIXEL_TO_TILE(bgCameraPixelX) & 0xFFFE;
    // bgCameraTileY only changes when the camera moves 16 pixels vertically.  & 0xFFFE means it will always be an even number.
    bgCameraTileY = PIXEL_TO_TILE(bgCameraPixelY) & 0xFFFE;
}
