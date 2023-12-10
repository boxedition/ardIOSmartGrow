// #include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"

// WIFI
char ssid[] = SECRET_SSID;   // network SSID (name)
char pass[] = SECRET_PASS;   // network password
int status = WL_IDLE_STATUS; // the WiFi radio's idle status
byte mac[6];
const String url = "api.boxdev.site";

// Initialize the Wifi client
WiFiClient client;

// DHT11
#define DHTPin 12
#define DHTTYPE DHT11
DHT dht = DHT(DHTPin, DHTTYPE);
const byte sensorDHTVCC = 13; // VCC for DHT

// Soil - Value represents dryness
const byte sensorSoil = A0;
const byte sensorSoilVCC = 7; // VCC for Soil Sensor
const int DRY_VALUE = 620;
const int WET_VALUE = 310;
int min_water_auto = 50;
unsigned int soilValue = 0;
unsigned int soilPercent = 0;

// App
const bool defaultAnalyse = false;
unsigned long appMillis = 0;
const long APP_INTERVAL = 2000;         // Main Loop Interval
unsigned long httpStartTime = millis(); // HTTP Time
unsigned int RELAY_TRIGGER_LEVEL = 60;

// Relay
const byte relayVCC = 11;
const byte relaySignal = 10;
const long RELAY_CHECK_INTERVAL = 500;
unsigned long relayMillis = 0;
bool overrideRelay = false;
StaticJsonDocument<96> doc;

struct KeyValue
{
  String key;
  String value;
};

// Proto
String getMacAddress(byte mac[]);
float read_sen_humidity();
float read_sen_temperature();
void postRequest(String path, KeyValue keyValue[], size_t size, bool analyzeResponse = false);
void relayCheck(bool state = false);

void setup()
{
  // Relay Setup
  pinMode(relayVCC, OUTPUT);
  digitalWrite(relayVCC, HIGH);
  pinMode(relaySignal, OUTPUT);
  digitalWrite(relayVCC, LOW);

  // Soil Setup
  pinMode(sensorSoilVCC, OUTPUT);
  digitalWrite(sensorSoilVCC, HIGH);
  // DHTV Setup
  pinMode(sensorDHTVCC, OUTPUT);
  digitalWrite(sensorDHTVCC, HIGH);

  // Initialize serial and wait for port to open
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect.
  }

  // check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  while (status != WL_CONNECTED)
  {
    Serial.println((String) "Attempting to connect wifi: " + ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    delay(500);
  }

  Serial.println((String) "You're connected to: " + ssid);
  IPAddress ipAdd = WiFi.localIP();
  String macAdd = getMacAddress(WiFi.macAddress(mac));

  Serial.println((String) "Mac: " + macAdd);
  Serial.println((String) "IP: " + ipAdd);
  KeyValue payload[] = {
      {"imei", (String)macAdd}};
  postRequest("/api/arduino/create", payload, sizeof(payload) / sizeof(payload[0]));
  delay(500);
  dht.begin();

  // Initialize appMillis to current time
  appMillis = millis() + APP_INTERVAL;
  Serial.println("[INFO] Setup Completed!");
}

void loop()
{
  unsigned long currentMillis = millis();

  /* Executes every APP_INTERVALL ms
   *
   * Note: Time and code execution matters.
   */
  if (currentMillis - appMillis >= APP_INTERVAL)
  {
    Serial.println((String) "\n=====> LOGGING VAR");
    // Serial.println((String) "\n" + "Temperature: " + read_sen_temperature() + "°C Humidity: " + read_sen_humidity() + "%");
    soilValue = analogRead(sensorSoil); // put Sensor insert into soil
    soilPercent = map(soilValue, DRY_VALUE, WET_VALUE, 0, 100);
    // Serial.println((String) "Moister value:" + soilValue + " Moister Level:" + soilPercent);
    KeyValue payload[] = {
        {"imei", (String)getMacAddress(WiFi.macAddress(mac))},
        {"temperature", (String)read_sen_temperature()},
        {"humidity", (String)read_sen_humidity()},
        {"soil_value", (String)soilValue},
        {"soil_percentage", (String)soilPercent}};
    postRequest("/api/arduino/log", payload, sizeof(payload) / sizeof(payload[0]));
    appMillis = currentMillis;
  }
  else
  {
    Serial.println((String) "\n ===> SERVER");
    KeyValue payloadRelay[] = {
        {"imei", (String)getMacAddress(WiFi.macAddress(mac))}};
    postRequest("/api/arduino/water", payloadRelay, sizeof(payloadRelay) / sizeof(payloadRelay[0]), true);
    relayCheck();
  }
}

