#ifndef HIL_TIME_H
#define HIL_TIME_H

#include <stdint.h>

#include "hil_common.h"

typedef struct hil_virtual_time {
    uint64_t now_ms;
} hil_virtual_time_t;

void hil_time_init(hil_virtual_time_t *time);
uint64_t hil_time_now(const hil_virtual_time_t *time);
hil_status_t hil_time_advance(hil_virtual_time_t *time, uint64_t delta_ms);
uint64_t hil_time_elapsed(const hil_virtual_time_t *time, uint64_t since_ms);

#endif
