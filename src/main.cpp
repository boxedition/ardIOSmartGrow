// #include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include "arduino_secrets.h"

// WIFI
char ssid[] = LABS_SECRET_SSID; // network SSID (name)
char pass[] = LABS_SECRET_PASS; // network password
int status = WL_IDLE_STATUS;    // the WiFi radio's idle status
byte mac[6];
const String remote = "api.boxdev.site";
const String local = "localhost";
const String url = remote;
// WiFiClient client; // Initialize the Wifi client
WiFiClient client;

// DHT11
#define DHTPin 2
#define DHTTYPE DHT11
DHT dht = DHT(DHTPin, DHTTYPE);

// Soil - Value represents dryness
const int DRY_VALUE = 620;
const int WET_VALUE = 310;
int soilValue = 0;
int soilPercent = 0;

// App
unsigned long previousMillis = 0;
const long APP_INTERVAL = 5000; // 5 seconds
boolean isRegisted = false;

struct KeyValue
{
  String key;
  String value;
};

// Proto
String getMacAddress(byte mac[]);
float read_sen_humidity();
float read_sen_temperature();
void postRequest(String path, KeyValue keyValue[], size_t size);

void setup()
{
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
      {"imei", (String) "" + macAdd}};
  postRequest("/api/arduino/create", payload, sizeof(payload) / sizeof(payload[0]));
  delay(5000);
  dht.begin();

  // Initialize previousMillis to current time
  previousMillis = millis() + APP_INTERVAL;
  Serial.println("Setup??");
}

void loop()
{

  /* Executes every APP_INTERVALL ms
   *
   * Note: Time and code execution matters.
   */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= APP_INTERVAL)
  {

    Serial.println((String) "\n" + "Temperature: " + read_sen_temperature() + "°C Humidity: " + read_sen_humidity() + "%");
    soilValue = analogRead(A0); // put Sensor insert into soil
    soilPercent = map(soilValue, DRY_VALUE, WET_VALUE, 0, 100);
    Serial.println((String) "Moister value:" + soilValue + " Moister Level:" + soilPercent);
    KeyValue payload[] = {
        {"temperature", (String)read_sen_temperature()},
        {"humidity", (String)read_sen_humidity()},
        {"soil_value", (String)soilValue},
        {"soil_percentage", (String)soilPercent}};
    // postRequest("/api/", payload);
    previousMillis = currentMillis;
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

void postRequest(String path, KeyValue keyValue[], size_t size)
{
  client.stop();
  Serial.println((String) "Is Wifi Connected: " + boolean(WiFi.status() == WL_CONNECTED));
  if (WiFi.status() == WL_CONNECTED)
  {

    Serial.println((String) "Attemting connection to: " + url);

    if (client.connect(url.c_str(), 80))
    {
      Serial.println("Connected to server");

      client.print("POST " + path + " HTTP/1.1\r\n");
      client.print("Host: " + url + "\r\n");
      client.print("User-Agent: ArduinoR4Wifi/0.1\r\n");
      client.print("Content-Type: application/x-www-form-urlencoded\r\n");

      Serial.println((String) "Number of elements in the array: " + size);
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

      Serial.println("Request sent \n" + client);

      // Wait for a response
      while (client.connected())
      {
        if (client.available())
        {
          char c = client.read();
          Serial.print(c);
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