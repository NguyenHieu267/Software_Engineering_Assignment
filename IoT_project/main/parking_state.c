#include "parking_state.h"

#include <string.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define RFID_ARM_WINDOW_MS 10000

typedef struct {
    int total_spaces;
    int empty_spaces;
    int occupied_spaces;
    bool ir_occupied;
    char last_uid[32];
    bool entry_armed;
    uint64_t arm_expire_at_ms;
    SemaphoreHandle_t lock;
} parking_state_ctx_t;

static parking_state_ctx_t s_ctx;

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void fill_event(app_event_t *evt, app_event_type_t type)
{
    if (!evt) {
        return;
    }

    memset(evt, 0, sizeof(*evt));
    evt->type = type;
    evt->total_spaces = s_ctx.total_spaces;
    evt->empty_spaces = s_ctx.empty_spaces;
    evt->occupied_spaces = s_ctx.occupied_spaces;
    evt->ir_occupied = s_ctx.ir_occupied;
    evt->timestamp_ms = now_ms();
    strncpy(evt->uid, s_ctx.last_uid, sizeof(evt->uid) - 1);
}

void parking_state_init(int total_spaces)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.total_spaces = total_spaces;
    s_ctx.empty_spaces = total_spaces;
    s_ctx.lock = xSemaphoreCreateMutex();
}

void parking_state_on_rfid(const char *uid, bool *should_open_gate, app_event_t *event_out)
{
    if (should_open_gate) {
        *should_open_gate = false;
    }

    if (!s_ctx.lock) {
        return;
    }

    xSemaphoreTake(s_ctx.lock, portMAX_DELAY);

    if (uid) {
        strncpy(s_ctx.last_uid, uid, sizeof(s_ctx.last_uid) - 1);
        s_ctx.last_uid[sizeof(s_ctx.last_uid) - 1] = '\0';
    }

    s_ctx.entry_armed = true;
    s_ctx.arm_expire_at_ms = now_ms() + RFID_ARM_WINDOW_MS;

    if (should_open_gate) {
        *should_open_gate = true;
    }

    fill_event(event_out, APP_EVENT_RFID);

    xSemaphoreGive(s_ctx.lock);
}

void parking_state_on_ir_change(bool ir_occupied, app_event_t *event_out)
{
    if (!s_ctx.lock) {
        return;
    }

    xSemaphoreTake(s_ctx.lock, portMAX_DELAY);

    bool changed = (s_ctx.ir_occupied != ir_occupied);
    s_ctx.ir_occupied = ir_occupied;

    uint64_t ts = now_ms();
    if (s_ctx.entry_armed && ts > s_ctx.arm_expire_at_ms) {
        s_ctx.entry_armed = false;
    }

    app_event_type_t evt_type = APP_EVENT_IR_CHANGE;

    if (changed && ir_occupied && s_ctx.entry_armed) {
        if (s_ctx.empty_spaces > 0) {
            s_ctx.empty_spaces--;
            s_ctx.occupied_spaces++;
        }
        s_ctx.entry_armed = false;
        evt_type = APP_EVENT_ENTRY_COUNTED;
    }

    fill_event(event_out, evt_type);
    xSemaphoreGive(s_ctx.lock);
}

void parking_state_get_snapshot(parking_snapshot_t *snapshot_out)
{
    if (!snapshot_out || !s_ctx.lock) {
        return;
    }

    xSemaphoreTake(s_ctx.lock, portMAX_DELAY);
    snapshot_out->total_spaces = s_ctx.total_spaces;
    snapshot_out->empty_spaces = s_ctx.empty_spaces;
    snapshot_out->occupied_spaces = s_ctx.occupied_spaces;
    snapshot_out->ir_occupied = s_ctx.ir_occupied;
    strncpy(snapshot_out->last_uid, s_ctx.last_uid, sizeof(snapshot_out->last_uid) - 1);
    snapshot_out->last_uid[sizeof(snapshot_out->last_uid) - 1] = '\0';
    xSemaphoreGive(s_ctx.lock);
}
