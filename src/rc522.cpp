#include "rc522.h"

MFRC522 rfid(SS_PIN, RST_PIN);

//Convert byte UID -> string (to compare with JSON) 
//[0xE3, 0xA1, 0xB2, 0xC4]  → "E3A1B2C4"
String getUIDString(byte *uid, byte uidSize) {
  String uidStr = "";
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}

void rfid_reader( void*pvParameters){
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    rfid.PCD_Init();

    while(1){
      if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
        String scannedUID = getUIDString(rfid.uid.uidByte, rfid.uid.size);
        Serial.println(scannedUID);
        
        UID = scannedUID;
        xSemaphoreGive(xLCDSemaphore);
        // // Pubish UID to MQTT Broker
        // if(client.connected()){
        //   client.publish("parking/rfid/scan", scannedUID.c_str());
        // }


        // Delay 1,5s to avoid scan 1 card many times
        rfid.PICC_HaltA();            // Incicate the card to HALT state 
        rfid.PCD_StopCrypto1();       // Stop encrypte
        vTaskDelay(pdMS_TO_TICKS(1500));
      }
    vTaskDelay(pdMS_TO_TICKS(50)); // Nhường CPU cho các task khác chạy
    } 
}