#pragma once

#include "esphome.h"

/**
 * @brief Sets the voltage mode (range) of the DAC.
 * 
 * This function configures the DAC to operate in 0–10 V output mode.
 * It writes to the fixed DAC register 0x01 using the ESPHome I2C bus.
 * 
 * @param bus     Pointer to the ESPHome I2C bus
 * @param address I2C address of the DAC
 * @return esphome::i2c::ErrorCode Returns ERROR_OK if successful, otherwise the I2C error code
 * 
 * @note The DAC register for output range is fixed at 0x01. The data value 0x11
 *       corresponds to 0–10 V mode. Errors are logged using ESP_LOGW.
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
 * @brief Sets the output voltage of a 15-bit DAC over I2C.
 * 
 * This function converts a desired voltage (0–10 V) into a 15-bit DAC value
 * and writes it to the DAC using the ESPHome I2C bus. Values are clamped
 * to the DAC range to avoid out-of-bounds output.
 * 
 * @param bus     Pointer to the ESPHome I2C bus
 * @param address I2C address of the DAC
 * @param volts   Desired output voltage in volts (0.0–10.0 V)
 * @return esphome::i2c::ErrorCode Returns ERROR_OK if successful, otherwise the I2C error code
 * 
 * @note The function assumes the DAC register for setting voltage is fixed at 0x02
 *       and that the DAC resolution is 15 bits (0–32767). Values outside 0–10 V
 *       are automatically clamped.
 */
inline esphome::i2c::ErrorCode dac_set_voltage(
    esphome::i2c::I2CBus *bus,
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

/**
 * @brief Routes returned grid power to a resistive heater via DAC.
 * 
 * This function calculates the voltage required across the heater to consume
 * the same power as is being returned to the grid. It then maps this voltage
 * to a 0–10V DAC output and sends it to the DAC over I2C.
 * 
 * @param bus                 Pointer to the ESPHome I2C bus
 * @param address             DAC I2C address
 * @param grid_power_returned Power currently being returned to the grid (W)
 * @param load_max_power      Maximum power allowed for the heater (W)
 * @param load_resistance     Resistance of the heater (Ohm)
 * @param grid_voltage        Current measured grid voltage (V)
 */
inline void route_power_to_load(
    esphome::i2c::I2CBus *bus,
    uint8_t address,
    float grid_power_returned,
    float load_max_power,
    float load_resistance,
    float grid_voltage)
{
    // Clamp returned watts
    if (grid_power_returned < 0)
        grid_power_returned = 0;
    if (grid_power_returned > load_max_power)
        grid_power_returned = load_max_power;

    // Target heater power = returned power
    float load_target_power = grid_power_returned;

    // Calculate voltage needed across heater to achieve target power
    float load_voltage = sqrt(load_target_power * load_resistance);

    // Map heater voltage to DAC 0–10V range
    float dac_volts = load_voltage * 10.0f / grid_voltage;

    // Send voltage to DAC
    dac_set_voltage(bus, address, dac_volts);

    ESP_LOGI("heater", "Returned %.1f W → heater %.1f W, DAC %.2f V", grid_power_returned, load_target_power, dac_volts);
}