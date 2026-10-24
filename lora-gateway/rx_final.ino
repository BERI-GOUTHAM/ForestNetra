
#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <RadioLib.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <esp_eap_client.h>
#include <esp_wifi.h>
#include <string.h>

// ============================================================
// FOREST NETRA - LORA RX + CAMPUS WIFI + CLOUD + INFLUXDB
// ============================================================

// ============================================================
// CAMPUS WI-FI CONFIGURATION - WPA ENTERPRISE
// ============================================================

#define WIFI_SSID       "GUEST_SECURED"
#define EAP_IDENTITY    "upload your identity"
#define EAP_USERNAME    "your username"
#define EAP_PASSWORD    "your password"

// ============================================================
// ADAFRUIT IO CONFIGURATION
// ============================================================

#define AIO_USERNAME    "gouthamberi"
#define AIO_KEY         "aio_TtnR52u9FYMaPejRhh2CGHmtTJud"
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_FEED_NAME   "forest-netra-data"

// ============================================================
// LINUX SERVER / INFLUXDB CONFIGURATION
// ============================================================

#define INFLUX_URL "http://10.13.0.19:8086/write?db=forest_netra"

// ============================================================
// SX1262 CONNECTIONS
// ============================================================

#define LORA_CS        5
#define LORA_DIO1      26
#define LORA_RESET     14
#define LORA_BUSY      27

SX1262 loraTransceiver = new Module(
  LORA_CS,
  LORA_DIO1,
  LORA_RESET,
  LORA_BUSY
);

// ============================================================
// WIFI + ADAFRUIT MQTT + HTTP CLIENT
// ============================================================

WiFiClient client;

Adafruit_MQTT_Client mqtt(
  &client,
  AIO_SERVER,
  AIO_SERVERPORT,
  AIO_USERNAME,
  AIO_KEY
);

Adafruit_MQTT_Publish forestNetraFeed(
  &mqtt,
  AIO_USERNAME "/feeds/" AIO_FEED_NAME
);

WiFiClient influxClient;

volatile bool loraPacketReceived = false;

// ============================================================
// LORA INTERRUPT CALLBACK
// ============================================================

void setLoraFlag()
{
  loraPacketReceived = true;
}

// ============================================================
// CAMPUS ENTERPRISE WIFI CONNECTION
// ============================================================

bool connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return true;

  Serial.println("\nConnecting to campus enterprise Wi-Fi...");

  // Reset the previous Wi-Fi connection.
  WiFi.disconnect(true);
  delay(500);

  WiFi.mode(WIFI_STA);
  delay(200);

  esp_err_t err;

  // Configure enterprise identity.
  err = esp_eap_client_set_identity(
    (uint8_t *)EAP_IDENTITY,
    strlen(EAP_IDENTITY)
  );

  if (err != ESP_OK)
  {
    Serial.printf("EAP identity configuration failed: %d\n", err);
    return false;
  }

  // Configure enterprise username.
  err = esp_eap_client_set_username(
    (uint8_t *)EAP_USERNAME,
    strlen(EAP_USERNAME)
  );

  if (err != ESP_OK)
  {
    Serial.printf("EAP username configuration failed: %d\n", err);
    return false;
  }

  // Configure enterprise password.
  err = esp_eap_client_set_password(
    (uint8_t *)EAP_PASSWORD,
    strlen(EAP_PASSWORD)
  );

  if (err != ESP_OK)
  {
    Serial.printf("EAP password configuration failed: %d\n", err);
    return false;
  }

  // Enable enterprise authentication.
  err = esp_wifi_sta_enterprise_enable();

  if (err != ESP_OK)
  {
    Serial.printf("Enterprise Wi-Fi enable failed: %d\n", err);
    return false;
  }

  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 30000)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nCampus Wi-Fi connection TIMEOUT.");
    Serial.print("Wi-Fi status code: ");
    Serial.println(WiFi.status());
    return false;
  }

  Serial.println("\nCampus Wi-Fi connected!");

  Serial.print("RX IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  Serial.print("Subnet mask: ");
  Serial.println(WiFi.subnetMask());

  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());

  return true;
}

// ============================================================
// TIME SYNCHRONIZATION - INDIA STANDARD TIME
// ============================================================

