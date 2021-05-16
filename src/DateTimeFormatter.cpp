#include "DateTimeFormatter.h"

String DateTimeFormatter::GetDateTime(const RtcDateTime& dt) {
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return String(datestring);
}

String DateTimeFormatter::GetDate(const RtcDateTime& dt) {
    char datestring[11];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u/%02u/%02u"),
            dt.Year(),
            dt.Month(),
            dt.Day() );
    return String(datestring);
}

String DateTimeFormatter::GetTime(const RtcDateTime& dt) {
    char datestring[9];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return String(datestring);
}