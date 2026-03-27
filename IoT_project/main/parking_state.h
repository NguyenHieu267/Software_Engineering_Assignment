#ifndef PARKING_STATE_H
#define PARKING_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_EVENT_RFID = 1,
    APP_EVENT_IR_CHANGE,
    APP_EVENT_ENTRY_COUNTED,
} app_event_type_t;

typedef struct {
    app_event_type_t type;
    char uid[32];
    int total_spaces;
    int empty_spaces;
    int occupied_spaces;
    bool ir_occupied;
    uint64_t timestamp_ms;
} app_event_t;

typedef struct {
    int total_spaces;
    int empty_spaces;
    int occupied_spaces;
    bool ir_occupied;
    char last_uid[32];
} parking_snapshot_t;

void parking_state_init(int total_spaces);
void parking_state_on_rfid(const char *uid, bool *should_open_gate, app_event_t *event_out);
void parking_state_on_ir_change(bool ir_occupied, app_event_t *event_out);
void parking_state_get_snapshot(parking_snapshot_t *snapshot_out);

#ifdef __cplusplus
}
#endif

#endif
