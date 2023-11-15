// #include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include "arduino_secrets.h"

// WIFI
char ssid[] = LABS_SECRET_SSID; // network SSID (name)
char pass[] = LABS_SECRET_PASS; // network password
int status = WL_IDLE_STATUS;    // the WiFi radio's idle status
byte mac[6];
const String url = "http://api.boxdev.site/";
WiFiClient client; // Initialize the Wifi client

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
const long APP_INTERVAL = 2000; // 2 seconds
boolean isRegisted = false;

// Proto
void printMacAddress(byte mac[]);
float read_sen_humidity();
float read_sen_temperature();

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
  WiFi.macAddress(mac);
  printMacAddress(mac);
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
    // Update Millis
    previousMillis = currentMillis;
  }
}

void printMacAddress(byte mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if (mac[i] < 16)
    {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0)
    {
      Serial.print(":");
    }
  }
  Serial.println();
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
