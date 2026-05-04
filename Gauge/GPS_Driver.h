#pragma once
#include <Arduino.h>

extern float GPS_Speed_MPH;
extern float GPS_Speed_KMH;
extern bool GPS_Fix;

void GPS_Init(void);
void GPS_Loop(void);