#include "SdUtility.h"

// Initialize SD card
bool SdUtility::InitSd(size_t count) {
    for (byte i{}; i < count; i++) {
        if(SD.begin(SD_CS) == false) {
            #if DEBUG == 1
            SerialBoth::println();
            SerialBoth::println("Failed to mount SD");
            SerialBoth::println("Try " + String(i) + "/" + String(count));
            #endif
            delay(1000);
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