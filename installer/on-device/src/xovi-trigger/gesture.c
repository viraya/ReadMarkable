#include "gesture.h"

#include <string.h>

void gesture_reset(gesture_t *g) {
    memset(g, 0, sizeof(*g));
}

int gesture_record(gesture_t *g, uint64_t now_ms) {
    if (g->fired) {
        return 0;
    }
    if (g->count < GESTURE_TAP_COUNT) {
        g->ts_ms[g->count++] = now_ms;
    } else {
        g->ts_ms[0] = g->ts_ms[1];
        g->ts_ms[1] = g->ts_ms[2];
        g->ts_ms[2] = now_ms;
    }
    if (g->count == GESTURE_TAP_COUNT &&
        (g->ts_ms[GESTURE_TAP_COUNT - 1] - g->ts_ms[0]) <= GESTURE_WINDOW_MS) {
        g->fired = 1;
        return 1;
    }
    return 0;
}

int gesture_has_fired(const gesture_t *g) {
    return g->fired;
}

void gesture_disarm(gesture_t *g) {
    g->fired = 1;
    g->count = 0;
}
