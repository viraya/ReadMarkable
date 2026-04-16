#ifndef XOVI_TRIGGER_GESTURE_H
#define XOVI_TRIGGER_GESTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GESTURE_WINDOW_MS 2000
#define GESTURE_TAP_COUNT 3

typedef struct {
    uint64_t ts_ms[GESTURE_TAP_COUNT];
    int count;
    int fired;
} gesture_t;

void gesture_reset(gesture_t *g);
int gesture_record(gesture_t *g, uint64_t now_ms);
int gesture_has_fired(const gesture_t *g);
void gesture_disarm(gesture_t *g);

#ifdef __cplusplus
}
#endif

#endif
