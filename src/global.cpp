#include "global.h"

SemaphoreHandle_t xGateSemaphore = xSemaphoreCreateBinary();
SemaphoreHandle_t xLCDSemaphore = xSemaphoreCreateBinary();

int total_slots = 50;
int empty = 50;

String UID = "";

// WiFiClient espClient;
// PubSubClient client(espClient);