bool syncClock()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Cannot synchronize time: Wi-Fi unavailable.");
    return false;
  }

  time_t now = time(nullptr);

  if (now >= 1760000000)
  {
    Serial.println("System clock already has valid time.");
    return true;
  }

  Serial.println("Synchronizing clock with NTP...");

  configTime(
    19800,
    0,
    "pool.ntp.org",
    "time.google.com",
    "time.cloudflare.com"
  );

  for (int attempt = 0; attempt < 15; attempt++)
  {
    now = time(nullptr);

    if (now >= 1760000000)
    {
      Serial.println("NTP time synchronization SUCCESS.");
      return true;
    }

    delay(1000);
  }

  Serial.println("NTP synchronization FAILED.");
  return false;
}

// ============================================================
// ADD TIMESTAMP TO THE RECEIVED PAYLOAD
// ============================================================

String addTimestamp(String packet)
{
  time_t now = time(nullptr);

  if (now < 1760000000)
    return "TS=UNSYNCED," + packet;

  struct tm timeInfo;

  if (localtime_r(&now, &timeInfo) == nullptr)
    return "TS=UNSYNCED," + packet;

  char timestamp[32];

  strftime(
    timestamp,
    sizeof(timestamp),
    "%Y-%m-%dT%H:%M:%S+05:30",
    &timeInfo
  );

  return "TS=" + String(timestamp) + "," + packet;
}

// ============================================================
// ADAFRUIT IO MQTT CONNECTION
// ============================================================

bool connectMQTT()
{
  if (mqtt.connected())
    return true;

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("MQTT connection skipped: Wi-Fi unavailable.");
    return false;
  }

  Serial.println("\nConnecting to Adafruit IO...");

  for (int attempt = 1; attempt <= 3; attempt++)
  {
    Serial.print("MQTT attempt ");
    Serial.print(attempt);
    Serial.println("...");

    int8_t ret = mqtt.connect();

    if (ret == 0)
    {
      Serial.println("Adafruit IO MQTT connected!");
      return true;
    }

    Serial.print("MQTT connection failed, error: ");
    Serial.println(ret);

    mqtt.disconnect();
    delay(2000);
  }

  Serial.println("Unable to connect to Adafruit IO.");
  return false;
}

// ============================================================
// PUBLISH TO ADAFRUIT IO
// ============================================================

void publishToAdafruit(String packet)
{
  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Adafruit IO skipped: Wi-Fi unavailable.");
    return;
  }

  if (!connectMQTT())
  {
    Serial.println("Adafruit IO skipped: MQTT unavailable.");
    return;
  }

  Serial.println("\nPublishing to Adafruit IO...");
  Serial.print("Payload: ");
  Serial.println(packet);

  if (forestNetraFeed.publish(packet.c_str()))
    Serial.println("Adafruit IO publish SUCCESS!");
  else
    Serial.println("Adafruit IO publish FAILED!");
}

// ============================================================
// PUBLISH TO LINUX INFLUXDB SERVER
// ============================================================

