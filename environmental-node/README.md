# Forest Netra — Environmental Monitoring Node

## 1. Overview

The Forest Netra Environmental Node uses an ESP32 with three sensors: BMP390, DHT11, and BH1750.

The node collects atmospheric temperature and pressure, calculates altitude from pressure, obtains relative humidity, and measures ambient light intensity. It sends a compact environmental packet to the central Forest Netra TX node through Bluetooth Classic SPP.

## 2. Hardware Used

| Component | Purpose |
|---|---|
| ESP32 development board | Sensor controller and Bluetooth communication |
| BMP390 | Atmospheric temperature and pressure |
| DHT11 | Relative humidity and temperature |
| BH1750 | Ambient light intensity |
| 12 V battery | Source for the DC-DC buck converter |
| DC-DC buck converter | Provides approximately 5 V for the ESP32 input, as documented |

## 3. Parameters Measured

| Packet Field | Parameter | Unit | Source |
|---|---|---|---|
| `T` | Environmental temperature | °C | BMP390 |
| `P` | Atmospheric pressure | hPa | BMP390 |
| `A` | Calculated altitude | m | Calculated from BMP390 pressure |
| `H` | Relative humidity | %RH | DHT11 |
| `L` | Ambient light intensity | lux | BH1750 |

The current telemetry uses the BMP390 temperature and pressure, altitude calculated from pressure, DHT11 humidity, and BH1750 light intensity.

## 4. Hardware Connections

### 4.1 BMP390 to ESP32

| BMP390 Pin | Documented Connection |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | D1 / GPIO5 |
| SCL | D2 / GPIO4 |
| INT | Not connected |

### 4.2 BH1750 to ESP32

| BH1750 Pin | Documented Connection |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | D1 / GPIO5 |
| SCL | D2 / GPIO4 |
| ADDR | GND |

The BMP390 and BH1750 share the same I²C bus.

### 4.3 DHT11 to ESP32

| DHT11 Pin | Documented Connection |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| DATA | D5 / GPIO14 |

### Board Mapping Note

The hardware reference records the labels D1, D2, and D5 alongside the GPIO numbers. Confirm these mappings against the specific ESP32 board and firmware before reconnecting the sensors.

## 5. Sensor Configuration

### BMP390

- I²C communication.
- Temperature oversampling: 8×.
- Pressure oversampling: 4×.
- IIR filter coefficient: 3.
- Output data rate setting: 50 Hz.

### BH1750

- I²C communication.
- Continuous high-resolution measurement mode.

### DHT11

- Digital data connection on GPIO14, as recorded in the hardware reference.
- Humidity is included in the current compact telemetry packet.

## 6. Bluetooth Communication

The ESP32 operates as a Bluetooth Classic SPP server.

| Setting | Value |
|---|---|
| Bluetooth device name | `ForestNetra_Environmental` |
| Bluetooth MAC address | `A4:CF:12:9E:45:0A` |
| Bluetooth mode | Classic Bluetooth SPP server |
| Receiving device | Forest Netra ESP32 + SX1262 TX node |

### Packet Format

```text
ENV|T=25.03,P=1003.25,A=83.58,H=77.00,L=467.50
```

This is an example packet. Actual values depend on the environment and sensor readings.

## 7. Data Flow

```text
BMP390 ──┐
         ├── I²C / Digital ──> ESP32 Environmental Node
BH1750 ──┘                           |
                                     |
DHT11 ─────── Digital ───────────────┘
                                     |
                                     | Format ENV packet
                                     v
                            Bluetooth Classic SPP
                                     |
                                     v
                            Forest Netra LoRa TX Node
```

The environmental node sends its packet to the TX node. The TX node combines the available environmental values with the soil and tree data before transmitting a complete message to the RX gateway over LoRa.

## 8. Important Notes

- BMP390 and BH1750 share the documented I²C lines.
- Use the correct power supply for each sensor module.
- Verify the board-specific pin mapping before changing physical connections.
- Altitude is calculated from pressure and should be interpreted as an estimate.
- Do not publish Wi-Fi passwords or cloud credentials in this repository.

## 9. Project Role

The Environmental Node provides atmospheric and illumination context for interpreting soil and tree measurements in Forest Netra.
