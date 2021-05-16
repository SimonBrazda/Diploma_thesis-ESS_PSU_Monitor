#pragma once
#include <Arduino.h>

struct Config {
    // WARNING: maxVoltage, maxCurrent values only set when the display should change its output color
    // Maximum voltage that the device can handle is determined by the physical combination of rezistors in your voltage divider of the voltage measurement circuit
    // Maximum current that the device can handle is determined by the chosen current sensor, width of your routes and physical combination of rezistors in your voltage divider of the current measurement circuit

    float maxVoltage = 15.00F; // A maximum allowed measurement voltage [V]. Higher values will result in red color of the output
    float minVoltage = 11.50F; // A minumum allowed measurement voltage [V]. Lower values will result in red color of the output
    float maxCurrent = 10.00F; // A maximum allowed measurement voltage [V]. Higher values will result in red color of the output
    float minCurrent = 0.10F; // A minimum allowed measurement voltage [V]. Lower values will result in red color of the output
    unsigned int measurementDelay = 1000; // Delay between each measurement [ms]
    float sensitivity = 0.185F; // Sensitivity of current sensor in V/A
    unsigned int voltageR1 = 180200; // Rezistivity of the first rezistor in the voltage divider of the voltage measurement circuit [Ohm]
    unsigned int voltageR2 = 47100; // Rezistivity of the second rezistor in the voltage divider of the voltage measurement circuit [Ohm]
    unsigned int currentR1 = 62100; // Rezistivity of the first rezistor in the voltage divider of the current measurement circuit [Ohm]
    unsigned int currentR2 = 120300; // Rezistivity of the second rezistor in the voltage divider of the current measurement circuit [Ohm]
    unsigned int resolution = 4095; // Resolution of your A/D converter
    float referenceVoltage = 3.30F; // Reference voltage used be your A/D converter [V]
    float currentCalibration = 0.1652F; // Calibration value of measured current [A]
    float currentVoltageCalibration = -0.5F; // Calibration value of the output voltage of your current sensor [V]
    float voltageCalibration = -0.3586F; // Calibration value of voltage measurement [V]
    unsigned int displayRotation = 2; // Screen rotation oriantation of your display [0-3]
    unsigned int timeToSleep = 600;  // Minimum time of inactivity required to go into sleep state [s]

    void print();
};

// Enum for EvaluatableQuantity
enum Eval{ None = 0, Low, Fine, High };

// DEBUG set to 1 enables printing of useful debugging info to Serial and SerialUSB
// Set it to 0 if this behaviour is not desired
#define DEBUG 0

// Depth of the menu system
#define MAX_DEPTH 3

// Input pins
#define CURRENT_INPUT_PIN A9
#define VOLTAGE_INPUT_PIN A2

// Software debounce time for buttons
#define SOFT_DEBOUNCE_MS 100

// Font scaling ///////////////////////////////
#define TEXT_SCALE 3

#define FONT_WIDTH 6
#define FONT_HEIGHT 9
///////////////////////////////////////////////

// Last display line
#define LAST_DISPLAY_LINE 13

// Encoder /////////////////////////////////////
#define ENCODER_A 11
#define ENCODER_B 12
#define ENCODER_BUTTON 10
#define ENCODER_STEPS 4

// Colors //////////////////////////////////////
#ifndef RGB565
    #define RGB565(r,g,b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#endif

#define Black RGB565(0,0,0)
#define Red	RGB565(255,0,0)
#define Green RGB565(0,255,0)
#define Blue RGB565(0,0,255)
#define Gray RGB565(128,128,128)
#define LighterRed RGB565(255,150,150)
#define LighterGreen RGB565(150,255,150)
#define LighterBlue RGB565(150,150,255)
#define DarkerRed RGB565(150,0,0)
#define DarkerGreen RGB565(0,150,0)
#define DarkerBlue RGB565(0,0,150)
#define Cyan RGB565(0,255,255)
#define Magenta RGB565(255,0,255)
#define Yellow RGB565(255,255,0)
#define White RGB565(255,255,255)