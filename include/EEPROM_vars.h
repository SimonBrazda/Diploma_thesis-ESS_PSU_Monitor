#pragma once
#include <Arduino.h>
#include <I2C_eeprom.h>

// DESCRIPTION: Extends functionality of Rob Tillaart's library by allowing client to easily read/write any data types from/to an EEPROM.

class EEPROM : public I2C_eeprom
{
public:
    EEPROM(const uint8_t deviceAddress, const uint32_t deviceSize, TwoWire *wire = &Wire) : I2C_eeprom(deviceAddress, deviceSize, wire) {};
    ~EEPROM() {};

    //Read any data type from EEPROM.
    template <typename T>
    T &get(uint16_t idx, T &t) {
        uint8_t *ptr = (uint8_t *)&t;
        readBlock(idx, ptr, sizeof(T)); //Address, data, sizeOfData
        return t;
    }

    //Write any data type from EEPROM.
    template <typename T>
    const T &put(uint16_t idx, const T &t) {
        const uint8_t *ptr = (const uint8_t *)&t;
        updateBlock(idx, ptr, sizeof(T)); //Address, data, sizeOfData
        return t;
    }

    void dumpEEPROM(uint16_t memoryAddress, uint16_t length) {
        const int BLOCK_TO_LENGTH = 10;
        
        Serial.print("\t  ");
        for (int x = 0; x < 10; x++)
        {
            if (x != 0) Serial.print("    ");
            Serial.print(x);
        }
        Serial.println();

        // block to defined length
        memoryAddress = memoryAddress / BLOCK_TO_LENGTH * BLOCK_TO_LENGTH;
        length = (length + BLOCK_TO_LENGTH - 1) / BLOCK_TO_LENGTH * BLOCK_TO_LENGTH;

        byte b = readByte(memoryAddress);
        for (unsigned int i = 0; i < length; i++)
        {
            char buf[6];
            if (memoryAddress % BLOCK_TO_LENGTH == 0)
            {
            if (i != 0) Serial.println();
            sprintf(buf, "%05d", memoryAddress);
            Serial.print(buf);
            Serial.print(":\t");
            }
            sprintf(buf, "%03d", b);
            Serial.print(buf);
            b = readByte(++memoryAddress);
            Serial.print("  ");
        }
        Serial.println();
    }
};