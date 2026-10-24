#include <Arduino.h>
#include "BluetoothSerial.h"
#include <RadioLib.h>


// ======================================================
// BLUETOOTH
// ======================================================

BluetoothSerial SerialBT;

#define TX_BLUETOOTH_NAME "ForestNetra_TX"


// ======================================================
// SENSOR NODE BLUETOOTH MAC ADDRESSES
// ======================================================

// Soil Node
// 84:0D:8E:0F:4E:2A
uint8_t SOIL_NODE_ADDRESS[6] = {
  0x84,
  0x0D,
  0x8E,
  0x0F,
  0x4E,
  0x2A
};


// Tree Node
// A0:A3:B3:29:C3:BE
uint8_t TREE_NODE_ADDRESS[6] = {
  0xA0,
  0xA3,
  0xB3,
  0x29,
  0xC3,
  0xBE
};


// Environmental Node
// A4:CF:12:9E:45:0A
uint8_t ENV_NODE_ADDRESS[6] = {
  0xA4,
  0xCF,
  0x12,
  0x9E,
  0x45,
  0x0A
};


// ======================================================
// NODE NAMES
// ======================================================

#define SOIL_NODE_NAME "ForestNetra_Soil"
#define TREE_NODE_NAME "ForestNetra_Tree"
#define ENV_NODE_NAME  "ForestNetra_Environmental"


// ======================================================
// SX1262 LORA
// ======================================================
//
// SX1262      -> ESP32
// ----------------------
// NSS (CS)    -> GPIO5
// SCK         -> GPIO18
// MOSI        -> GPIO23
// MISO        -> GPIO19
// DIO1        -> GPIO26
// RESET       -> GPIO14
// BUSY        -> GPIO27
//
// ======================================================

SX1262 loraTransceiver = new Module(
  5,    // NSS / CS
  26,   // DIO1
  14,   // RESET
  27    // BUSY
);


// ======================================================
// SOIL DATA
// ======================================================

float soilMoisture = 0.0;
float soilTemperature = 0.0;
int soilEC = 0;
float soilPH = 0.0;
int soilNitrogen = 0;
int soilPhosphorus = 0;
int soilPotassium = 0;

bool soilDataAvailable = false;


// ======================================================
// ENVIRONMENTAL DATA
// ======================================================

float envTemperature = 0.0;
float envPressure = 0.0;
float envAltitude = 0.0;
float envHumidity = 0.0;
float envLight = 0.0;

bool environmentalDataAvailable = false;


// ======================================================
// TREE DATA
// ======================================================

int treeClear = 0;
float treeGLI = 0.0;

bool treeDataAvailable = false;


// ======================================================
// GET VALUE FROM COMPACT PACKET
// ======================================================

String getValue(String data, String key)
{
  int start = data.indexOf(key);

  if (start < 0)
    return "";

  start += key.length();

  int end = data.indexOf(',', start);

  if (end < 0)
    end = data.length();

  return data.substring(start, end);
}


// ======================================================
// PARSE SOIL DATA
// ======================================================
//
// Expected:
//
// M=0.20,ST=27.20,E=0,PH=7.00,N=0,PP=0,K=0
//
// ======================================================

bool parseSoilData(String data)
{
  if (!data.startsWith("M="))
    return false;

  String m  = getValue(data, "M=");
  String st = getValue(data, "ST=");
  String e  = getValue(data, "E=");
  String ph = getValue(data, "PH=");
  String n  = getValue(data, "N=");
  String pp = getValue(data, "PP=");
  String k  = getValue(data, "K=");

  if (m == "" || st == "" || e == "" ||
      ph == "" || n == "" || pp == "" || k == "")
  {
    return false;
  }

  soilMoisture = m.toFloat();
  soilTemperature = st.toFloat();
  soilEC = e.toInt();
  soilPH = ph.toFloat();
  soilNitrogen = n.toInt();
  soilPhosphorus = pp.toInt();
  soilPotassium = k.toInt();

  soilDataAvailable = true;

  return true;
}


