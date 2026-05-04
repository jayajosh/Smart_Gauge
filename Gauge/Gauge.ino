/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

//#include "Wireless.h"
//#include "Gyro_QMI8658.h"
//#include "RTC_PCF85063.h"
//#include "SD_Card.h"
#include "LVGL_Driver.h"
//#include "BAT_Driver.h"
#include "GPS_Driver.h"
#include "BLE_App_Driver.h"
#include "ui.h"

#define LED_R 10
#define LED_G 11
#define LED_B 12

enum SpeedoStyle { Cat = 0, Fire = 1, Hypersport = 2};

SpeedoStyle backgroundStyle = Cat;
SpeedoStyle measureStyle = Cat;
SpeedoStyle needleStyle = Cat;


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

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
void Driver_Init()
{
  //Flash_test();
  //BAT_Init();
  I2C_Init();
  //TCA9554PWR_Init(0x00);   
  //Set_EXIO(EXIO_PIN8,Low);
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

void updateSpeedDisplay()
{
    static uint32_t lastUpdate = 0;
    static uint32_t tick = 0;

    if (millis() - lastUpdate < 200) return;
    lastUpdate = millis();
    tick++;

    char speedStr[32];

    if(GPS_Fix) {
        snprintf(speedStr, sizeof(speedStr), "%.1f", GPS_Speed_MPH);
    } else {
        snprintf(speedStr, sizeof(speedStr), "GPS Unavailable");
    }

    lv_label_set_text(ui_SpeedLabel, speedStr);

    // static uint32_t lastUpdate = 0;

    // if(millis() - lastUpdate < 100) return;
    // lastUpdate = millis();

    // char speedStr[16];

    // if(GPS_Fix) {
    //     snprintf(speedStr, sizeof(speedStr), "%.1f", GPS_Speed_MPH);
    //     lv_label_set_text(ui_SpeedLabel, speedStr);

    //     int angle = 2300 + (int)(GPS_Speed_MPH * 10); // placeholder mapping
    //     lv_img_set_angle(ui_Needle, angle);
    // } else {
    //     lv_label_set_text(ui_SpeedLabel, "--");
    // }
}

static void screen_gesture_cb(lv_event_t * e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    // Swipe LEFT → go to Screen2
    if(lv_scr_act() == ui_SpeedometerScreen && dir == LV_DIR_LEFT) {
        backgroundStyle = Fire;
        //updateSpeedometerScreenGraphics();
        lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    }

    // Swipe RIGHT → go back to SpeedometerScreen
    else if(lv_scr_act() == ui_Screen2 && dir == LV_DIR_RIGHT) {
        lv_scr_load_anim(ui_SpeedometerScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}

void setup()
{
  //Wireless_Test2();
  Driver_Init();
  LCD_Init();                                     // If you later reinitialize the LCD, you must initialize the SD card again !!!!!!!!!!
  //SD_Init();                                      // It must be initialized after the LCD, and if the LCD is reinitialized later, the SD also needs to be reinitialized
  Lvgl_Init();

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  void setLed(bool r, bool g, bool b)
  {
    digitalWrite(LED_R, r);
    digitalWrite(LED_G, g);
    digitalWrite(LED_B, b);
  }

  ui_init();

  lv_obj_add_event_cb(ui_SpeedometerScreen, screen_gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(ui_Screen2, screen_gesture_cb, LV_EVENT_GESTURE, NULL);
  // lv_demo_widgets();               
  // lv_demo_benchmark();          
  // lv_demo_keypad_encoder();     
  // lv_demo_music();              
  // lv_demo_printer();
  // lv_demo_stress();   
  
}

void loop()
{
    if (App_DeviceConnected) {
    setLed(false, false, true); // blue
  } else {
    setLed(true, false, false); // red
  }
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
}
