# Forest Netra — Soil Monitoring Node

## 1. Overview

The Forest Netra Soil Node collects soil measurements using a JXBS 7-in-1 soil sensor connected to an ESP32 through an RS485-to-TTL interface.

The ESP32 reads the sensor data, formats the measurements into a compact text packet, and makes the packet available to the central Forest Netra TX node through Bluetooth Classic Serial Port Profile (SPP).

This node performs soil-data acquisition and transmission. It does not send data directly to the LoRa gateway or cloud.

## 2. Hardware Used

| Component | Purpose |
|---|---|
| ESP32 development board | Sensor controller and Bluetooth communication |
| JXBS 7-in-1 soil sensor | Measures soil parameters |
| RS485-to-TTL interface | Converts RS485 communication to UART signals |
| 12 V battery | Powers the soil sensor |
| DC-DC buck converter | Supplies approximately 4.83 V to the RS485-to-TTL interface |
| 1469 Ω resistor | Upper resistor in the documented voltage divider |
| 3210 Ω resistor | Lower resistor in the documented voltage divider |

## 3. Parameters Measured

| Packet Field | Parameter | Unit |
|---|---|---|
| `M` | Soil moisture | % |
| `ST` | Soil temperature | °C |
| `E` | Electrical conductivity (EC) | µS/cm |
| `PH` | Soil pH | pH |
| `N` | Nitrogen | mg/kg |
| `PP` | Phosphorus | mg/kg |
| `K` | Potassium | mg/kg |

The reliability and physical interpretation of the readings depend on the sensor model, calibration, installation, and measurement conditions.

## 4. Hardware Connections

### 4.1 JXBS Soil Sensor to Power and RS485

| Sensor Wire | Connection |
|---|---|
| Brown | +12 V supply |
| Black | GND |
| Yellow | RS485 A+ |
| Blue | RS485 B− |

The JXBS sensor receives power from the 12 V source.

### 4.2 Power Supply for the RS485-to-TTL Interface

The documented power arrangement is:

1. Connect the 12 V source to the DC-DC buck converter input.
2. Adjust the buck converter output to approximately 4.83 V.
3. Connect buck output positive to the RS485-to-TTL interface VCC.
4. Connect buck output negative to the RS485-to-TTL interface GND.

The exact ESP32 power connection is not specified in the current hardware reference. Do not infer it from this wiring description.

### 4.3 RS485-to-TTL Interface to ESP32

| RS485-to-TTL Signal | ESP32 Connection |
|---|---|
| TXD | GPIO27 |
| RXD | Voltage divider output → GPIO26 |
| GND | Common ground |

UART configuration: **9600 baud**.

The documented voltage divider uses a 1469 Ω resistor between the RS485-to-TTL RXD signal and GPIO26, and a 3210 Ω resistor between GPIO26 and GND.

At an input of approximately 4.83 V, the divider output is approximately 3.31 V:

`Vout = 4.83 × 3210 / (1469 + 3210) ≈ 3.31 V`

This arrangement reduces the signal voltage before it reaches the ESP32 input.

## 5. Bluetooth Communication

The ESP32 operates as a Bluetooth Classic SPP server.

| Setting | Value |
|---|---|
| Bluetooth device name | `ForestNetra_Soil` |
| Bluetooth MAC address | `84:0D:8E:0F:4E:2A` |
| Bluetooth mode | Classic Bluetooth SPP server |
| Receiving device | Forest Netra ESP32 + SX1262 TX node |

The TX node connects to this soil node, receives its compact packet, and disconnects before collecting data from the next node.

### Packet Format

```text
M=0.30,ST=27.50,E=0,PH=7.00,N=0,PP=0,K=0
```

The values above are an example packet, not guaranteed sensor readings.

## 6. Data Flow

```text
JXBS 7-in-1 Soil Sensor
          |
          | RS485
          v
   RS485-to-TTL Interface
          |
          | UART, 9600 baud
          v
       ESP32 Soil Node
          |
          | Bluetooth Classic SPP
          v
   Forest Netra LoRa TX Node
```

The soil node sends its measurements only to the central TX node. The TX node combines soil data with available tree and environmental data, then transmits the combined payload to the RX gateway over LoRa.

## 7. Important Notes

- The sensor requires its documented 12 V supply.
- Use a common ground for the relevant power and signal interfaces.
- The RS485-to-TTL RXD signal must be at a voltage suitable for the ESP32 input.
- A zero value should not automatically be interpreted as a valid physical measurement. It may indicate unavailable or invalid data.
- Keep Bluetooth names, MAC addresses, UART settings, and GPIO assignments consistent with the firmware.
- Do not publish Wi-Fi passwords, cloud API keys, or other credentials in this repository.

## 8. Project Role

This node is the soil-data acquisition endpoint of Forest Netra. It converts field measurements into a compact Bluetooth packet for collection by the central transmitter.
