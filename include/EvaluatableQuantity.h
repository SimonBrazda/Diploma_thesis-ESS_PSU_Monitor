#pragma once
#include "Quantity.h"

class EvaluatableQuantity : public Quantity {
protected:
    float min{};
    float max{};

public:
    EvaluatableQuantity(const float& value, const String& unit, float min, float max);
    ~EvaluatableQuantity();

    void Evaluate(double min_value, double max_value);
};