#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Wifi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "LiquidCrystal_I2C.h"
#include "MFRC522.h"
#include "ArduinoJson.h"
#include "ESP32Servo.h"
#include "PubSubClient.h"

// Semaphore
extern SemaphoreHandle_t xGateSemaphore;
extern SemaphoreHandle_t xLCDSemaphore;

// Variables 
extern int total_slots;
extern int empty;

extern String UID;

// extern WiFiClient espClient;
// extern PubSubClient client;

#endif