# ForestNetra LoRa Gateway

This directory contains the firmware and documentation for the ForestNetra receiving gateway.

## Main functions

- Receive sensor payloads from the field-side LoRa transmitter.
- Display received packet information, including packet length, RSSI and SNR, where available.
- Forward the received sensor data to internet-connected services.
- Support data logging on the ForestNetra Linux server.

## Communication

- Transmitter to gateway: LoRa using an SX1262 radio module.
- Gateway to server and cloud: Wi-Fi and internet connectivity.

## Configuration

The verified firmware will document the LoRa radio settings, GPIO pin assignments, Wi-Fi configuration method, server endpoint, cloud integration and timestamp handling.

Passwords, API keys and other private credentials must not be committed to this repository.
