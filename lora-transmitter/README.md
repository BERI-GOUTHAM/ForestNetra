# ForestNetra LoRa Transmitter

This directory contains the firmware and documentation for the ForestNetra field-side transmitter.

## Main functions

- Connect to the soil, tree and environmental sensor nodes using Bluetooth Classic.
- Receive sensor readings from each node.
- Combine the received readings into a compact data payload.
- Transmit the aggregated payload to the ForestNetra LoRa receiver.

## Communication

- Sensor nodes to transmitter: Bluetooth Classic
- Transmitter to gateway: LoRa using an SX1262 radio module

## Configuration

The verified firmware will document the Bluetooth device names and MAC addresses, LoRa frequency and radio settings, GPIO pin assignments, payload format and error-handling behaviour.

Actual settings will be confirmed against the working source code before being finalized.
