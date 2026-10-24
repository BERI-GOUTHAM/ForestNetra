#include <Wire.h>
#include <Adafruit_BMP3XX.h>
#include <BH1750.h>
#include <DHT.h>
#include <BluetoothSerial.h>


// ============================================================
// PIN DEFINITIONS
// ============================================================

// ESP32 I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// DHT11
#define DHT_PIN 14
#define DHT_TYPE DHT11


// ============================================================
// BLUETOOTH
// ============================================================

BluetoothSerial SerialBT;

#define BLUETOOTH_NAME "ForestNetra_Environmental"


// ============================================================
// SENSOR OBJECTS
// ============================================================

Adafruit_BMP3XX bmp;
BH1750 lightMeter;
DHT dht(DHT_PIN, DHT_TYPE);


// ============================================================
// SENSOR STATUS
// ============================================================

bool bmp390Available = false;
bool bh1750Available = false;


// ============================================================
// SENSOR VALUES FOR BLUETOOTH
// ============================================================

float environmentalTemperature = 0.0;
float environmentalPressure = 0.0;
float environmentalAltitude = 0.0;

float dhtTemperatureValue = 0.0;
float humidityValue = 0.0;

float lightValue = 0.0;


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" ESP32 Environmental Monitoring System");
  Serial.println("======================================");


  // =================================================
  // INITIALIZE I2C
  // =================================================

  Wire.begin(SDA_PIN, SCL_PIN);


  // =================================================
  // BMP390
  // =================================================

  Serial.println();
  Serial.println("Checking BMP390...");

  if (bmp.begin_I2C())
  {
    bmp390Available = true;

    Serial.println("BMP390: DETECTED");

    // BMP390 configuration
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  }
  else
  {
    bmp390Available = false;

    Serial.println("BMP390: NOT FOUND");
    Serial.println("Continuing without BMP390...");
  }


  // =================================================
  // BH1750
  // =================================================

  Serial.println();
  Serial.println("Checking BH1750...");

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    bh1750Available = true;

    Serial.println("BH1750: DETECTED");
  }
  else
  {
    bh1750Available = false;

    Serial.println("BH1750: NOT FOUND");
  }


  // =================================================
  // DHT11
  // =================================================

  Serial.println();
  Serial.println("Initializing DHT11...");

  dht.begin();

  Serial.println("DHT11: INITIALIZED");


  // =================================================
  // BLUETOOTH SERVER
  // =================================================

  Serial.println();
  Serial.println("Initializing Bluetooth...");

  // false = Bluetooth server mode
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

  Serial.print("Environmental Node Bluetooth MAC: ");
  Serial.println(SerialBT.getBtAddressString());

  Serial.println("Waiting for ForestNetra_TX...");


  // =================================================

  Serial.println();
  Serial.println("======================================");
  Serial.println("Initialization complete");
  Serial.println("======================================");
  Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // =================================================
  // SENSOR READINGS
  // =================================================

  Serial.println();
  Serial.println("======================================");
  Serial.println("        SENSOR READINGS");
  Serial.println("======================================");


  // =================================================
  // BMP390
  // =================================================

  bool bmpReadingAvailable = false;

  if (bmp390Available)
  {
    if (bmp.performReading())
    {
      float temperature = bmp.temperature;

      float pressurePa = bmp.pressure;

      float pressurehPa = pressurePa / 100.0;

      // Standard sea-level pressure
      float altitude = bmp.readAltitude(1013.25);


      // Store values for Bluetooth
      environmentalTemperature = temperature;
      environmentalPressure = pressurehPa;
      environmentalAltitude = altitude;

      bmpReadingAvailable = true;


      Serial.println();
      Serial.println("[ BMP390 ]");

      Serial.print("Temperature : ");
      Serial.print(temperature);
      Serial.println(" °C");

      Serial.print("Pressure    : ");
      Serial.print(pressurePa);
      Serial.println(" Pa");

      Serial.print("Pressure    : ");
      Serial.print(pressurehPa);
      Serial.println(" hPa");

      Serial.print("Altitude    : ");
      Serial.print(altitude);
      Serial.println(" m");
    }
    else
    {
      Serial.println();
      Serial.println("[ BMP390 ]");
      Serial.println("Reading failed");
    }
  }
  else
  {
    Serial.println();
    Serial.println("[ BMP390 ]");
    Serial.println("NOT AVAILABLE");
  }


  // =================================================
  // DHT11
  // =================================================

  float dhtTemperature = dht.readTemperature();

  float humidity = dht.readHumidity();


  Serial.println();
  Serial.println("[ DHT11 ]");

  if (isnan(dhtTemperature) || isnan(humidity))
  {
    Serial.println("Reading failed");
  }
  else
  {
    dhtTemperatureValue = dhtTemperature;
    humidityValue = humidity;

    Serial.print("Temperature : ");
    Serial.print(dhtTemperature);
    Serial.println(" °C");

    Serial.print("Humidity    : ");
    Serial.print(humidity);
    Serial.println(" %RH");
  }


  // =================================================
  // BH1750
  // =================================================

  Serial.println();
  Serial.println("[ BH1750 ]");

  bool lightReadingAvailable = false;

  if (bh1750Available)
  {
    float lux = lightMeter.readLightLevel();

    if (lux >= 0)
    {
      lightValue = lux;

      lightReadingAvailable = true;

      Serial.print("Light       : ");
      Serial.print(lux);
      Serial.println(" lux");
    }
    else
    {
      Serial.println("Reading failed");
    }
  }
  else
  {
    Serial.println("NOT AVAILABLE");
  }


  // =================================================

  Serial.println();
  Serial.println("======================================");


  // =================================================
  // BLUETOOTH TRANSMISSION
  // =================================================
  //
  // Format:
  //
  // ENV|T=27.54,P=1003.63,A=80.41,H=77.00,L=305.00
  //
  // T = BMP390 temperature
  // P = BMP390 pressure
  // A = BMP390 altitude
  // H = DHT11 humidity
  // L = BH1750 light
  //
  // =================================================

  if (SerialBT.hasClient())
  {
    String environmentalDataPacket =
        "ENV|T=" +
        String(environmentalTemperature, 2) +
        ",P=" +
        String(environmentalPressure, 2) +
        ",A=" +
        String(environmentalAltitude, 2) +
        ",H=" +
        String(humidityValue, 2) +
        ",L=" +
        String(lightValue, 2);


    SerialBT.println(environmentalDataPacket);

    Serial.println("Bluetooth: Environmental data sent");

    Serial.print("Bluetooth Packet: ");
    Serial.println(environmentalDataPacket);
  }
  else
  {
    Serial.println("Bluetooth: ForestNetra_TX not connected");
  }


  // =================================================
  // DHT11 SHOULD NOT BE READ TOO FREQUENTLY
  // =================================================

  delay(2000);
}