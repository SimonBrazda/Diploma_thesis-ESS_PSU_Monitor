#include <Arduino.h>

#include "SerialBoth.h"

void SerialBoth::println() {
    Serial.println();
    SerialUSB.println();
}