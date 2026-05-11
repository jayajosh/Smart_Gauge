#pragma once
#include <Arduino.h>
#include <lvgl.h>

bool Map_Init(lv_obj_t * parent);
void Map_ShowTileGrid(int zoom, int centerX, int centerY);
void Map_SetOffset(int offsetX, int offsetY);
void Map_MovePixels(int dx, int dy);
void Map_CreateGpsMarker(void);