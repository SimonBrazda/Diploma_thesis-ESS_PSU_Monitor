#pragma once
#include <Arduino.h>
#include <SD.h>
#include <Wire.h>

#include <RtcDS1307.h>

#include "Config.h"
#include "SerialBoth.h"

extern RtcDS1307<TwoWire> rtc;

namespace SdUtility {
    // Initialize SD card
    bool InitSd(size_t count);

    // DateTime callback to set datetime of a file
    void dateTime(uint16_t* o_date, uint16_t* o_time);
}