// ======================================================
// PARSE ENVIRONMENTAL DATA
// ======================================================
//
// Expected:
//
// ENV|T=25.88,P=1003.50,A=81.47,H=78.00,L=323.33
//
// ======================================================

bool parseEnvironmentalData(String data)
{
  if (!data.startsWith("ENV|"))
    return false;

  data.remove(0, 4);

  String t = getValue(data, "T=");
  String p = getValue(data, "P=");
  String a = getValue(data, "A=");
  String h = getValue(data, "H=");
  String l = getValue(data, "L=");

  if (t == "" || p == "" || a == "" ||
      h == "" || l == "")
  {
    return false;
  }

  envTemperature = t.toFloat();
  envPressure = p.toFloat();
  envAltitude = a.toFloat();
  envHumidity = h.toFloat();
  envLight = l.toFloat();

  environmentalDataAvailable = true;

  return true;
}


// ======================================================
// PARSE TREE DATA
// ======================================================
//
// Expected:
//
// TREE|C=627,G=0.1048
//
// ======================================================

bool parseTreeData(String data)
{
  if (!data.startsWith("TREE|"))
    return false;

  data.remove(0, 5);

  String c = getValue(data, "C=");
  String g = getValue(data, "G=");

  if (c == "" || g == "")
    return false;

  treeClear = c.toInt();
  treeGLI = g.toFloat();

  treeDataAvailable = true;

  return true;
}


// ======================================================
// READ ONE BLUETOOTH PACKET
// ======================================================
//
// Sensor nodes use SerialBT.println()
// so every packet ends with '\n'.
//
// ======================================================

bool readBluetoothPacket(
    String &packet,
    unsigned long timeout
)
{
  packet = "";

  unsigned long startTime = millis();

  while (millis() - startTime < timeout)
  {
    while (SerialBT.available())
    {
      char incomingChar = SerialBT.read();

      if (incomingChar == '\n')
      {
        packet.trim();

        if (packet.length() > 0)
        {
          return true;
        }
      }
      else if (incomingChar != '\r')
      {
        packet += incomingChar;

        if (packet.length() > 250)
        {
          packet = "";

          Serial.println(
            "Bluetooth packet too long - buffer cleared."
          );
        }
      }
    }

    delay(10);
  }

  return false;
}


// ======================================================
// CONNECT TO SENSOR NODE USING MAC ADDRESS
// ======================================================

bool connectToNode(
    const char *nodeName,
    uint8_t nodeAddress[6]
)
{
  Serial.println();
  Serial.println("----------------------------------------");

  Serial.print("Connecting to: ");
  Serial.println(nodeName);

  Serial.print("MAC Address: ");

  for (int i = 0; i < 6; i++)
  {
    if (nodeAddress[i] < 0x10)
      Serial.print("0");

    Serial.print(nodeAddress[i], HEX);

    if (i < 5)
      Serial.print(":");
  }

  Serial.println();

  Serial.println("----------------------------------------");


  // Make sure no previous connection exists

  if (SerialBT.connected())
  {
    Serial.println(
      "Existing Bluetooth connection detected."
    );

    Serial.println(
      "Disconnecting previous connection..."
    );

    SerialBT.disconnect();

    delay(500);
  }


  Serial.println("Starting Bluetooth connection...");


  bool result =
      SerialBT.connect(nodeAddress);


  if (!result)
  {
    Serial.print(
      "Bluetooth connection failed: "
    );

    Serial.println(nodeName);

    return false;
  }


  // Give the connection a moment to become active

  unsigned long startTime = millis();

  while (!SerialBT.connected() &&
         millis() - startTime < 5000)
  {
    delay(50);
  }


  if (SerialBT.connected())
  {
    Serial.print(
      "Bluetooth connected to: "
    );

    Serial.println(nodeName);

    return true;
  }


  Serial.print(
    "Bluetooth connection timeout: "
  );

  Serial.println(nodeName);

  return false;
}


// ======================================================
// DISCONNECT FROM SENSOR NODE
// ======================================================

