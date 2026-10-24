#include <Arduino.h>
#include "BluetoothSerial.h"

/*
========================================================
        ESP32 SOIL SENSOR CONNECTIONS AUTO RS485
========================================================

TTL to RS485 Module to ESP32
--------------------------------
RS485 Module    to ESP32
--------------------------------
VCC             to 5V or 3.3V depending on module
GND             to GND
RO              to GPIO26 RXD2
DI              to GPIO27 TXD2

No RE DE pins required auto direction module

--------------------------------------------------------

RS485 Module to Soil Sensor JXBS 7 in 1
--------------------------------
RS485 Module    to Soil Sensor
--------------------------------
A D plus        to A plus
B D minus       to B minus
GND             to GND
VCC             to 12V sensor needs 9 to 24V

--------------------------------------------------------

Power Supply
--------------------------------
12V Battery or Solar to Soil Sensor direct
12V to Buck Converter to 5V or 3.3V to ESP32

--------------------------------------------------------

Important Notes
Common GND is required
If no data swap A and B wires
Baud rate 9600
Do not connect 12V to ESP32
Use buck converter

========================================================
*/


// ======================================================
// RS485 / SOIL SENSOR
// ======================================================

#define RXD2 26
#define TXD2 27

HardwareSerial mod(2);


// ======================================================
// BLUETOOTH
// ======================================================

BluetoothSerial SerialBT;

// Soil node Bluetooth name
#define BLUETOOTH_NAME "ForestNetra_Soil"


// ======================================================
// MODBUS
// ======================================================

byte request[8];
byte response[20];


uint16_t modbusCRC(byte *buf, int len)
{
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++)
  {
    crc ^= buf[pos];

    for (int i = 0; i < 8; i++)
    {
      if (crc & 1)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }

  return crc;
}


uint16_t readRegister(uint16_t reg)
{
  request[0] = 0x01;
  request[1] = 0x03;
  request[2] = reg >> 8;
  request[3] = reg & 0xFF;
  request[4] = 0x00;
  request[5] = 0x01;

  uint16_t crc = modbusCRC(request, 6);

  request[6] = crc & 0xFF;
  request[7] = crc >> 8;

  while (mod.available())
    mod.read();

  mod.write(request, 8);

  delay(300);

  if (mod.available() >= 7)
  {
    mod.readBytes(response, 7);

    return (response[3] << 8) | response[4];
  }

  return 0;
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  // Existing RS485 initialization
  mod.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Soil Sensor Ready");


  // ====================================================
  // BLUETOOTH SERVER INITIALIZATION
  // ====================================================

  if (!SerialBT.begin(BLUETOOTH_NAME))
  {
    Serial.println("Bluetooth Initialization Failed!");
  }
  else
  {
    Serial.println("Bluetooth Ready");

    Serial.print("Bluetooth Device Name: ");
    Serial.println(BLUETOOTH_NAME);

    Serial.print("Soil Node Bluetooth MAC: ");
    Serial.println(SerialBT.getBtAddressString());

    Serial.println("Waiting for ForestNetra_TX...");
  }
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
  // ====================================================
  // EXISTING SOIL SENSOR CODE
  // ====================================================

  float moisture = readRegister(0x0012) / 10.0;

  float temperature = readRegister(0x0013) / 10.0;

  int ec = readRegister(0x0015);

  float ph = readRegister(0x0006) / 100.0;

  int nitrogen = readRegister(0x001E);

  int phosphorus = readRegister(0x001F);

  int potassium = readRegister(0x0020);


  // ====================================================
  // EXISTING SERIAL MONITOR OUTPUT
  // ====================================================

  Serial.println("------ Soil Data ------");

  Serial.print("Moisture ");
  Serial.print(moisture);
  Serial.println(" percent");

  Serial.print("Temperature ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("EC ");
  Serial.print(ec);
  Serial.println(" us per cm");

  Serial.print("pH ");
  Serial.println(ph);

  Serial.print("Nitrogen ");
  Serial.print(nitrogen);
  Serial.println(" mg per kg");

  Serial.print("Phosphorus ");
  Serial.print(phosphorus);
  Serial.println(" mg per kg");

  Serial.print("Potassium ");
  Serial.print(potassium);
  Serial.println(" mg per kg");

  Serial.println("-----------------------");


  // ====================================================
  // BLUETOOTH DATA PACKET
  // ====================================================

  String soilDataPacket =
      "M=" + String(moisture, 2) +
      ",ST=" + String(temperature, 2) +
      ",E=" + String(ec) +
      ",PH=" + String(ph, 2) +
      ",N=" + String(nitrogen) +
      ",PP=" + String(phosphorus) +
      ",K=" + String(potassium);


  // ====================================================
  // BLUETOOTH TRANSMISSION
  // ====================================================

  if (SerialBT.hasClient())
  {
    SerialBT.println(soilDataPacket);

    Serial.println("Bluetooth Data Sent:");
    Serial.println(soilDataPacket);
    Serial.println("Destination: ForestNetra_TX");
  }
  else
  {
    Serial.println("Bluetooth: ForestNetra_TX not connected");
  }


  delay(3000);
}