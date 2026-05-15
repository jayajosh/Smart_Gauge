#pragma once
#include <Arduino.h>

extern volatile bool App_SettingsUpdated;
extern volatile int App_BackgroundStyle;
extern volatile int App_MeasureStyle;
extern volatile int App_NeedleStyle;
extern volatile bool App_DeviceConnected;

extern bool routeReady;
extern String fullRouteString;

void BLE_App_Init(void);