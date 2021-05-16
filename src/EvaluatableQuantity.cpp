// A Module for Power Supply Analysis of Electronic Security Systems
// Copyright (C) 2021 by Šimon Brázda <simonbrazda@seznam.cz>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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