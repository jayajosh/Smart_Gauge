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

void GPS_Init(void)
{
    GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

static void parseRMC(String line)
{
    // Example: $GNRMC,...
    int field = 0;
    String parts[12];

    for (int i = 0; i < line.length(); i++) {
        if (line[i] == ',' || i == line.length() - 1) {
            parts[field++] = line.substring(0, i);
            line = line.substring(i + 1);
            i = 0;
        }
    }

    // RMC format:
    // [2] = A/V (valid)
    // [7] = speed in knots

    if (parts[2] == "A") {
        GPS_Fix = true;

        float knots = parts[7].toFloat();
        GPS_Speed_KMH = knots * 1.852;
        GPS_Speed_MPH = knots * 1.15078;
    } else {
        GPS_Fix = false;
        GPS_Speed_KMH = 0;
        GPS_Speed_MPH = 0;
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