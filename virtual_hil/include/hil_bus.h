#ifndef HIL_BUS_H
#define HIL_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hil_common.h"
#include "hil_frame.h"
#include "hil_logger.h"

typedef struct hil_bus {
    hil_frame_t *queue;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
    uint32_t next_sequence;
    uint8_t next_alive_counter;
    uint64_t now_ms;
    unsigned int loss_probability_percent;
    uint64_t delay_ms;
    bool tamper_enabled;
    uint64_t random_state;
    uint64_t total_published;
    uint64_t total_delivered;
    uint64_t total_lost;
    uint64_t total_tampered;
} hil_bus_t;

void hil_bus_init(hil_bus_t *bus, size_t capacity, unsigned int loss_percent,
                  uint64_t delay_ms, bool tamper_enabled, uint64_t seed);
void hil_bus_deinit(hil_bus_t *bus);
void hil_bus_set_time(hil_bus_t *bus, uint64_t now_ms);
hil_status_t hil_bus_publish(hil_bus_t *bus, uint32_t id, uint8_t dlc,
                             const uint8_t *data, uint64_t timestamp_ms,
                             const hil_logger_t *logger);
hil_status_t hil_bus_poll(hil_bus_t *bus, hil_frame_t *out_frame,
                          const hil_logger_t *logger);
bool hil_bus_is_empty(const hil_bus_t *bus);
bool hil_bus_is_full(const hil_bus_t *bus);
size_t hil_bus_count(const hil_bus_t *bus);

hil_status_t hil_bus_inject_frame(hil_bus_t *bus, const hil_frame_t *frame,
                                  const hil_logger_t *logger);

#endif
