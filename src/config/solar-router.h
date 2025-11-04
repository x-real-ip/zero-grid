#pragma once

#include "esphome.h"

/**
 * Set DAC output range to 0-10V
 * Uses fixed register 0x01 for output range
 */
inline esphome::i2c::ErrorCode dac_set_output_range(
    esphome::i2c::I2CBus *bus,
    uint8_t address)
{
    uint8_t reg = 0x01;  // fixed register for output range
    uint8_t data = 0x11; // 0-10V setting

    ESP_LOGI("dac", "Set output range to 0-10V");

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

/**
 * Set DAC output voltage (0–10V) for 15-bit DAC
 * Pointer to the I2C bus
 * DAC I2C address
 * Desired output voltage (0.0–10.0V)
 * esphome::i2c::ErrorCode
 */
inline esphome::i2c::ErrorCode dac_set_voltage(
    sphome::i2c::I2CBus *bus,
    uint8_t address,
    float volts)
{
    // Clamp volts to 0–10V
    if (volts < 0.0f)
        volts = 0.0f;
    if (volts > 10.0f)
        volts = 10.0f;

    // Convert volts to 15-bit DAC value
    uint16_t data = static_cast<uint16_t>((volts / 10.0f) * 32767.0f);
    data <<= 1; // shift for 15-bit DAC

    // Prepare I2C buffer: [register, low byte, high byte]
    uint8_t buf[3] = {0x02, static_cast<uint8_t>(data & 0xFF), static_cast<uint8_t>(data >> 8)};

    // Write buffer to DAC
    esphome::i2c::ErrorCode err = bus->write(address, buf, 3);
    if (err == esphome::i2c::ERROR_OK)
    {
        ESP_LOGI("dac", "DAC set to %.2f V (data=%d)", volts, data);
    }
    else
    {
        ESP_LOGW("dac", "DAC I2C write error %d", static_cast<int>(err));
    }

    return err;
}