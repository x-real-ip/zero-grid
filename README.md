# zero-grid

Routes surplus solar power to a heating element using an ESP32. 
The goal: **keep grid power near 0 W** by dynamically adjusting heater load based on live grid readings.

This project is **fully integrated with Home Assistant**  
all PID parameters (**Kp**, **Ki**, **Kd**) can be **adjusted live** from within Home Assistant using sliders. This allows you to fine-tune the control behavior in real time while monitoring power flow and system response.


---

## ⚙️ Features

- 🧠 **PID-controlled power routing** - keeps grid export/import balanced around 0 W  
- ⚡ **DAC output (0–10 V)** - controls an external voltage regulator for heater power  
- 🔌 **Bypass relay (GPIO 33)** - enables full-power mode when solar output exceeds the heater capacity  
- ☀️ **Automatic operation** - reacts to grid power changes several times per second  
- 🧰 **Manual mode** - allows manual control for testing or calibration  
- 📡 **MQTT integration** - receives power and voltage data from a smart meter or Home Assistant  
- 🧾 **Debug logging** - shows PID loop behavior and DAC output in real time  

---

## 🪜 How it works

1. **Grid power** is measured via MQTT (delivered – returned).  
2. The **PID controller** adjusts the **target load power** so that grid power → 0 W.  
3. The **DAC output** generates a voltage (0–10 V) proportional to desired heater power.  
4. The external **voltage regulator** translates that DAC signal to an AC output for the heater.  
5. If surplus ≥ heater max power, the **bypass relay** closes, sending full power to the load.

---

## 🧮 PID Control

| Term | Function | Tuning tip |
|------|-----------|------------|
| **Kp** | Proportional gain | Increase to make response faster, but risk oscillation |
| **Ki** | Integral gain | Compensates long-term offset; too high → overshoot |
| **Kd** | Derivative gain | Dampens fast changes; helps stabilize |

**Target behavior:**  
> If grid power is **negative** (exporting), increase heater power.  
> If grid power is **positive** (importing), decrease heater power.  

---

## 🔌 Hardware

| Component | Description | Notes |
|------------|-------------|-------|
| **ESP32** | Main controller | Any ESP32 dev board |
| **DAC (I²C)** | 15-bit DAC, 0–10 V output | Controlling the voltage regulator |
| **Voltage regulator** | Accepts 0–10 V control | Drives the heater |
| **Heater load** | Resistive element | e.g. 2000 W @ 230 V |
| **Bypass relay** | GPIO 33 | Engages full power when surplus ≥ max power |

---

## 📡 MQTT Topics

| Topic example | Description | Value example |
|--------|--------------|----------|
| `dsmr/reading/electricity_currently_delivered` | Power drawn from grid (kW) | `0.300` |
| `dsmr/reading/electricity_currently_returned` | Power exported to grid (kW) | `0.600` |
| `dsmr/reading/phase_voltage_l1` | Grid voltage (V) | `230.0` |

---

## ⚙️ ESPHome Configuration

Main config (where the action take place):

```yaml
esphome:
  name: solar-router
  includes:
    - solar-router.h

interval:
  - interval: 200ms
    then:
      - lambda: |-
          id(pid_load_power) = pid_target_load_power(
              id(grid_power),
              id(pid_load_power),
              id(target_load_power_max),
              id(pid_Kp).state,
              id(pid_Ki).state,
              id(pid_Kd).state,
              id(pid_dt),
              id(pid_integral),
              id(pid_error_prev)
          );

          route_power_to_load(
            id(i2cbus),
            id(dac_addr_reg),
            id(pid_load_power),
            id(target_load_power_max),
            id(load_resistance),
            id(grid_voltage).state
          );
