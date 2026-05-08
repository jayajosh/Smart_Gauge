#pragma once
#include <Arduino.h>
#include <lvgl.h>

bool Map_Init(lv_obj_t * parent);
bool Map_LoadTile(const char* path);
void Map_ShowTestTile(void);