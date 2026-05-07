#include "GPS_Driver.h"

HardwareSerial GPS(1);

// Adjust these to your wiring
#define GPS_RX_PIN 44
#define GPS_TX_PIN 43
#define GPS_BAUD   38400

float GPS_Speed_MPH = 0.0;
float GPS_Speed_KMH = 0.0;
bool GPS_Fix = false;

static String nmeaLine = "";

const uint8_t UBX_SET_RATE_10HZ[] = {
    0xB5, 0x62,
    0x06, 0x08,
    0x06, 0x00,
    0x64, 0x00, // 100ms = 10Hz
    0x01, 0x00,
    0x01, 0x00,
    0x7A, 0x12
};


void GPS_Init(void)
{
    GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(200);

    GPS.write(UBX_SET_RATE_10HZ, sizeof(UBX_SET_RATE_10HZ));
}

static void parseRMC(const String& line)
{
    String parts[20];
    int partIndex = 0;
    int start = 0;

    for (int i = 0; i <= line.length(); i++) {
        if (i == line.length() || line[i] == ',') {
            parts[partIndex++] = line.substring(start, i);
            start = i + 1;
            if (partIndex >= 20) break;
        }
    }

    // RMC:
    // parts[2] = A/V valid status
    // parts[7] = speed in knots

    if (partIndex > 7 && parts[2] == "A") {
        GPS_Fix = true;

        float knots = parts[7].toFloat();
        GPS_Speed_KMH = knots * 1.852f;
        GPS_Speed_MPH = knots * 1.15078f;
    } else {
        GPS_Fix = false;
        GPS_Speed_KMH = 0.0f;
        GPS_Speed_MPH = 0.0f;
    }
}

void GPS_Loop(void)
{
    while (GPS.available()) {
        char c = GPS.read();

        if (c == '\n') {
            if (nmeaLine.startsWith("$GNRMC") || nmeaLine.startsWith("$GPRMC")) {
                parseRMC(nmeaLine);
            }
            nmeaLine = "";
        } else if (c != '\r') {
            nmeaLine += c;
        }
    }
}