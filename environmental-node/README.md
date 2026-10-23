# Environmental Sensor Node

This directory contains the firmware and documentation for the ForestNetra environmental monitoring node.

## Hardware

- ESP32
- BMP390 pressure and temperature sensor
- DHT11 temperature and humidity sensor
- BH1750 light intensity sensor

## Measurements

- `T` — Temperature from BMP390 (°C)
- `P` — Atmospheric pressure (hPa)
- `A` — Estimated altitude derived from pressure (m)
- `H` — Relative humidity from DHT11 (%)
- `L` — Light intensity from BH1750 (lux)

## Communication

The node sends environmental sensor data to the ForestNetra transmitter over Bluetooth Classic.

## Firmware and configuration

The verified working firmware, Bluetooth device name, MAC address, wiring details and sensor configuration will be documented after checking the actual implementation.
