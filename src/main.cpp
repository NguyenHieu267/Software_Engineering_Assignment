#include "global.h"

#include "lcd.h"
#include "servo.h"
#include "tcrt5k.h"
#include "rc522.h"
// #include "mqtt.h"


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);          // remember to set in platformio.ini
  
  xTaskCreate(lcd_task, "Task LCD", 2048, NULL, 2, NULL);
  xTaskCreate(servo, "Task servo", 2048, NULL, 2, NULL);
  xTaskCreate(tcrt, "Task tcrt", 2048, NULL, 2, NULL);
  xTaskCreate(rfid_reader, "Task rc522", 4096, NULL, 2, NULL);
  // xTaskCreate(mqtt_task, "Task mqtt", 4096, NULL, 3, NULL);
}

void loop() {
  //This project using FreeRTOS so that we do not need to use "looptask" -> run 1 time then delete it
  vTaskDelete(NULL); 
}