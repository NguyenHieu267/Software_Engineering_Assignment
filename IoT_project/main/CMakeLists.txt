idf_component_register(
    SRCS
        "main.c"
        "coreIoT.c"
        "servo.c"
        "ir_sensor.c"
        "rfid.c"
        "lcd.c"
        "parking_state.c"
    INCLUDE_DIRS "."
    REQUIRES lcd rc522 servo ir_sensor esp_wifi esp_netif nvs_flash esp_event esp_http_client json
)
