#include "lcd.h"

LiquidCrystal_I2C lcd(0x27,16,2);   // Using LCD 16x2 and I2C add: 0x27

void lcd_init(){
    Wire.begin(21, 22);     // SDA = 21, SCL = 22
    lcd.begin();
}

void lcd_display(){
    lcd.setCursor(0,0);    // Start at col 0 - row 0
    lcd.print("Empty:");
    lcd.print(empty);
    lcd.print("/");
    lcd.print(total_slots);

    lcd.setCursor(0,1);    // Start at col 0 - row 1
    lcd.print("UID: ");
    lcd.print(UID);       
}
void lcd_task(void *pvParameters){
    lcd_init();
    lcd_display();

    while(1){
        if(xSemaphoreTake(xLCDSemaphore, portMAX_DELAY) == pdTRUE)
        {
            lcd_display();
        }
    }
}