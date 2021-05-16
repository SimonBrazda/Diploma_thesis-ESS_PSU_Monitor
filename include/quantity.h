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