#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "9f1c0d2a-2b5a-4e2b-8d37-0a4e1f6b9001"
#define SETTINGS_CHAR_UUID  "9f1c0d2a-2b5a-4e2b-8d37-0a4e1f6b9002"

volatile int App_BackgroundStyle = 0;
volatile int App_MeasureStyle = 0;
volatile int App_NeedleStyle = 0;
volatile bool App_SettingsUpdated = false;

bool deviceConnected = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    deviceConnected = true;
    Serial.println("Phone connected");
  }

  void onDisconnect(BLEServer* server) override {
    deviceConnected = false;
    Serial.println("Phone disconnected");
    server->startAdvertising();
  }
};

class SettingsCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
    String value = characteristic->getValue();

    Serial.print("Received: ");
    Serial.println(value);

    int bgIndex = value.indexOf("\"bg\":");
    int measureIndex = value.indexOf("\"measure\":");
    int needleIndex = value.indexOf("\"needle\":");

    if (bgIndex >= 0) {
      App_BackgroundStyle = value.substring(bgIndex + 5).toInt();
    }

    if (measureIndex >= 0) {
      App_MeasureStyle = value.substring(measureIndex + 10).toInt();
    }

    if (needleIndex >= 0) {
      App_NeedleStyle = value.substring(needleIndex + 9).toInt();
    }

    App_SettingsUpdated = true;

    Serial.print("BG: ");
    Serial.print(App_BackgroundStyle);
    Serial.print(" | Measure: ");
    Serial.print(App_MeasureStyle);
    Serial.print(" | Needle: ");
    Serial.println(App_NeedleStyle);
  }
};

void BLE_App_Init(void)
{
  BLEDevice::init("MX5 Gauge");

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(SERVICE_UUID);

  BLECharacteristic* settingsCharacteristic = service->createCharacteristic(
    SETTINGS_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );

  settingsCharacteristic->setCallbacks(new SettingsCallbacks());

  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();
}