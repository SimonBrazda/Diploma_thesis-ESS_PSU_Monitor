#include "EvaluatableQuantity.h"

EvaluatableQuantity::EvaluatableQuantity(const float& value, const String& unit, float min, float max) :
        Quantity{ value, unit },
        min{ min },
        max{ max } {
    Evaluate(min, max);
}

EvaluatableQuantity::~EvaluatableQuantity() {}

void EvaluatableQuantity::Evaluate(double min_value, double max_value) {
    if (value < min_value) eval = Eval::Low;
    else if (value > max_value) eval =  Eval::High;
    else eval = Eval::Fine;
}