void disconnectFromNode()
{
  if (SerialBT.connected())
  {
    Serial.println();
    Serial.println("Disconnecting Bluetooth...");

    SerialBT.disconnect();

    delay(500);

    Serial.println("Bluetooth disconnected.");
  }
}


// ======================================================
// COLLECT SOIL DATA
// ======================================================

bool collectSoilData()
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("STEP 1: SOIL NODE");
  Serial.println("========================================");


  if (!connectToNode(
        SOIL_NODE_NAME,
        SOIL_NODE_ADDRESS))
  {
    soilDataAvailable = false;

    Serial.println(
      "Soil node unavailable."
    );

    Serial.println(
      "Soil values will remain ZERO."
    );

    return false;
  }


  String packet;


  Serial.println(
    "Waiting for Soil data..."
  );


  bool received =
      readBluetoothPacket(
        packet,
        5000
      );


  if (!received)
  {
    Serial.println(
      "Soil data reception TIMEOUT."
    );

    Serial.println(
      "Soil values will remain ZERO."
    );

    disconnectFromNode();

    soilDataAvailable = false;

    return false;
  }


  Serial.println();
  Serial.println("Soil packet received:");

  Serial.println(packet);


  if (!parseSoilData(packet))
  {
    Serial.println(
      "Invalid Soil packet."
    );

    Serial.println(
      "Soil values will remain ZERO."
    );

    disconnectFromNode();

    soilDataAvailable = false;

    return false;
  }


  Serial.println();
  Serial.println("SOIL DATA");

  Serial.print("Moisture: ");
  Serial.println(soilMoisture);

  Serial.print("Temperature: ");
  Serial.println(soilTemperature);

  Serial.print("EC: ");
  Serial.println(soilEC);

  Serial.print("pH: ");
  Serial.println(soilPH);

  Serial.print("Nitrogen: ");
  Serial.println(soilNitrogen);

  Serial.print("Phosphorus: ");
  Serial.println(soilPhosphorus);

  Serial.print("Potassium: ");
  Serial.println(soilPotassium);


  disconnectFromNode();


  return true;
}


// ======================================================
// COLLECT TREE DATA
// ======================================================

bool collectTreeData()
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("STEP 2: TREE NODE");
  Serial.println("========================================");


  if (!connectToNode(
        TREE_NODE_NAME,
        TREE_NODE_ADDRESS))
  {
    treeDataAvailable = false;

    Serial.println(
      "Tree node unavailable."
    );

    Serial.println(
      "Tree values will remain ZERO."
    );

    return false;
  }


  String packet;


  Serial.println(
    "Waiting for Tree data..."
  );


  bool received =
      readBluetoothPacket(
        packet,
        5000
      );


  if (!received)
  {
    Serial.println(
      "Tree data reception TIMEOUT."
    );

    Serial.println(
      "Tree values will remain ZERO."
    );

    disconnectFromNode();

    treeDataAvailable = false;

    return false;
  }


  Serial.println();
  Serial.println("Tree packet received:");

  Serial.println(packet);


  if (!parseTreeData(packet))
  {
    Serial.println(
      "Invalid Tree packet."
    );

    Serial.println(
      "Tree values will remain ZERO."
    );

    disconnectFromNode();

    treeDataAvailable = false;

    return false;
  }


  Serial.println();
  Serial.println("TREE DATA");

  Serial.print("Clear intensity: ");
  Serial.println(treeClear);

  Serial.print("GLI: ");
  Serial.println(treeGLI);


  disconnectFromNode();


  return true;
}


// ======================================================
// COLLECT ENVIRONMENTAL DATA
// ======================================================

