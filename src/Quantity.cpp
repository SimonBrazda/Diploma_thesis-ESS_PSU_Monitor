#include "Quantity.h"

Quantity::Quantity(const float& value, const String& unit) : value(value), unit(unit), eval(Eval::None) {}
Quantity::~Quantity() {}
float Quantity::get_value() const { return value; }
String Quantity::get_unit() const { return unit; }
Eval Quantity::get_eval() const { return eval; }