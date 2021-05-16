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

// Methods of this header are copies of SparkFun_External_EEPROM_Arduino_Library <https://github.com/sparkfun/SparkFun_External_EEPROM_Arduino_Library> with slight modifications.

#pragma once
#include <Arduino.h>
#include <I2C_eeprom.h>

#include "SerialBoth.h"

class EEPROM : public I2C_eeprom
{
public:
    EEPROM(const uint8_t deviceAddress, const uint32_t deviceSize, TwoWire *wire = &Wire) : I2C_eeprom(deviceAddress, deviceSize, wire) {};
    ~EEPROM() {};

    // Read any data type from EEPROM.
    template <typename T>
    T &get(uint16_t idx, T &t) {
        uint8_t *ptr = (uint8_t *)&t;
        readBlock(idx, ptr, sizeof(T)); // Address, data, sizeOfData
        return t;
    }

    // Write any data type to EEPROM.
    template <typename T>
    const T &put(uint16_t idx, const T &t) {
        const uint8_t *ptr = (const uint8_t *)&t;
        updateBlock(idx, ptr, sizeof(T)); // Address, data, sizeOfData
        return t;
    }

    // Dump EEPROM memory to Serials
    void dumpEeprom(uint16_t memoryAddress, uint16_t length) {
        const int BLOCK_TO_LENGTH = 10;
        
        SerialBoth::print("\t  ");
        for (int x = 0; x < 10; x++)
        {
            if (x != 0) SerialBoth::print("    ");
            SerialBoth::print(x);
        }
        SerialBoth::println();

        // block to defined length
        memoryAddress = memoryAddress / BLOCK_TO_LENGTH * BLOCK_TO_LENGTH;
        length = (length + BLOCK_TO_LENGTH - 1) / BLOCK_TO_LENGTH * BLOCK_TO_LENGTH;

        byte b = readByte(memoryAddress);
        for (unsigned int i = 0; i < length; i++)
        {
            char buf[6];
            if (memoryAddress % BLOCK_TO_LENGTH == 0)
            {
            if (i != 0) SerialBoth::println();
            sprintf(buf, "%05d", memoryAddress);
            SerialBoth::print(buf);
            SerialBoth::print(":\t");
            }
            sprintf(buf, "%03d", b);
            SerialBoth::print(buf);
            b = readByte(++memoryAddress);
            SerialBoth::print("  ");
        }
        SerialBoth::println();
    }
};