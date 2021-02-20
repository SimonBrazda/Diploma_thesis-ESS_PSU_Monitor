#pragma once
#include <Arduino.h>

#include "config.h"

class Quantity {
private:
    float value;
    String unit;

public:
    Quantity(const float& value, const String& unit) : value(value), unit(unit) {}
    
    ~Quantity() {}

    float get_value() const {
        return value;
    }

    String get_unit() const {
        return unit;
    }
};