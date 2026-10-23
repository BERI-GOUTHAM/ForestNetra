# Soil Sensor Node

This directory contains the firmware and documentation for the ForestNetra soil sensor node.

## Hardware

- ESP32
- JXBS 7-in-1 soil sensor
- RS485 interface with automatic direction control

## Communication

The node reads soil measurements and sends them to the ForestNetra transmitter over Bluetooth Classic.

## Data fields

- `M` — Soil moisture (%)
- `ST` — Soil temperature (°C)
- `E` — Electrical conductivity (µS/cm)
- `PH` — Soil pH
- `N` — Nitrogen (mg/kg)
- `PP` — Phosphorus (mg/kg)
- `K` — Potassium (mg/kg)

Firmware will be added after the current working version is reviewed.
