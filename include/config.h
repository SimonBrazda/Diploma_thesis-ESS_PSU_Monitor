#pragma once
#include <Arduino.h>

struct Config {
    uint8_t conf_in_eeprom = 0;
    unsigned int n_calibrations = 40;
    float max_voltage = 15.00F;
    float min_voltage = 11.50F;
    float max_current = 10.00F;
    float min_current = 0.10F;
    unsigned int measurement_delay = 1000;
    float sensitivity = 0.185F;
    unsigned int R1 = 180000;
    unsigned int R2 = 47000;
    unsigned short resolution = 4096;
    float reference_voltage = 3.30F;
    float calibration_voltage = 0.39F;

    void print();
};

enum Eval{ None = 0, Low, Fine, High };
// String eval_name[] = {"Low", "Fine", "High"};