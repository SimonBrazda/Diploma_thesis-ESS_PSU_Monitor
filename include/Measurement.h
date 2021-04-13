#pragma once
#include <Arduino.h>
#include <RtcDateTime.h>

#include "Config.h"
#include "DateTimeFormatter.h"
#include "SerialBoth.h"

namespace Measurement {
    #if DEBUG == 0
    static float MeasureVoltage(const uint32_t pin, const Config& conf) {
        auto rawVoltage = analogRead(pin);
        float voltage = conf.referenceVoltage / conf.resolution * rawVoltage;
        float converted_voltage = voltage * (conf.voltageR1 + conf.voltageR2) / conf.voltageR2;
        float calibrated_converted_voltage = converted_voltage + conf.voltageCalibration;
        return calibrated_converted_voltage >= 0 ? calibrated_converted_voltage : 0;
    }

    static float MeasureCurrent(const uint32_t pin, const Config& conf) {
        auto rawCurrentVoltage = analogRead(pin);
        float currentVoltage = conf.referenceVoltage / conf.resolution * rawCurrentVoltage;
        float convertedCurrentVoltage = currentVoltage * (conf.currentR1 + conf.currentR2) / conf.currentR2;
        float calibratedConvertedCurrentVoltage = convertedCurrentVoltage + conf.currentVoltageCalibration;
        float current = calibratedConvertedCurrentVoltage / conf.sensitivity;
        float calibratedCurrent = current + conf.currentCalibration;
        return calibratedCurrent > 0 ? calibratedCurrent : 0;
    }
    #endif

    #if DEBUG == 1
    template<typename O>
    static float DebugMeasureVoltage(O& out, const uint32_t pin, const Config& conf) {
        auto rawVoltage = analogRead(pin);
        float voltage = conf.referenceVoltage / conf.resolution * rawVoltage;
        float converted_voltage = voltage * (conf.voltageR1 + conf.voltageR2) / conf.voltageR2;
        float calibrated_converted_voltage = converted_voltage + conf.voltageCalibration;

        // Print all steps of the measurement to the display output
        out.setCursor(0, 1);
        out.print(rawVoltage);
        out.setCursor(0, 2);
        out.print(String(voltage) + " V");
        out.setCursor(0, 3);
        out.print(String(converted_voltage) + " V");
        out.setCursor(0, 4);
        out.print(String(calibrated_converted_voltage) + " V");

        // Print all steps to Serial and SerialUSB
        SerialBoth::println("A/D voltage value: " + String(rawVoltage));
        SerialBoth::println("Voltage: " + String(voltage) + " V");
        SerialBoth::println("Converted voltage: " + String(converted_voltage) + " V");
        SerialBoth::println("Calibrated converted voltage: " + String(voltage) + " V");

        return calibrated_converted_voltage >= 0 ? calibrated_converted_voltage : 0;
    }

    template<typename O>
    static float DebugMeasureCurrent(O& out, const uint32_t pin, const Config& conf) {
        auto rawCurrentVoltage = analogRead(pin);
        float currentVoltage = conf.referenceVoltage / conf.resolution * rawCurrentVoltage;
        float convertedCurrentVoltage = currentVoltage * (conf.currentR1 + conf.currentR2) / conf.currentR2;
        float calibratedConvertedCurrentVoltage = convertedCurrentVoltage + conf.currentVoltageCalibration;
        float current = calibratedConvertedCurrentVoltage / conf.sensitivity;
        float calibratedCurrent = current + conf.currentCalibration;

        // Print all steps of the measurement to the display output
        out.setCursor(0, 5);
        out.print(rawCurrentVoltage);
        out.setCursor(0, 6);
        out.print(String(currentVoltage) + " V");
        out.setCursor(0, 7);
        out.print(String(convertedCurrentVoltage) + " V");
        out.setCursor(0, 8);
        out.print(String(calibratedConvertedCurrentVoltage) + " V");
        out.setCursor(0, 9);
        out.print(String(current) + " A");
        out.setCursor(0, 10);
        out.print(String(calibratedCurrent) + " A");
        out.setCursor(0, 11);

        // Print all steps to Serial and SerialUSB
        SerialBoth::println("A/D current voltage value: " + String(rawCurrentVoltage));
        SerialBoth::println("Current voltage: " + String(currentVoltage) + " V");
        SerialBoth::println("Converted current voltage: " + String(convertedCurrentVoltage) + " V");
        SerialBoth::println("Calibrated converted current voltage: " + String(calibratedConvertedCurrentVoltage) + " V");
        SerialBoth::println("Current: " + String(current) + " A");
        SerialBoth::println("Calibrated current: " + String(calibratedCurrent) + " A");

        return calibratedCurrent > 0 ? calibratedCurrent : 0;
    }
    #endif

    static float Power(const float voltage, const float current) {
        return voltage * current;
    }

    static float Consumption(const float power, const Config& conf) {
        return power / conf.measurementDelay / 1000 * 3600;
    }

    #if DEBUG == 0
    template<typename O>
    static void print(O& out, const RtcDateTime& now, size_t index) {
        out.setCursor(0, index);
        out.print(DateTimeFormatter::GetTime(now));
        #if SERIAL_DEBUG == 1
        SerialBoth::println(DateTimeFormatter::GetTime(now));
        SerialBoth::println();
        #endif
        out.gfx.setTextColor(White, Black);
    }

    template<typename O, typename T, typename... Args>
    static void print(O& out, const RtcDateTime& now, size_t index, T& arg, Args... args) {
        switch (arg.get_eval()) {
        case Eval::Low:
            out.gfx.setTextColor(Yellow, Black);
            break;
        case Eval::High:
            out.gfx.setTextColor(Red, Black);
            break;
        default:
            break;
        }

        out.setCursor(0, index);
        out.print(String(arg.get_value()) + " " + String(arg.get_unit()));
        #if SERIAL_DEBUG == 1
        SerialBoth::println(String(arg.get_value()) + " " + String(arg.get_unit()));
        #endif
        out.gfx.setTextColor(White, Black);
        print(out, now, index + 1, args...);
    }
    #endif
};