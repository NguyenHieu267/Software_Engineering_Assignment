# IoT Smart Parking Project

This project is split into three layers:

- `components/`: peripheral libraries (`lcd`, `rc522`, `servo`, `ir_sensor`)
- `main/`: FreeRTOS tasks and parking business logic
- `website/`: Node.js + SQLite dashboard and API

## Runtime flow

1. RFID reads UID (`main/rfid.c`).
2. Servo opens gate to 90 deg, waits 5 seconds, closes (`main/servo.c`).
3. RFID event is sent to CoreIoT queue (`main/coreIoT.c`) and posted to website API.
4. IR sensor edge confirms vehicle entry and decrements empty spaces (`main/ir_sensor.c`, `main/parking_state.c`).
5. LCD shows `Empty/Total` and last UID (`main/lcd.c`).

## Build firmware (ESP-IDF)

```bash
cd IoT_project
idf.py build
idf.py -p <PORT> flash monitor
```

## Run website

```bash
cd IoT_project/website
npm install
npm start
```

Open `http://localhost:3000`.
