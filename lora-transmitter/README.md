# Forest Netra — LoRa RX Gateway

## 1. Overview

The Forest Netra RX gateway uses an ESP32 combined with an SX1262 LoRa transceiver.

It continuously listens for LoRa packets transmitted by the central Forest Netra TX node. When a packet is received, the gateway extracts the complete compact telemetry message and reports reception information such as message length, RSSI, and SNR.

The gateway then provides the Wi-Fi/internet connection used to forward the received telemetry toward the cloud. The current hardware reference identifies Adafruit IO as the temporary cloud telemetry destination.

## 2. Hardware Used

| Component | Purpose |
|---|---|
| ESP32 development board | Gateway control and internet connectivity |
| SX1262 LoRa transceiver | Receives packets from the TX node |
| LoRa antenna | RF reception, suitable for the module and operating frequency |
| Wi-Fi connection | Provides internet access for cloud forwarding |

The exact RX GPIO wiring is not specified in the current hardware reference. Do not copy the TX pin assignments into the RX repository unless the actual RX board and firmware confirm that they are the same.

## 3. LoRa Communication Settings

The TX and RX must use compatible radio settings.

| Parameter | Documented Setting |
|---|---|
| Frequency | 865.0 MHz |
| Bandwidth | 125.0 kHz |
| Spreading factor | 7 |
| Coding rate | 4/5 |
| Sync word | `0x34` |
| TX power | 14 dBm, configured on TX |
| Preamble length | 8 |

TX power is a transmitter setting; it is not an RX power setting.

## 4. Data Received

The RX gateway receives the complete combined telemetry payload from the TX node.

Example:

```text
T=25.03,P=1003.25,A=83.58,H=77.00,L=467.50,C=1306,G=0.1021,M=0.30,ST=27.50,E=0,PH=7.00,N=0,PP=0,K=0
```

The message contains fields from the environmental, tree, and soil nodes:

- Environmental: `T, P, A, H, L`
- Tree: `C, G`
- Soil: `M, ST, E, PH, N, PP, K`

The gateway should preserve the original payload when forwarding or storing the received data.

## 5. Reception Diagnostics

The documented receiver can report:

| Diagnostic | Meaning |
|---|---|
| Received data | The complete payload decoded from the LoRa packet |
| Message length | Length of the received payload |
| RSSI | Received Signal Strength Indicator, in dBm |
| SNR | Signal-to-noise ratio, in dB |

Example reception output:

```text
Received Data:
T=25.03,P=1003.25,A=83.58,H=77.00,L=467.50,C=1306,G=0.1021,M=0.30,ST=27.50,E=0,PH=7.00,N=0,PP=0,K=0

Message Length: 99
RSSI: -47.00 dBm
SNR: 12.50 dB
```

These values are examples from the hardware reference, not guaranteed reception results.

## 6. End-to-End Data Flow

```text
Soil / Tree / Environmental Nodes
                |
                | Bluetooth Classic SPP
                v
       Forest Netra ESP32 TX
                |
                | LoRa, 865 MHz
                v
       ESP32 + SX1262 RX Gateway
                |
                | Wi-Fi / Internet
                v
             Adafruit IO
```

The RX gateway is the bridge between the field LoRa network and the internet/cloud side.

The sensor nodes do not communicate directly with the RX gateway. Their data first reaches the TX node through Bluetooth, is combined into one payload, and then travels to the RX gateway over LoRa.

## 7. Cloud Telemetry

The current hardware reference identifies Adafruit IO as the temporary telemetry destination.

The gateway's cloud-forwarding workflow is:

1. Listen for LoRa packets.
2. Receive the complete telemetry payload.
3. Extract or retain the received data.
4. Connect to the configured Wi-Fi network.
5. Forward the telemetry to Adafruit IO using the implemented cloud communication method.
6. Report or handle communication errors as supported by the firmware.

**Implementation note:** The precise RX GPIO assignments and the exact cloud-upload code path must be confirmed from the current firmware. Do not place Wi-Fi passwords, Adafruit IO keys, or other credentials directly in a public README or source file.

## 8. Data Quality Considerations

- A received zero does not always mean the corresponding physical measurement is zero. The TX node may use zero for a sensor value that was unavailable during that collection cycle.
- Retain the original received payload for debugging and future parsing.
- Monitor RSSI and SNR to help diagnose link-quality problems.
- Ensure the TX and RX use compatible radio settings.
- A received packet does not by itself guarantee that cloud forwarding succeeded; check the relevant upload status separately.

## 9. Project Role

The RX gateway receives the aggregated field telemetry over LoRa and provides the internet connection for forwarding the data to the cloud. It completes the wireless communication path between the distributed sensing nodes and the cloud telemetry platform.