bool collectEnvironmentalData()
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("STEP 3: ENVIRONMENTAL NODE");
  Serial.println("========================================");


  if (!connectToNode(
        ENV_NODE_NAME,
        ENV_NODE_ADDRESS))
  {
    environmentalDataAvailable = false;

    Serial.println(
      "Environmental node unavailable."
    );

    Serial.println(
      "Environmental values will remain ZERO."
    );

    return false;
  }


  String packet;


  Serial.println(
    "Waiting for Environmental data..."
  );


  bool received =
      readBluetoothPacket(
        packet,
        5000
      );


  if (!received)
  {
    Serial.println(
      "Environmental data reception TIMEOUT."
    );

    Serial.println(
      "Environmental values will remain ZERO."
    );

    disconnectFromNode();

    environmentalDataAvailable = false;

    return false;
  }


  Serial.println();
  Serial.println(
    "Environmental packet received:"
  );

  Serial.println(packet);


  if (!parseEnvironmentalData(packet))
  {
    Serial.println(
      "Invalid Environmental packet."
    );

    Serial.println(
      "Environmental values will remain ZERO."
    );

    disconnectFromNode();

    environmentalDataAvailable = false;

    return false;
  }


  Serial.println();
  Serial.println("ENVIRONMENTAL DATA");

  Serial.print("Temperature: ");
  Serial.println(envTemperature);

  Serial.print("Pressure: ");
  Serial.println(envPressure);

  Serial.print("Altitude: ");
  Serial.println(envAltitude);

  Serial.print("Humidity: ");
  Serial.println(envHumidity);

  Serial.print("Light: ");
  Serial.println(envLight);


  disconnectFromNode();


  return true;
}


// ======================================================
// BUILD COMPLETE FOREST NETRA MESSAGE
// ======================================================
//
// Final format:
//
// T=...,P=...,A=...,H=...,L=...,C=...,G=...,
// M=...,ST=...,E=...,PH=...,N=...,PP=...,K=...
//
// Missing sensor data automatically remains ZERO.
//
// ======================================================

String buildFullCompactMessage()
{
  String message =

      "T=" + String(envTemperature, 2) +

      ",P=" + String(envPressure, 2) +

      ",A=" + String(envAltitude, 2) +

      ",H=" + String(envHumidity, 2) +

      ",L=" + String(envLight, 2) +

      ",C=" + String(treeClear) +

      ",G=" + String(treeGLI, 4) +

      ",M=" + String(soilMoisture, 2) +

      ",ST=" + String(soilTemperature, 2) +

      ",E=" + String(soilEC) +

      ",PH=" + String(soilPH, 2) +

      ",N=" + String(soilNitrogen) +

      ",PP=" + String(soilPhosphorus) +

      ",K=" + String(soilPotassium);


  return message;
}


// ======================================================
// SEND MESSAGE THROUGH LORA
// ======================================================

