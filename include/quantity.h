#pragma once
#include <Arduino.h>

#include "config.h"

class Quantity {
protected:
    float value;
    String unit;
    Eval eval;

public:
    Quantity(const float& value, const String& unit) : value(value), unit(unit), eval(Eval::None) {}
    
    ~Quantity() {}

    float get_value() const {
        return value;
    }

    String get_unit() const {
        return unit;
    }

    Eval get_eval() const { return eval; }
};

class EvaluatableQuantity : public Quantity {
protected:
    float min{};
    float max{};

public:
    EvaluatableQuantity(const float& value, const String& unit, float min, float max) :
            Quantity{ value, unit },
            min{ min },
            max{ max } {
        evaluate(min, max);
    }
    
    ~EvaluatableQuantity() {}

    void evaluate(double min_value, double max_value) {
        if (value < min_value) eval = Eval::Low;
        else if (value > max_value) eval =  Eval::High;
        else eval = Eval::Fine;
    }
};