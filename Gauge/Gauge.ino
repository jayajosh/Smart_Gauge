/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

//#include "Wireless.h"
//#include "Gyro_QMI8658.h"
//#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
//#include "BAT_Driver.h"
#include "GPS_Driver.h"
#include "BLE_App_Driver.h"
#include "Map_Driver.h"
#include "ui.h"
#include <SD_MMC.h>

enum SpeedoStyle { Cat = 0, Fire = 1, Hypersport = 2};

SpeedoStyle backgroundStyle = Cat;
SpeedoStyle measureStyle = Cat;
SpeedoStyle needleStyle = Cat;

bool mapLoaded = false;

lv_obj_t * bleStatusSquare;

const void* backgroundImages[] = {
    &ui_img_cat_background2x_png,
    &ui_img_fire_baclground2x_png,
};

const void* measureImages[] = {
    &ui_img_cat_measure2x_png,
    &ui_img_fire_measure2x_png,
};

const void* needleImages[] = {
    &ui_img_cat_needle2x_png,
    &ui_img_fire_needle2x_png,
};

void Driver_Loop(void *parameter)
{
  while(1)
  {
    //QMI8658_Loop();
    //RTC_Loop();
    //BAT_Get_Volts();

    GPS_Loop();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void Driver_Init()
{
  //Flash_test();
  //BAT_Init();
  I2C_Init();
  TCA9554PWR_Init(0x00);   
  Set_EXIO(EXIO_PIN8,Low);
  //PCF85063_Init();
  //QMI8658_Init(); 
  BLE_App_Init();

  GPS_Init();
  
  xTaskCreatePinnedToCore(
    Driver_Loop,     
    "Other Driver task",   
    4096,                
    NULL,                 
    3,                    
    NULL,                
    0                    
  );
}

void updateSpeedometerScreenGraphics()
{
    lv_img_set_src(ui_Background, backgroundImages[backgroundStyle]);
    lv_img_set_src(ui_Measure, measureImages[measureStyle]);
    lv_img_set_src(ui_Needle, needleImages[needleStyle]);
}

float getStableSpeed(float rawMph)
{
    static float smoothedSpeed = 0.0f;

    if (rawMph < 1.5f) {
        rawMph = 0.0f;
    }

    smoothedSpeed = (smoothedSpeed * 0.6f) + (rawMph * 0.4f);

    if (smoothedSpeed < 1.0f) {
        smoothedSpeed = 0.0f;
    }

    return smoothedSpeed;
}

void updateSpeedDisplay()
{
    static uint32_t lastUpdate = 0;
    static uint32_t tick = 0;

    if (millis() - lastUpdate < 50) return;
    lastUpdate = millis();
    tick++;

    char speedStr[32];

    if(GPS_Fix) {
        float stableSpeed = getStableSpeed(GPS_Speed_MPH);
        snprintf(speedStr, sizeof(speedStr), "%.0f", stableSpeed);

        int angle = 0 + (int)(stableSpeed * 18);
        lv_img_set_angle(ui_Needle, angle);
    } else {
        snprintf(speedStr, sizeof(speedStr), "GPS Unavailable");
    }

    lv_label_set_text(ui_SpeedLabel, speedStr);
}

void clearScreen2ForMap()
{
  lv_obj_add_flag(ui_Image1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_Background1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_Measure1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_Needle1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_Image2, LV_OBJ_FLAG_HIDDEN);

  lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui_Screen2, LV_OPA_COVER, LV_PART_MAIN);
}

static void screen_gesture_cb(lv_event_t * e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    if(lv_scr_act() == ui_SpeedometerScreen && dir == LV_DIR_LEFT) {

    Serial.println("SWIPE LEFT TO MAP");

    clearScreen2ForMap();

    if (!mapLoaded) {
      Serial.println("INIT MAP");
      Map_Init(ui_Screen2);
      Map_ShowTileGrid(16, 32363, 21355);
      mapLoaded = true;
    }

    lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
  }
}

void createBleStatusSquare()
{
  bleStatusSquare = lv_obj_create(ui_SpeedometerScreen);

  lv_obj_set_size(bleStatusSquare, 16, 16);

  // Lower-right area, but still inside the round screen
  lv_obj_align(bleStatusSquare, LV_ALIGN_CENTER, 125, 125);

  lv_obj_clear_flag(bleStatusSquare, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_style_radius(bleStatusSquare, 3, LV_PART_MAIN);
  lv_obj_set_style_border_width(bleStatusSquare, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(bleStatusSquare, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(bleStatusSquare, lv_color_hex(0xFF0000), LV_PART_MAIN);

  lv_obj_move_foreground(bleStatusSquare);
}

void updateBleStatusSquare()
{
  if (bleStatusSquare == NULL) return;

  static int lastState = -1;
  int currentState = App_DeviceConnected ? 1 : 0;

  if (currentState == lastState) return;
  lastState = currentState;

  lv_obj_set_style_bg_color(
    bleStatusSquare,
    App_DeviceConnected ? lv_color_hex(0x007BFF) : lv_color_hex(0xFF0000),
    LV_PART_MAIN
  );

  lv_obj_move_foreground(bleStatusSquare);
}

void setup()
{
  //Wireless_Test2();
  Driver_Init();
  LCD_Init();                                     
  SD_Init();                                    
  Lvgl_Init();

  ui_init();



  lv_obj_add_event_cb(ui_SpeedometerScreen, screen_gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(ui_Screen2, screen_gesture_cb, LV_EVENT_GESTURE, NULL);


  Serial.begin(115200);

  createBleStatusSquare();
  // lv_demo_widgets();               
  // lv_demo_benchmark();          
  // lv_demo_keypad_encoder();     
  // lv_demo_music();              
  // lv_demo_printer();
  // lv_demo_stress();   
}

static int fakeOffset = 0;
static uint32_t lastMapMove = 0;

void loop()
{
  updateBleStatusSquare();
  if (App_SettingsUpdated) {
      App_SettingsUpdated = false;

      backgroundStyle = (SpeedoStyle)App_BackgroundStyle;
      measureStyle = (SpeedoStyle)App_MeasureStyle;
      needleStyle = (SpeedoStyle)App_NeedleStyle;

      updateSpeedometerScreenGraphics();
  }
  updateSpeedDisplay();
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));



  if (mapLoaded == true) {
    if (millis() - lastMapMove > 30) {
      lastMapMove = millis();

      fakeOffset = (fakeOffset + 1) % 256;
      Map_MovePixels(fakeOffset, 0);
    }
  }

}
