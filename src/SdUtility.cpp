// A Module for Power Supply Analysis of Electronic Security Systems
// Copyright (C) 2021 by Šimon Brázda <simonbrazda@seznam.cz>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "SdUtility.h"

// Initialize SD card
bool SdUtility::InitSd(size_t count) {
    for (byte i{}; i < count; i++) {
        if(SD.begin(SD_CS) == false) {
            #if DEBUG == 1
            SerialBoth::println();
            SerialBoth::println("Failed to mount SD");
            SerialBoth::println("Try " + String(i + 1) + "/" + String(count));
            #endif
        } else {
            #if DEBUG == 1
            SerialBoth::println();
            SerialBoth::println("SD card mounted successfully");
            #endif
            return true;
        }
    }
    #if DEBUG == 1
    SerialBoth::println();
    SerialBoth::println("Could not mount SD card");
    #endif
    return false;
}

// DateTime callback to set datetime of a file
void SdUtility::dateTime(uint16_t* o_date, uint16_t* o_time) {
    RtcDateTime now = rtc.GetDateTime();
    *o_date = FAT_DATE(now.Year(), now.Month(), now.Day());
    *o_time = FAT_DATE(now.Hour(), now.Minute(), now.Second());
}