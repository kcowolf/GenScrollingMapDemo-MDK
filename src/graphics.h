/* Autogenerated by GenImageTool */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

extern const uint16_t PAL_BG[16];

extern const uint16_t PAL_FG[16];

#define TILESET_BG_TILE_COUNT 934
extern const uint32_t TILESET_BG[TILESET_BG_TILE_COUNT][8];

#define TILESET_FG_TILE_COUNT 40
extern const uint32_t TILESET_FG[TILESET_FG_TILE_COUNT][8];

#define TILEMAP_BG_TILE_WIDTH 60
#define TILEMAP_BG_TILE_HEIGHT 47
#define TILEMAP_BG_PIXEL_WIDTH 480
#define TILEMAP_BG_PIXEL_HEIGHT 376
#define TILEMAP_BG_TILE_COUNT 2820
extern const uint16_t TILEMAP_BG[TILEMAP_BG_TILE_COUNT];

#define TILEMAP_FG_TILE_WIDTH 80
#define TILEMAP_FG_TILE_HEIGHT 66
#define TILEMAP_FG_PIXEL_WIDTH 640
#define TILEMAP_FG_PIXEL_HEIGHT 528
#define TILEMAP_FG_TILE_COUNT 5280
extern const uint16_t TILEMAP_FG[TILEMAP_FG_TILE_COUNT];

#endif
