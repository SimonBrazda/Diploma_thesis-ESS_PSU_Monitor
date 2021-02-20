#pragma once
#include "config.h"

class MeasurementTemplate {
public:
    // void measure_PSU_values() {
        // this->measure_voltage();
        // this->measure_current();
        // this->measure_power();
        // consumption = this->measure_consumption();
    // }
    virtual ~MeasurementTemplate() {};
    virtual float measure_voltage(uint32_t pin, const Config& conf) const;
    virtual float measure_current(uint32_t pin, const Config& conf) const;
    virtual float measure_power(const float& voltage, const float& current) const;

    // TODO: Implement this method
    // virtual double measure_consumption() const = 0;
};