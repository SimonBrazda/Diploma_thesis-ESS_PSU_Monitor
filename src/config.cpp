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

#include "Config.h"

void Config::print() {
    Serial.println("maxVoltage " + String(maxVoltage));
    Serial.println("minVoltage " + String(minVoltage));
    Serial.println("maxCurrent " + String(maxCurrent));
    Serial.println("minCurrent " + String(minCurrent));
    Serial.println("measurementDelay " + String(measurementDelay));
    Serial.println("sensitivity " + String(sensitivity));
    Serial.println("voltageR1 " + String(voltageR1));
    Serial.println("voltageR2 " + String(voltageR2));
    Serial.println("currentR1 " + String(currentR1));
    Serial.println("currentR2 " + String(currentR2));
    Serial.println("resolution " + String(resolution));
    Serial.println("referenceVoltage " + String(referenceVoltage));
    Serial.println("currentCalibration " + String(currentCalibration));
    Serial.println("currentVoltageCalibration " + String(currentVoltageCalibration));
    Serial.println("calibrationVoltage " + String(voltageCalibration));
    Serial.println("displayRotation " + String(displayRotation));
    Serial.println("timeToSleep " + String(timeToSleep));
}