#pragma once

#include <Arduino.h>
#include <SD.h>

#include "Measurement.h"
#include "SdUtility.h"
#include "EvaluatableQuantity.h"

struct MyMeasurement {
    const EvaluatableQuantity& voltage;
    const EvaluatableQuantity& current;
    const Quantity& power;
    const Quantity& consumption;
    const RtcDateTime& now;

    MyMeasurement(const EvaluatableQuantity& voltage,
                  const EvaluatableQuantity& current,
                  const Quantity& power,
                  const Quantity& consumption,
                  const RtcDateTime& now);

    bool LogToSd();
};