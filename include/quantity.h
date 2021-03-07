#pragma once
#include <Arduino.h>

#include "config.h"

class Quantity {
protected:
    float value;
    String unit;
    Eval eval;

    Eval _evaluate(double min_value, double max_value) {
        if (value < min_value) return Eval::Low;
        if (value > max_value) return Eval::High;
        return Eval::Fine;
    }

public:
    Quantity(const float& value, const String& unit) : value(value), unit(unit), eval(Eval::None) {}
    
    ~Quantity() {}

    void evaluate(double min_value, double max_value) {
        eval = _evaluate(min_value, max_value);
    }

    float get_value() const {
        return value;
    }

    String get_unit() const {
        return unit;
    }

    Eval get_eval() const { return eval; }
};