bool publishToInfluxDB(String packet)
{
  Serial.println("\n========================================");
  Serial.println("Sending data to Linux InfluxDB server...");
  Serial.println("========================================");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("InfluxDB publish FAILED: Wi-Fi unavailable.");
    return false;
  }

  String fields = "";
  int start = 0;

  while (start < packet.length())
  {
    int comma = packet.indexOf(',', start);
    String item;

    if (comma == -1)
    {
      item = packet.substring(start);
      start = packet.length();
    }
    else
    {
      item = packet.substring(start, comma);
      start = comma + 1;
    }

    item.trim();

    int equals = item.indexOf('=');

    if (equals <= 0)
      continue;

    String key = item.substring(0, equals);
    String value = item.substring(equals + 1);

    key.trim();
    value.trim();

    // Timestamp is represented by the InfluxDB point timestamp.
    if (key == "TS")
      continue;

    // Validate field name.
    bool validKey = true;

    for (unsigned int i = 0; i < key.length(); i++)
    {
      char c = key[i];

      if (!((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
             c == '_'))
      {
        validKey = false;
        break;
      }
    }

    if (!validKey || value.length() == 0)
      continue;

    // Accept numeric values only.
    for (unsigned int i = 0; i < value.length(); i++)
    {
      char c = value[i];

      if (!((c >= '0' && c <= '9') ||
             c == '-' || c == '+' || c == '.'))
      {
        validKey = false;
        break;
      }
    }

    if (!validKey)
      continue;

    if (fields.length() > 0)
      fields += ",";

    fields += key + "=" + value;
  }

  if (fields.length() == 0)
  {
    Serial.println("InfluxDB publish FAILED: no numeric fields.");
    return false;
  }

  // Existing measurement and source tag are preserved.
  String line = "forest_netra,source=rx_gateway " + fields;

  // Use the RX gateway timestamp if synchronized.
  // Otherwise, InfluxDB assigns the server-side timestamp.
  time_t now = time(nullptr);

  if (now >= 1760000000)
  {
    char timestampNs[32];

snprintf(
  timestampNs,
  sizeof(timestampNs),
  "%lld",
  ((long long)now + 19800LL) * 1000000000LL
);

    line += " ";
    line += timestampNs;
  }

  Serial.print("InfluxDB line: ");
  Serial.println(line);

  HTTPClient http;

  if (!http.begin(influxClient, INFLUX_URL))
  {
    Serial.println("InfluxDB publish FAILED: HTTP begin failed.");
    return false;
  }

  http.setConnectTimeout(3000);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "text/plain");

  int httpCode = http.POST(line);

  Serial.print("InfluxDB HTTP response: ");
  Serial.println(httpCode);

  bool success = (httpCode == 204);

  if (success)
  {
    Serial.println("Linux server / InfluxDB publish SUCCESS!");
  }
  else
  {
    Serial.println("Linux server / InfluxDB publish FAILED!");

    if (httpCode > 0)
    {
      Serial.print("Server response: ");
      Serial.println(http.getString());
    }
    else
    {
      Serial.println(
        "Could not reach server. Check network and server IP."
      );
    }
  }

  http.end();

  Serial.println("========================================");

  return success;
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println(" FOREST NETRA - LORA RX + CAMPUS WIFI");
  Serial.println("========================================");

  // Initialize LoRa first so the radio is ready even if
  // campus Wi-Fi takes time to connect.
  Serial.println("\nInitializing SX1262...");

  int state = loraTransceiver.begin(
    865.0, 125.0, 7, 5, 0x34, 14, 8
  );

  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.print("SX1262 initialization FAILED. Error: ");
    Serial.println(state);

    while (true)
      delay(1000);
  }

  Serial.println("SX1262 initialization SUCCESS");

  loraTransceiver.setDio1Action(setLoraFlag);

  state = loraTransceiver.startReceive();

  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.print("Failed to start LoRa receive. Error: ");
    Serial.println(state);

    while (true)
      delay(1000);
  }

  Serial.println("LoRa receiver started.");
  Serial.println("Waiting for Forest Netra packet...");

  // Connect to campus enterprise Wi-Fi.
  if (connectWiFi())
  {
    syncClock();
    connectMQTT();
  }
  else
  {
    Serial.println("RX will continue trying Wi-Fi in loop().");
  }

  Serial.println("\n========================================");
  Serial.println(" RX READY");
  Serial.println("========================================\n");
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  // Maintain Wi-Fi connection.
  if (WiFi.status() != WL_CONNECTED)
  {
    if (connectWiFi())
      syncClock();
  }

  // Maintain MQTT connection for Adafruit IO.
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqtt.connected())
      connectMQTT();

    if (mqtt.connected())
    {
      mqtt.processPackets(10);
      mqtt.ping();
    }
  }

  // Handle incoming LoRa packet.
  if (loraPacketReceived)
  {
    loraPacketReceived = false;

    String receivedPacket;

    int state = loraTransceiver.readData(receivedPacket);

    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println("\n========================================");
      Serial.println("         LORA PACKET RECEIVED");
      Serial.println("========================================");

      Serial.println("Original LoRa Data:");
      Serial.println(receivedPacket);

      String timestampedPacket = addTimestamp(receivedPacket);

      Serial.println("\nTimestamped Forest Netra Data:");
      Serial.println(timestampedPacket);

      Serial.print("Message Length: ");
      Serial.println(receivedPacket.length());

      Serial.print("RSSI: ");
      Serial.print(loraTransceiver.getRSSI());
      Serial.println(" dBm");

      Serial.print("SNR: ");
      Serial.print(loraTransceiver.getSNR());
      Serial.println(" dB");

      // Send independently to both existing destinations.
      publishToAdafruit(timestampedPacket);
      publishToInfluxDB(timestampedPacket);

      Serial.println("\nReturning to LoRa receive mode...");
    }
    else
    {
      Serial.print("LoRa read failed. Error: ");
      Serial.println(state);
    }

    loraTransceiver.startReceive();

    Serial.println("Waiting for next Forest Netra packet...");
  }

  delay(10);
}
