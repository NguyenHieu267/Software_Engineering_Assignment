# Smart Parking Website + Database

## Run

```bash
cd website
npm install
npm start
```

Server will run on `http://localhost:3000`.

## APIs

- `GET /api/state` -> current parking state
- `GET /api/events?limit=50` -> latest events
- `POST /api/events` -> ingest event from ESP32 CoreIoT task

## ESP32 integration

In `main/coreIoT.c`, update:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `API_ENDPOINT` (example: `http://<pc-ip>:3000/api/events`)

Use your PC LAN IP, not localhost, for ESP32 HTTP posting.
