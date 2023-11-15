//#include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include "arduino_secrets.h"

// DHT11
#define DHTPin 2
#define DHTTYPE DHT11
DHT dht = DHT(DHTPin, DHTTYPE);

// Soil - Value represents dryness
const int DRY_VALUE = 620;
const int WET_VALUE = 310;
int soilValue = 0;
int soilPercent = 0;

unsigned long previousMillis = 0;
const long APP_INTERVAL = 2000; // 2 seconds
// Proto
float read_sen_humidity();
float read_sen_temperature();

void setup()
{
  Serial.begin(9600);
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
