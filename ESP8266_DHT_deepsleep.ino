/*
 * DIY Weather Station 
 * http://lazyzero.de/en/elektronik/esp8266/diy_weather/
 * Copyright 2016 Christian Moll
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *   http://www.apache.org/licenses/LICENSE-2.0
 *   
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"
#include <MQTTClient.h>
#include "config.h"

#define DHTTYPE DHT22
#define DHTPIN D5

#define FORCE_DEEPSLEEP //comment when you like to use battery level dependent sleep

WiFiClient wifi;
MQTTClient mqtt;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp280; 

unsigned int batt;
double battV;

void connect();

void setup(void){
 
  dht.begin();

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Wire.begin();
  if (!bmp280.begin(0x76)) {  
    Serial.println("no BMP280 detected");
    delay(1000);
  }
  mqtt.begin(broker, wifi);

  connect();
  Serial.printf("ready!");
}

void loop(void){
  if(!mqtt.connected()) {
    connect();
  }
  
  mqtt.loop();
  delay(30000); // time to make reprogramming possible
    
  batt = analogRead(A0);
  battV = batt * (4.2/827); //440kOhm/100kOhm 
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
 //Only send value for DHT when read successfully
  if (!isnan(h) || !isnan(t)) {
    mqtt.publish("/room1/dht22/temp", String(t));
    mqtt.publish("/room1/dht22/humidity", String(h));
  }
  
  mqtt.publish("/room1/batt", String(battV));
  mqtt.publish("/room1/battRaw", String(batt));
  mqtt.publish("/room1/resetReason", ESP.getResetReason());
  mqtt.publish("/room1/bmp280/temp", String(bmp280.readTemperature()));
  mqtt.publish("/room1/bmp280/pressure", String(bmp280.readPressure()/100));

  //Work with a fixed sleep cycle
  #ifdef FORCE_DEEPSLEEP
    Serial.println("Force deepsleep 15min!");
    ESP.deepSleep(15 * 60 * 1000000);
    delay(100);
  #endif
  
  //or handle deep sleep depending on battV
  if (battV < 3.3) {
    ESP.deepSleep(30 * 1000000); //send IFTTT low_bat warning
    delay(100);
  } else if (battV < 4.0) {
    ESP.deepSleep(2 * 1000000);
    delay(100);
  }
}

void connect() {
  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  while (!mqtt.connect(mqttClientId, mqttUser, mqttPwd)) {
    Serial.print(".");
  }
  Serial.println("\nconnected!");
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}


