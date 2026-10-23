# Tree Sensor Node

This directory contains the firmware and documentation for the ForestNetra tree-monitoring sensor node.

## Hardware

- ESP32
- TCS34725 RGB colour sensor

## Function

The node collects colour readings from the TCS34725 sensor and calculates the Green Leaf Index (GLI) as an optical indicator of vegetation greenness.

## Communication

The node sends tree sensor data to the ForestNetra transmitter over Bluetooth Classic.

## Data fields

- `C` — Raw clear-channel reading from the TCS34725
- `G` — Green Leaf Index (GLI)

The GLI is an optical greenness indicator, not a direct measurement of leaf chlorophyll content.

## Firmware

The verified working firmware, Bluetooth device name, MAC address and wiring details will be added after checking the actual implementation.
