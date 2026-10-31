# Forest Netra — LoRa TX and Aggregator Node

## 1. Overview

The Forest Netra TX node uses an ESP32 combined with an SX1262 LoRa transceiver.

It acts as the Bluetooth Classic client/master for the three sensing nodes. It connects to each node sequentially, receives its compact packet, disconnects, and then moves to the next node.

After collecting the available data, the TX node combines the environmental, tree, and soil measurements into one compact telemetry message and transmits that message as a single LoRa packet to the Forest Netra RX gateway.

## 2. Hardware Used

| Component | Purpose |
|---|---|
| ESP32 development board | Bluetooth aggregation and control |
| SX1262 LoRa transceiver | Long-range wireless transmission |
| Bluetooth Classic interface | Receives packets from sensor nodes |
| LoRa antenna | RF transmission, using an antenna suitable for the module and operating frequency |

The exact power wiring and antenna connection depend on the specific combined ESP32+SX1262 board.

## 3. Sensor Nodes Connected

The TX node collects packets from these Bluetooth Classic SPP servers:

| Sensor Node | Bluetooth Name | MAC Address | Packet Fields |
|---|---|---|---|
| Soil | `ForestNetra_Soil` | `84:0D:8E:0F:4E:2A` | `M, ST, E, PH, N, PP, K` |
| Tree | `ForestNetra_Tree` | `A0:A3:B3:29:C3:BE` | `C, G` |
| Environmental | `ForestNetra_Environmental` | `A4:CF:12:9E:45:0A` | `T, P, A, H, L` |

The node connects to the sensor nodes one at a time rather than requiring simultaneous Bluetooth connections.

## 4. SX1262 to ESP32 Connections

The documented SPI and control-pin mapping is:

| SX1262 Signal | ESP32 GPIO |
|---|---:|
| NSS / CS | GPIO5 |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| DIO1 | GPIO26 |
| RESET | GPIO14 |
| BUSY | GPIO27 |

These assignments are based on the system hardware reference. Verify them against the exact combined board and firmware before rewiring.

## 5. Bluetooth Collection Sequence

The TX node follows this sequence:

1. Connect to the Soil Node using its Bluetooth MAC address.
2. Receive the soil packet.
3. Disconnect from the Soil Node.
4. Connect to the Tree Node.
5. Receive the tree packet.
6. Disconnect from the Tree Node.
7. Connect to the Environmental Node.
8. Receive the environmental packet.
9. Disconnect from the Environmental Node.
10. Combine the available measurements into a complete telemetry payload.
11. Transmit one LoRa packet to the RX gateway.

## 6. Combined Telemetry Format

The TX node combines these fields:

| Field | Source |
|---|---|
| `T` | BMP390 |
| `P` | BMP390 |
| `A` | Calculated from BMP390 pressure |
| `H` | DHT11 |
| `L` | BH1750 |
| `C` | TCS34725 clear channel |
| `G` | TCS34725-derived GLI |
| `M` | JXBS 7-in-1 |
| `ST` | JXBS 7-in-1 |
| `E` | JXBS 7-in-1 |
| `PH` | JXBS 7-in-1 |
| `N` | JXBS 7-in-1 |
| `PP` | JXBS 7-in-1 |
| `K` | JXBS 7-in-1 |

Example combined packet:

```text
T=25.03,P=1003.25,A=83.58,H=77.00,L=467.50,C=1306,G=0.1021,M=0.30,ST=27.50,E=0,PH=7.00,N=0,PP=0,K=0
```

The packet contains the available readings from all three sensor nodes. The values shown are illustrative examples from the system reference.

## 7. LoRa Configuration

| Parameter | Value |
|---|---|
| Frequency | 865.0 MHz |
| Bandwidth | 125.0 kHz |
| Spreading factor | 7 |
| Coding rate | 4/5 |
| Sync word | `0x34` |
| TX power | 14 dBm |
| Preamble length | 8 |

The RX gateway must use compatible LoRa settings to receive the packet.

## 8. Handling Missing Sensor Data

If a sensor node fails to connect or provide valid data during a collection cycle, the TX node can still build and transmit the complete payload.

The documented logic resets the sensor values at the beginning of a cycle. Values that are not successfully obtained remain zero for that cycle.

Therefore, a zero-filled field may mean data was unavailable, rather than that the physical measurement was truly zero. The receiver or backend should preserve this distinction where possible.

## 9. End-to-End Communication

```text
Soil Node ───────────┐
Tree Node ───────────┼── Bluetooth Classic SPP
Environmental Node ──┘
                           |
                           v
                    ESP32 TX Aggregator
                           |
                           | Combine available fields
                           v
                   One compact telemetry packet
                           |
                           | LoRa, 865 MHz
                           v
                    ESP32 SX1262 RX Gateway
```

The TX node does not connect directly to Adafruit IO. Its primary role is Bluetooth collection, packet aggregation, and LoRa transmission.

## 10. Project Role

The TX node is the central field aggregator. It bridges short-range Bluetooth communication from the sensor nodes to the long-range LoRa link toward the RX gateway.
