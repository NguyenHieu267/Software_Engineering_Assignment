#include "tcrt5k.h"
#include "lcd.h"

bool last_state = HIGH;

void tcrt( void*pvParameters){
    pinMode(TCRT_PIN, INPUT);

    while(1){
        bool current_state = digitalRead(TCRT_PIN);  // TCRT5000 always HIGH if no object in front of 

        // if dectect (current_state == LOW)
        if(current_state != last_state){
            if(current_state == LOW){ 
                if(empty > 0){
                    empty--;
                    xSemaphoreGive(xLCDSemaphore);
                }
            } else { 
                if(empty < total_slots){
                    empty++;
                    xSemaphoreGive(xLCDSemaphore);
                }
            }
        last_state = current_state;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}