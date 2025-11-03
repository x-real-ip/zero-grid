#pragma once

#include "esphome.h"

enum class OutputRange
{
    RANGE_0_5V,
    RANGE_0_10V
};

/**
 * Set DAC output range (0-5V or 0-10V)
 * Uses fixed register 0x01 for output range
 */
inline esphome::i2c::ErrorCode dac_set_output_range(
    esphome::i2c::I2CBus *bus,
    uint8_t address,
    OutputRange output)
{
    uint8_t reg = 0x01; // fixed register for output range
    uint8_t data;

    switch (output)
    {
    case OutputRange::RANGE_0_5V:
        ESP_LOGI("dac", "Set output range to 0-5V");
        data = 0x00;
        break;

    case OutputRange::RANGE_0_10V:
        ESP_LOGI("dac", "Set output range to 0-10V");
        data = 0x11;
        break;

    default:
        ESP_LOGE("dac", "Invalid DAC output range");
        return esphome::i2c::ERROR_UNKNOWN;
    }

    // Step 1: Send register address without STOP condition
    esphome::i2c::ErrorCode err = bus->write(address, &reg, 1, false);
    if (err != esphome::i2c::ERROR_OK)
    {
        ESP_LOGW("dac", "Failed to write DAC register address, I2C error %d", (int)err);
        return err;
    }

    // Step 2: Send data byte
    err = bus->write(address, &data, 1);
    if (err == esphome::i2c::ERROR_OK)
    {
        ESP_LOGI("dac", "DAC output range set successfully");
    }
    else
    {
        ESP_LOGW("dac", "Failed to set DAC output range, I2C error %d", (int)err);
    }

    return err;
}