"""Simulate the heater and PID loop used by the temperature controller."""
from __future__ import annotations

import argparse


def simulate(setpoint: float, kp: float, ki: float, seconds: float) -> float:
    temperature, integral = 20.0, 0.0
    for _ in range(int(seconds * 10)):
        error = setpoint - temperature
        integral = max(-40.0, min(40.0, integral + error * 0.1))
        output = max(0.0, min(1.0, kp * error + ki * integral))
        temperature += (output * 8.0 - (temperature - 20.0) * 0.18) * 0.1
    return temperature


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--setpoint", type=float, default=55.0)
    parser.add_argument("--kp", type=float, default=0.08)
    parser.add_argument("--ki", type=float, default=0.01)
    parser.add_argument("--seconds", type=float, default=120.0)
    args = parser.parse_args()
    final = simulate(args.setpoint, args.kp, args.ki, args.seconds)
    print(f"final_temperature_c={final:.2f} error_c={args.setpoint - final:.2f}")
