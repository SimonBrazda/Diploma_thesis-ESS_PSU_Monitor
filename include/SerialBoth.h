#pragma once
#include <Arduino.h>

// Wrapper class for Serial and SerialUSB to call defined methonds on both of them
struct SerialBoth {
    template<typename T>
    static void println(const T& msg) {
        Serial.println(msg);
        SerialUSB.println(msg);
    }

    static void println();

    template<typename T>
    static void print(const T& msg) {
        Serial.print(msg);
        SerialUSB.print(msg);
    }
};