#ifndef __RC522_H__
#define __RC522_H__
#include "global.h"

void rfid_reader( void*pvParameters);

#define SS_PIN 5 
#define SCK_PIN 18
#define MOSI_PIN 23
#define MISO_PIN 19
#define RST_PIN 27

// 922B6D06 -> keychain
// 71DD385D -> card

#endif