void sendLoRaMessage(String message)
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("PREPARING LORA TRANSMISSION");
  Serial.println("========================================");


  Serial.println(
    "Complete Forest Netra message:"
  );

  Serial.println(message);


  Serial.print("Message length: ");
  Serial.println(message.length());


  int txState =
      loraTransceiver.transmit(message);


  if (txState == RADIOLIB_ERR_NONE)
  {
    Serial.println();

    Serial.println(
      "LoRa Transmission: SUCCESS"
    );

    Serial.println(
      "Complete Forest Netra packet sent."
    );
  }
  else
  {
    Serial.print(
      "LoRa Transmission FAILED. Error code: "
    );

    Serial.println(txState);
  }


  Serial.println(
    "========================================"
  );
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("========================================");
  Serial.println("FOREST NETRA TX NODE");
  Serial.println("ESP32 + BLUETOOTH MASTER + SX1262");
  Serial.println("========================================");


  // ====================================================
  // BLUETOOTH MASTER
  // ====================================================

  if (!SerialBT.begin(
        TX_BLUETOOTH_NAME,
        true))
  {
    Serial.println(
      "Bluetooth initialization FAILED!"
    );


    while (true)
    {
      delay(1000);
    }
  }


  Serial.println(
    "Bluetooth Master Ready"
  );


  Serial.print(
    "Bluetooth Device Name: "
  );

  Serial.println(
    TX_BLUETOOTH_NAME
  );


  Serial.print(
    "TX Bluetooth MAC Address: "
  );

  Serial.println(
    SerialBT.getBtAddressString()
  );


  Serial.println();

  Serial.println(
    "Configured Sensor Nodes:"
  );


  Serial.println(
    "Soil: 84:0D:8E:0F:4E:2A"
  );


  Serial.println(
    "Tree: A0:A3:B3:29:C3:BE"
  );


  Serial.println(
    "Environmental: A4:CF:12:9E:45:0A"
  );


  // ====================================================
  // LORA INITIALIZATION
  // ====================================================

  Serial.println();

  Serial.println(
    "Initializing LoRa SX1262..."
  );


  int initState =
      loraTransceiver.begin(
        865.0,   // Frequency MHz
        125.0,   // Bandwidth kHz
        7,       // Spreading Factor
        5,       // Coding Rate 4/5
        0x34,    // Sync Word
        14,      // Output Power dBm
        8        // Preamble Length
      );


  if (initState != RADIOLIB_ERR_NONE)
  {
    Serial.print(
      "LoRa Initialization Failed: "
    );

    Serial.println(initState);


    while (true)
    {
      delay(1000);
    }
  }


  Serial.println(
    "LoRa SX1262 Ready"
  );

  Serial.println(
    "LoRa Frequency: 865 MHz"
  );


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "TX NODE READY"
  );

  Serial.println(
    "========================================"
  );
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
  Serial.println();
  Serial.println();

  Serial.println(
    "########################################"
  );

  Serial.println(
    "NEW FOREST NETRA COLLECTION CYCLE"
  );

  Serial.println(
    "########################################"
  );


  // ====================================================
  // RESET CURRENT CYCLE STATUS AND DATA
  // ====================================================
  //
  // IMPORTANT:
  // This prevents old sensor values from being used
  // when a node fails in the current cycle.
  //
  // ====================================================

  soilDataAvailable = false;
  treeDataAvailable = false;
  environmentalDataAvailable = false;


  // Reset Soil data

  soilMoisture = 0.0;
  soilTemperature = 0.0;
  soilEC = 0;
  soilPH = 0.0;
  soilNitrogen = 0;
  soilPhosphorus = 0;
  soilPotassium = 0;


  // Reset Environmental data

  envTemperature = 0.0;
  envPressure = 0.0;
  envAltitude = 0.0;
  envHumidity = 0.0;
  envLight = 0.0;


  // Reset Tree data

  treeClear = 0;
  treeGLI = 0.0;


  // ====================================================
  // STEP 1 - SOIL
  // ====================================================

  bool soilOK =
      collectSoilData();


  // ====================================================
  // STEP 2 - TREE
  // ====================================================

  bool treeOK =
      collectTreeData();


  // ====================================================
  // STEP 3 - ENVIRONMENTAL
  // ====================================================

  bool environmentalOK =
      collectEnvironmentalData();


  // ====================================================
  // CHECK COLLECTION RESULT
  // ====================================================

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "COLLECTION CYCLE RESULT"
  );

  Serial.println(
    "========================================"
  );


  Serial.print("Soil: ");

  if (soilOK)
  {
    Serial.println("RECEIVED");
  }
  else
  {
    Serial.println("FAILED -> USING ZERO VALUES");
  }


  Serial.print("Tree: ");

  if (treeOK)
  {
    Serial.println("RECEIVED");
  }
  else
  {
    Serial.println("FAILED -> USING ZERO VALUES");
  }


  Serial.print("Environmental: ");

  if (environmentalOK)
  {
    Serial.println("RECEIVED");
  }
  else
  {
    Serial.println("FAILED -> USING ZERO VALUES");
  }


  // ====================================================
  // ALWAYS TRANSMIT
  // ====================================================
  //
  // Working nodes contain their received values.
  //
  // Failed nodes contain ZERO because all values were
  // reset at the beginning of this collection cycle.
  //
  // Even if all three nodes fail, a zero packet is sent.
  //
  // ====================================================

  Serial.println();

  Serial.println(
    "Building Forest Netra packet..."
  );


  String completeMessage =
      buildFullCompactMessage();


  sendLoRaMessage(
    completeMessage
  );


  // ====================================================
  // COLLECTION CYCLE COMPLETE
  // ====================================================

  Serial.println();

  Serial.println(
    "Collection cycle complete."
  );


  // Small pause before next cycle

  delay(1000);
}