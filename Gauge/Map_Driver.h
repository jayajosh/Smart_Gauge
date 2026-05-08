#pragma once
#include <Arduino.h>
#include <lvgl.h>

bool Map_Init(lv_obj_t * parent);
void Map_ShowTileGrid(int zoom, int centerX, int centerY);