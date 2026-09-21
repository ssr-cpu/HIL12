#include "hil_time.h"

#include <limits.h>
#include <stdint.h>

void hil_time_init(hil_virtual_time_t *time)
{
    if (time != NULL) {
        time->now_ms = 0U;
    }
}

uint64_t hil_time_now(const hil_virtual_time_t *time)
{
    return time != NULL ? time->now_ms : 0U;
}

hil_status_t hil_time_advance(hil_virtual_time_t *time, uint64_t delta_ms)
{
    if (time == NULL) {
        return HIL_ERR_NULL;
    }
    if (delta_ms > UINT64_MAX - time->now_ms) {
        return HIL_ERR_OVERFLOW;
    }
    time->now_ms += delta_ms;
    return HIL_OK;
}

uint64_t hil_time_elapsed(const hil_virtual_time_t *time, uint64_t since_ms)
{
    uint64_t now = hil_time_now(time);
    if (now < since_ms) {
        return 0U;
    }
    return now - since_ms;
}
