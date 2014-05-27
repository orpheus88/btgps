/*************************************************************************
* Arduino OBD-II/G-Force Data Logger and Transmitter
* Distributed under GPL v2.0
* Copyright (c) 2013-14 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*************************************************************************/

#include <Arduino.h>
#include <SD.h>
#include <OBD.h>
#include <Wire.h>
#include <SPI.h>
#include "config.h"
#if USE_SOFTSERIAL
#include <SoftwareSerial.h>
#endif
#include "btgps.h"

// logger states
#define STATE_GPS_FOUND 0x4
#define STATE_GPS_READY 0x8
#define STATE_SLEEPING 0x20

#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F\r"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C\r"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F\r"

class COBDLogger : public COBD, public CDataLogger
{
public:
    COBDLogger():state(0) {}
    void setup()
    {
        state = 0;
#if USE_GPS
        if (initGPS()) {
            state |= STATE_GPS_FOUND;
        }
#endif
    }
#if USE_GPS
    void logGPSData()
    {
        char lastChar;
        // issue the command to get NMEA data (one line per request)
        write("ATGRR\r");
        dataTime = millis();
        for (;;) {
            if (available()) {
                char c = read();
                if (c == '>') {
                    // prompt char received
                    break;
                } else {
                    logData(c);
                    lastChar = c;
                }
            } else if (millis() - dataTime > 100) {logData('t');
                // timeout
                break;
            }
        }
#if LOG_GPS_NMEA_DATA
        if (lastChar != '\r') {
            logData('\r');
        }
#endif
    }
#endif

#if USE_GPS
    bool initGPS()
    {
     //   char buf[OBD_RECV_BUF_SIZE];
        // setting GPS baudrate
        write("ATBR2 38400\r");
        receive();
        return true;
    }
#endif
    byte state;
private:

};

static COBDLogger logger;

void setup()
{
    logger.begin();
    logger.initSender();
    logger.setup();
}

void loop()
{
    uint32_t t = millis();
#if USE_GPS
    if (logger.state & STATE_GPS_FOUND) {
        logger.logGPSData();
    }
#endif

#if LOOP_INTERVAL
    // get time elapsed
    t = millis() - t;
    if (t < LOOP_INTERVAL) {
        delay(LOOP_INTERVAL - t);
    }
#endif
}
