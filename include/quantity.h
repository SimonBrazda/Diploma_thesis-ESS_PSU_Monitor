#pragma once
#include <Arduino.h>

#include "config.h"

class Quantity {
protected:
    float value;
    String unit;
    Eval eval;

public:
    Quantity(const float& value, const String& unit);
    ~Quantity();

    float get_value() const;
    String get_unit() const;
    Eval get_eval() const;
};