/// @brief getMacAddress
/// @param mac Mac from WiFi.macAddress()
/// @return Mac as String
String getMacAddress(byte mac[])
{
  String macAddress;

  for (int i = 5; i >= 0; i--)
  {
    if (mac[i] < 16)
    {
      macAddress += "0";
    }
    macAddress += String(mac[i], HEX);
    // Adds : to Mac
    // if (i > 0) {
    //   macAddress += ":";
    // }
  }

  return macAddress;
}

float read_sen_humidity()
{
  float h = dht.readHumidity();
  return h;
}

float read_sen_temperature()
{
  float t = dht.readTemperature();
  return t;
}

void relayCheck(bool state)
{
  digitalWrite(relaySignal, state || overrideRelay ? HIGH : LOW);
}

void postRequest(String path, KeyValue keyValue[], size_t size, bool analyzeResponse)
{
  // Serial.println((String) "Is Wifi Connected: " + boolean(WiFi.status() == WL_CONNECTED));
  if (WiFi.status() == WL_CONNECTED)
  {

    Serial.println((String) "Connecting: " + url + path);

    if (client.connect(url.c_str(), 80))
    {
      // Serial.println("Connected to server");

      client.print("POST " + path + " HTTP/1.1\r\n");
      client.print("Host: " + url + "\r\n");
      client.print("User-Agent: ArduinoR4Wifi/0.1\r\n");
      client.print("Content-Type: application/x-www-form-urlencoded\r\n");

      // Serial.println((String) "Number of elements in the array: " + size);
      String requestBody = "";

      // Construct the request body with all key-value pairs
      for (int i = 0; i < size; i++)
      {
        requestBody += keyValue[i].key + "=" + keyValue[i].value;
        if (i < size - 1)
        {
          requestBody += "&";
        }
      }

      client.print("Content-Length: " + String(requestBody.length()) + "\r\n");
      client.print("\r\n");
      client.print(requestBody);

      // Serial.println("Request sent \n" + client);
      if (analyzeResponse)
      {
        String response = client.readString();

        // Find the position of the double line break
        int jsonStart = response.indexOf("\r\n\r\n");
        if (jsonStart != -1)
        {
          // Extract only the JSON payload
          String jsonResponse = response.substring(jsonStart + 4);
          DeserializationError error = deserializeJson(doc, jsonResponse);

          // Check if deserialization was successful
          if (error)
          {
            Serial.print("Deserialization error: ");
            Serial.println(error.c_str());
          }
          else
          {
            //  Access the deserialized data
            overrideRelay = doc["water"];
          }
        }
      }

      client.stop();
      Serial.println("\nConnection closed");
    }
    else
    {
      Serial.println("Connection failed");
    }
  }
}

/*
void postRequest(String path, KeyValue keyValue[], size_t size, bool analyzeResponse)
{
  httpStartTime = millis();
  client.stop();
  // Serial.println((String) "Is Wifi Connected: " + boolean(WiFi.status() == WL_CONNECTED));
  if (WiFi.status() == WL_CONNECTED)
  {

    Serial.println((String) "Attemting connection to: " + url);

    if (client.connect(url.c_str(), 80))
    {
      // Serial.println("Connected to server");

      client.print("POST " + path + " HTTP/1.1\r\n");
      client.print("Host: " + url + "\r\n");
      client.print("User-Agent: ArduinoR4Wifi/0.1\r\n");
      client.print("Content-Type: application/x-www-form-urlencoded\r\n");

      // Serial.println((String) "Number of elements in the array: " + size);
      String requestBody = "";

      // Construct the request body with all key-value pairs
      for (int i = 0; i < size; i++)
      {
        requestBody += keyValue[i].key + "=" + keyValue[i].value;
        if (i < size - 1)
        {
          requestBody += "&";
        }
      }

      client.print("Content-Length: " + String(requestBody.length()) + "\r\n");
      client.print("\r\n");
      client.print(requestBody);

      // Serial.println("Request sent \n" + client);

      while (client.connected())
      {
        String response = client.readString();

        if (client.available())
        {
          Serial.println("Response:" + response);
          // Deserialize the JSON response
          DeserializationError error = deserializeJson(doc, response);

          // Check if deserialization was successful
          if (error)
          {
            Serial.print("Deserialization error: ");
            Serial.println(error.c_str());
          }
          else
          {
            //Serial.println("DOC:" + (String)doc);
            // Access the deserialized data
            bool water = doc["water"];
            Serial.print("Water: ");
            Serial.println(water);
          }
        }
        //if (millis() - httpStartTime >= HTTP_TIMEOUT_DURATION)
        //{
        //  Serial.println("\nTimeout occurred");
        //  break;
        //}
      }
      client.stop();
      Serial.println("\nConnection closed");
    }
    else
    {
      Serial.println("Connection failed");
    }
  }
}
*/