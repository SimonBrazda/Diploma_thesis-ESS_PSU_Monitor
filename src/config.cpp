    #include "Config.h"

    void Config::print() {
        Serial.println("maxVoltage " + String(maxVoltage));
        Serial.println("minVoltage " + String(minVoltage));
        Serial.println("maxCurrent " + String(maxCurrent));
        Serial.println("minCurrent " + String(minCurrent));
        Serial.println("measurementDelay " + String(measurementDelay));
        Serial.println("sensitivity " + String(sensitivity));
        Serial.println("voltageR1 " + String(voltageR1));
        Serial.println("voltageR2 " + String(voltageR2));
        Serial.println("currentR1 " + String(currentR1));
        Serial.println("currentR2 " + String(currentR2));
        Serial.println("resolution " + String(resolution));
        Serial.println("referenceVoltage " + String(referenceVoltage));
        Serial.println("currentCalibration " + String(currentCalibration));
        Serial.println("currentVoltageCalibration " + String(currentVoltageCalibration));
        Serial.println("calibrationVoltage " + String(voltageCalibration));
        Serial.println("displayRotation " + String(displayRotation));
        Serial.println("timeToSleep " + String(timeToSleep));
    }