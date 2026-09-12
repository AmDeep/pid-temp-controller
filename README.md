# Simple PID Temperature Controller

## Objective

Implement a classic PID loop on an Arduino that regulates temperature with a heater element and a DS18B20 sensor. The firmware includes anti-windup, output limiting, and live serial monitoring so the loop can be tuned on real hardware.

## Strategy

- Sample the temperature at a fixed 1-second interval so the discrete-time PID coefficients stay consistent.
- Use proportional, integral and derivative terms with simple rectangular integration and backward difference for the derivative.
- Clamp both the integral term and the final output to prevent windup and actuator saturation.
- Allow the setpoint to be changed at runtime over serial without recompiling.

## What worked

- A 1-second sample time matched the thermal time constant of a small cartridge heater and a cup of water.
- Clamping the integral to a modest range stopped the output from staying saturated after a large setpoint step.
- Printing P, I and D contributions on every sample made it obvious which term was dominating.
- The OneWire / DallasTemperature libraries gave reliable CRC-checked readings with minimal code.

## What failed and how it was resolved

- Early versions with a large Ki produced integral windup that kept the heater on long after the temperature overshot. Resolution: clamp the integral term and also limit the final output to 0-255.
- Derivative kick on setpoint changes caused a large spike in the output. Resolution: compute the derivative on the process variable error only after the new setpoint has been accepted, and keep Kd modest.
- Reading the sensor every loop without waiting for the conversion produced the previous value or an error code. Resolution: call requestTemperatures and then getTempCByIndex only after the conversion window (implicitly satisfied by the 1 s sample period).

## Engineering principles and frameworks used

- Classical discrete-time PID with anti-windup.
- Fixed sample period for predictable controller dynamics.
- Actuator saturation handling.
- Observability of each PID term for tuning.
- Separation of sensing, control law, and actuation.

## Hardware

- Arduino Uno or compatible
- DS18B20 temperature sensor
- 4.7 k pull-up on the data line
- MOSFET or relay module to switch a resistive heater
- Suitable power supply for the heater (never power the heater from the Arduino)

## Wiring

| DS18B20 | Arduino |
|---------|---------|
| VDD     | 5 V     |
| GND     | GND     |
| DQ      | D2      |

| MOSFET / Relay | Arduino |
|----------------|---------|
| Gate / Signal  | D9 (PWM)|
| Source / GND   | GND     |

## Software

Requires the OneWire and DallasTemperature libraries.

Upload pid_temp.ino. Open Serial Monitor at 9600 baud. Type a new setpoint (degrees C) and press enter to change it at runtime.

## Possible extensions

- Add ambient temperature feed-forward.
- Store gains and setpoint in EEPROM.
- Drive a solid-state relay with a slower time-proportioned cycle for larger heaters.
