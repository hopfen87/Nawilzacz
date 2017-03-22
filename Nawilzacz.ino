/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Refactored and addjusted for Air humidifier by Marcin Chmielewski <marcin.chmielewski@gmail.com>
 * Copyright (C) 2013-2015 Sensnology AB & 2017 Hopfen87
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 * The letest varsion of Air Humidifier is available here: https://github.com/hopfen87/Nawilzacz
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * Version 1.1 - Marcin Chmielewski
 * 
 * DESCRIPTION
 * This sketch provides full automation of air humidifier. Controls humidity and temperature by DHT-22 sensor
 * and allows remote control of Stadler Form Oscar Little humidifier.
 * 
 */

#define MY_NODE_ID 9
// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>  
#include <DHT.h>  

#define CHILD_ID_HUM 10
#define CHILD_ID_TEMP 11
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
unsigned long lastRefreshTime = 0; // Use this to implement a non-blocking delay function

#define RELAY_1  4  // Arduino Digital I/O pin number for first relay PIN 4 for POWER BUTTON; PIN 5 for SPEED BUTTON; PIN 6 for DIMMER BUTTON
#define NUMBER_OF_RELAYS 3 // Total number of attached relays 
#define RELAY_ON 1  // GPIO value to write to simulate not pressed button
#define RELAY_OFF 0 // GPIO value to write to simulate pressed button

#define SPEED1_SENSOR_PIN 7
#define SPEED2_SENSOR_PIN 8
#define LIGHT_SENSOR_PIN  14
#define WATER_SENSOR_PIN  15

#define SPEED1_SENSOR_CHILD_ID 7
#define SPEED2_SENSOR_CHILD_ID 8
#define LIGHT_SENSOR_CHILD_ID  14
#define WATER_SENSOR_CHILD_ID  15



DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
int speed1OldValue = -1;
int speed2OldValue = -1;


MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgSpeed1(SPEED1_SENSOR_CHILD_ID, V_STATUS);
MyMessage msgSpeed2(SPEED2_SENSOR_CHILD_ID, V_STATUS);


void setup()  
{ 
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  metric = getControllerConfig().isMetric;

  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay (push button) default value
    digitalWrite(pin, RELAY_ON);
  }
    pinMode(SPEED1_SENSOR_PIN, INPUT_PULLUP);
    pinMode(SPEED2_SENSOR_PIN, INPUT_PULLUP);

}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Nawilżacz", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);

  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++) {
    // Register all sensors to gw (they will be created as child devices)
    present(sensor, S_BINARY);
  }

  present(SPEED1_SENSOR_CHILD_ID, S_BINARY);
  present(SPEED2_SENSOR_CHILD_ID, S_BINARY);
}


void loop()      
{ 
  boolean needRefresh = (millis() - lastRefreshTime) > SLEEP_TIME;
  if (needRefresh)
  {
      lastRefreshTime = millis();
      
      float temperature = dht.getTemperature();
      if (isnan(temperature)) {
          Serial.println("Failed reading temperature from DHT");
      } else if (temperature != lastTemp) {
        lastTemp = temperature;
        if (!metric) {
          temperature = dht.toFahrenheit(temperature);
        }
        send(msgTemp.set(temperature, 1));
        Serial.print("T: ");
        Serial.println(temperature);
      }
      
      float humidity = dht.getHumidity();
      if (isnan(humidity)) {
          Serial.println("Failed reading humidity from DHT");
      } else if (humidity != lastHum) {
          lastHum = humidity;
          send(msgHum.set(humidity, 1));
          Serial.print("H: ");
          Serial.println(humidity);
      }

  }

  int speed1SensorVal = digitalRead(SPEED1_SENSOR_PIN);
  if (speed1SensorVal != speed1OldValue) {
     // Send in the new value
     send(msgSpeed1.set(speed1SensorVal==HIGH ? 1 : 0));
     speed1OldValue = speed1SensorVal;
  }
  int speed2SensorVal = digitalRead(SPEED2_SENSOR_PIN);
  if (speed2SensorVal != speed2OldValue) {
     // Send in the new value
     send(msgSpeed2.set(speed2SensorVal==HIGH ? 1 : 0));
     speed2OldValue = speed2SensorVal;
  }
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_STATUS) {
     // Change relay state
     digitalWrite(message.sensor-1+RELAY_1, message.getBool()?RELAY_OFF:RELAY_ON);
     // Store state in eeprom - saving state is not necessary
     // saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}
