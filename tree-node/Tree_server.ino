#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "BluetoothSerial.h"


// ============================================================
// BLUETOOTH
// ============================================================

BluetoothSerial SerialBT;

#define BLUETOOTH_NAME "ForestNetra_Tree"


// ============================================================
// TCS34725 SENSOR
// ============================================================

// Create sensor object
// Integration time + gain

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("========================================");
  Serial.println("FOREST NETRA TREE NODE");
  Serial.println("ESP32 + TCS34725 + Bluetooth");
  Serial.println("========================================");


  // ----------------------------------------------------------
  // I2C
  // ESP32:
  // SDA = GPIO 21
  // SCL = GPIO 22
  // ----------------------------------------------------------

  Wire.begin(21, 22);


  // ----------------------------------------------------------
  // TCS34725 INITIALIZATION
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("--- SMART PLANT MONITORING SYSTEM ---");
  Serial.println("Initializing sensor...\n");


  if (!tcs.begin())
  {
    Serial.println("❌ TCS34725 NOT detected! Check your wiring.");

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println("✅ TCS34725 Sensor Ready\n");


  // ----------------------------------------------------------
  // BLUETOOTH SERVER INITIALIZATION
  // ----------------------------------------------------------

  if (!SerialBT.begin(BLUETOOTH_NAME))
  {
    Serial.println("Bluetooth initialization FAILED!");

    while (1)
    {
      delay(1000);
    }
  }


  Serial.println("Bluetooth Ready");

  Serial.print("Bluetooth Device Name: ");
  Serial.println(BLUETOOTH_NAME);

  Serial.print("Tree Node Bluetooth MAC: ");
  Serial.println(SerialBT.getBtAddressString());

  Serial.println("Waiting for ForestNetra_TX...");


  // ----------------------------------------------------------

  Serial.println();
  Serial.println("TREE NODE READY");
  Serial.println("========================================");
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ----------------------------------------------------------
  // READ TCS34725
  // ----------------------------------------------------------

  uint16_t raw_r, raw_g, raw_b, raw_c;


  tcs.getRawData(
    &raw_r,
    &raw_g,
    &raw_b,
    &raw_c
  );


  // ----------------------------------------------------------
  // CHECK LIGHT LEVEL
  // ----------------------------------------------------------

  if (raw_c == 0)
  {
    Serial.println("⚠️ No light detected / Sensor covered.");

    delay(2000);
    return;
  }


  // ----------------------------------------------------------
  // STEP 1: NORMALIZE RAW DATA
  // ----------------------------------------------------------

  float r = (float)raw_r / raw_c;

  float g = (float)raw_g / raw_c;

  float b = (float)raw_b / raw_c;


  // ----------------------------------------------------------
  // STEP 2: CALCULATE GREEN LEAF INDEX
  // ----------------------------------------------------------

  float numerator =
      (2.0 * g) - r - b;

  float denominator =
      (2.0 * g) + r + b;


  float gli = 0.0;


  if (denominator != 0)
  {
    gli = numerator / denominator;
  }


  // ----------------------------------------------------------
  // STEP 3: PLANT HEALTH STATUS
  // ----------------------------------------------------------

  String plantStatus;


  if (gli >= 0.15)
  {
    plantStatus = "Healthy";
  }
  else if (gli >= 0.08)
  {
    plantStatus = "Moderate (Stressed/Deficient)";
  }
  else
  {
    plantStatus = "Diseased/Chlorotic";
  }


  // ----------------------------------------------------------
  // ENVIRONMENT LIGHT STATUS
  // ----------------------------------------------------------

  String env;


  if (raw_c < 50 || raw_c > 10000)
  {
    env = "Unstable Light";
  }
  else
  {
    env = "Stable Light";
  }


  // ----------------------------------------------------------
  // REFLECTANCE INTENSITY
  // ----------------------------------------------------------

  String reflect;


  if (raw_c > 300)
  {
    reflect = "High";
  }
  else
  {
    reflect = "Low";
  }


  // ----------------------------------------------------------
  // NUTRIENT STATUS
  // ----------------------------------------------------------

  String nutrient;


  if (gli >= 0.15 && g > r && g > b)
  {
    nutrient = "High (Nitrogen Rich)";
  }
  else if (gli >= 0.00)
  {
    nutrient = "Medium";
  }
  else
  {
    nutrient = "Low (Deficient)";
  }


  // ==========================================================
  // SERIAL MONITOR OUTPUT
  // ==========================================================

  Serial.println("====== PLANT HEALTH REPORT ======");

  Serial.print("Raw Clear Intensity: ");
  Serial.println(raw_c);

  Serial.print("Calculated GLI Score: ");
  Serial.println(gli, 4);

  Serial.print("Plant Status:        ");
  Serial.println(plantStatus);

  Serial.print("Reflectance:         ");
  Serial.println(reflect);

  Serial.print("Environment Light:   ");
  Serial.println(env);

  Serial.print("Nutrient Status:     ");
  Serial.println(nutrient);

  Serial.println("=================================\n");


  // ==========================================================
  // BLUETOOTH DATA PACKET
  // ==========================================================
  //
  // TX Node expects:
  //
  // TREE|C=627,G=0.1084
  //
  // C = Raw Clear Intensity
  // G = GLI
  //
  // ==========================================================

  String treeDataPacket =
      "TREE|C=" +
      String(raw_c) +
      ",G=" +
      String(gli, 4);


  // ----------------------------------------------------------
  // SEND THROUGH BLUETOOTH
  // ----------------------------------------------------------

  if (SerialBT.hasClient())
  {
    SerialBT.println(treeDataPacket);

    Serial.println("Bluetooth: Tree data sent");

    Serial.print("Bluetooth Packet: ");
    Serial.println(treeDataPacket);
  }
  else
  {
    Serial.println("Bluetooth: ForestNetra_TX not connected");
  }


  // ----------------------------------------------------------
  // SENSOR UPDATE INTERVAL
  // ----------------------------------------------------------

  delay(2000);
}