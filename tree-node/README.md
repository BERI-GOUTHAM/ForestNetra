# Forest Netra — Tree Monitoring Node

## 1. Overview

The Forest Netra Tree Node uses an ESP32 and a TCS34725 RGB/clear optical colour sensor to collect colour-related measurements from a tree leaf or the selected plant surface.

The ESP32 reads the optical sensor over I²C, calculates a Green Leaf Index (GLI) using the red, green, and blue channels, and sends a compact packet to the central Forest Netra TX node through Bluetooth Classic SPP.

The GLI is used as an optical greenness indicator. It is not a direct chlorophyll measurement.

## 2. Hardware Used

| Component | Purpose |
|---|---|
| ESP32 development board | Sensor controller and Bluetooth communication |
| TCS34725 RGB/clear colour sensor | Collects red, green, blue, and clear-channel measurements |
| 12 V battery | Source for the DC-DC buck converter |
| DC-DC buck converter | Provides approximately 5 V for the ESP32 input, as documented |

## 3. Parameters Collected

| Field | Description |
|---|---|
| `R` | Raw red-channel measurement used in the calculation |
| `G` | Raw green-channel measurement used in the calculation |
| `B` | Raw blue-channel measurement used in the calculation |
| `C` | Raw clear-channel intensity |
| `GLI` | Calculated Green Leaf Index |

The compact Bluetooth packet currently includes `C` and the calculated greenness value `G`.

### GLI Calculation

The project uses:

```text
GLI = (2G - R - B) / (2G + R + B)
```

Here, `R`, `G`, and `B` are the optical sensor's red, green, and blue readings.

The current project interpretation is:

| GLI Range | Project Interpretation |
|---|---|
| Greater than 0.15 | Higher greenness |
| 0.08–0.15 | Moderate greenness |
| Less than 0.08 | Lower greenness |

These are project-specific thresholds. They should be validated experimentally before being treated as universal plant-health criteria.

## 4. Hardware Connections

### TCS34725 to ESP32

| TCS34725 Pin | Documented ESP32 Connection |
|---|---|
| SDA | D1 |
| SCL | D2 |
| VCC | 3V3 |
| GND | GND |
| INT | Not connected |

The sensor communicates with the ESP32 through the I²C interface.

**Board mapping note:** The hardware reference records the pin labels D1 and D2. Confirm that these labels correspond to the GPIO assignments in the specific ESP32 board and firmware being used before reconnecting the hardware.

### Sensor Configuration

| Setting | Value |
|---|---|
| Integration time | 50 ms |
| Gain | 4× |
| Communication interface | I²C |

## 5. Bluetooth Communication

The ESP32 operates as a Bluetooth Classic SPP server.

| Setting | Value |
|---|---|
| Bluetooth device name | `ForestNetra_Tree` |
| Bluetooth MAC address | `A0:A3:B3:29:C3:BE` |
| Bluetooth mode | Classic Bluetooth SPP server |
| Receiving device | Forest Netra ESP32 + SX1262 TX node |

### Packet Format

```text
TREE|C=1306,G=0.1021
```

This is an example packet. The clear-channel value and GLI will vary with the sensor readings and measurement conditions.

## 6. Data Flow

```text
Tree Leaf / Plant Surface
          |
          v
      TCS34725
          |
          | I²C
          v
       ESP32 Tree Node
          |
          | Calculate GLI
          | Format compact packet
          v
   Bluetooth Classic SPP
          |
          v
   Forest Netra LoRa TX Node
```

The tree node sends its compact packet to the TX node over Bluetooth. The TX node collects the available tree, soil, and environmental packets and combines their values into one LoRa payload.

## 7. Important Notes

- Keep the TCS34725 close to the target surface and maintain consistent measurement conditions where possible.
- Ambient illumination, distance, sensor orientation, and surface characteristics can influence optical readings.
- GLI is a greenness proxy, not a direct chlorophyll concentration or definitive diagnosis of tree health.
- The current thresholds are project-specific and require validation.
- Keep the Bluetooth name and MAC address consistent with the TX firmware.
- Do not commit credentials or private access keys.

## 8. Project Role

The Tree Node provides optical data related to leaf colour and greenness. Its readings complement the soil and environmental measurements collected by the other Forest Netra nodes.
