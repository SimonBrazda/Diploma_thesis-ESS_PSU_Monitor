    #include <config.h>

    void Config::print() {
        Serial.println("conf_in_eeprom " + String(conf_in_eeprom));
        Serial.println("n_calibrations " + String(n_calibrations));
        Serial.println("max_voltage " + String(max_voltage));
        Serial.println("min_voltage " + String(min_voltage));
        Serial.println("max_current " + String(max_current));
        Serial.println("min_current " + String(min_current));
        Serial.println("measurement_delay " + String(measurement_delay));
        Serial.println("sensitivity " + String(sensitivity));
        Serial.println("R1 " + String(R1));
        Serial.println("R2 " + String(R2));
        Serial.println("resolution " + String(resolution));
        Serial.println("reference_voltage " + String(reference_voltage));
        Serial.println("calibration_voltage " + String(calibration_voltage));
        Serial.println("\tDone...");
    }