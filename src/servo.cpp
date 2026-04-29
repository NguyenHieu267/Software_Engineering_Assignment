#include "servo.h"

Servo servo1;

void rotateForward(){
    for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
    servo1.write(posDegrees);
  }
}

void rotateBackward(){
    for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
    servo1.write(posDegrees);
  }
}

void servo( void*pvParameters){
    servo1.attach(SERVO_PIN);

    servo1.write(0);

    while(1){
        if(xSemaphoreTake(xGateSemaphore, portMAX_DELAY) == pdTRUE ){
            rotateForward();
            vTaskDelay(pdMS_TO_TICKS(5000));
            rotateBackward();
        }
    }
}