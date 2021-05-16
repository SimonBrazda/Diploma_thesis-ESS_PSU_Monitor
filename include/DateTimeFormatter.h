#pragma once
#include <Arduino.h>
#include <RtcDateTime.h>

#define countof(a) (sizeof(a) / sizeof(a[0])) // Number of elements in the variable 

struct DateTimeFormatter {
    static String GetDateTime(const RtcDateTime& dt);
    static String GetDate(const RtcDateTime& dt);
    static String GetTime(const RtcDateTime& dt);
};