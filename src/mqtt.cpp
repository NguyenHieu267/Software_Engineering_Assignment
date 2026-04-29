// #include "mqtt.h"

// const char* WIFI_SSID = "NgHao";
// const char* WIFI_PASS = "nguyenhao110403";
// const char* MQTT_SERVER = "192.168.1.8";

// void reconnect(){
//     while(!client.connected()){
//         Serial.println("Connecting....");
//         //Because Mosquitto using auth -> connect(id, user, pass) 
//         String clientId = "ESP32Client-";
//         clientId += String(random(0xffff), HEX);
//         if(client.connect(clientId.c_str())){   //convert string -> const char*
//             client.subscribe("parking/rfid/response");
//         }else{
//             Serial.print("failed, rc=");
//             Serial.print(client.state());
//             Serial.println(" try again in 5 seconds");
//             vTaskDelay(pdMS_TO_TICKS(5000));
//         }

//         }
// }

// // When IoT broker rcv topic -> convert byte into string for processing
// void callback (char*topic, byte *payload, unsigned int length){
//     String message = "";
//     for (int i = 0; i < length; i++) {
//         message += (char)payload[i];
//     }

//     Serial.println(topic);
//     Serial.println("Payload: " + message);
//     if(String(topic) == "parking/rfid/response"){
//         if(message == "VALID"){
//             xSemaphoreGive(xGateSemaphore);
//         }
//     }
// }

// void mqtt_task(void *pvParameters) {
//     WiFi.begin(WIFI_SSID, WIFI_PASS);
//     while (WiFi.status() != WL_CONNECTED) {
//         vTaskDelay(pdMS_TO_TICKS(500));
//         Serial.print(".");
//     }
//     Serial.println("WiFi connected");

//     // setup mqtt
//     client.setServer(MQTT_SERVER, 1883);
//     client.setCallback(callback);

//     while(1) {
//         if (!client.connected()) {
//             reconnect();
//         }
//         client.loop(); 
//         vTaskDelay(pdMS_TO_TICKS(10)); 
//     }
// }