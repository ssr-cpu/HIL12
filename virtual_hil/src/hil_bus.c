#include "hil_bus.h"

#include <stdlib.h>
#include <string.h>

static uint64_t bus_random_next(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x << 13U;
    x ^= x >> 7U;
    x ^= x << 17U;
    *state = x;
    return x;
}

static bool should_occur(unsigned int percent, uint64_t random_value)
{
    return (random_value % 100U) < (uint64_t)percent;
}

void hil_bus_init(hil_bus_t *bus, size_t capacity, unsigned int loss_percent,
                  uint64_t delay_ms, bool tamper_enabled, uint64_t seed)
{
    if (bus == NULL) {
        return;
    }
    (void)memset(bus, 0, sizeof(*bus));
    if (capacity == 0U) {
        capacity = 1U;
    }
    bus->queue = (hil_frame_t *)calloc(capacity, sizeof(*bus->queue));
    if (bus->queue == NULL) {
        bus->capacity = 0U;
        return;
    }
    bus->capacity = capacity;
    bus->loss_probability_percent = loss_percent;
    bus->delay_ms = delay_ms;
    bus->tamper_enabled = tamper_enabled;
    bus->random_state = seed == 0U ? 1U : seed;
    bus->next_sequence = 1U;
    bus->next_alive_counter = 0U;
}

void hil_bus_deinit(hil_bus_t *bus)
{
    if (bus == NULL) {
        return;
    }
    free(bus->queue);
    bus->queue = NULL;
    bus->capacity = 0U;
    bus->count = 0U;
}

void hil_bus_set_time(hil_bus_t *bus, uint64_t now_ms)
{
    if (bus != NULL) {
        bus->now_ms = now_ms;
    }
}

static hil_status_t bus_push(hil_bus_t *bus, const hil_frame_t *frame,
                             const hil_logger_t *logger)
{
    if (bus->queue == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "bus",
                        "bus queue is not initialized");
        return HIL_ERR_RESOURCE;
    }
    if (bus->count >= bus->capacity) {
        hil_log_message(logger, HIL_LOG_WARN, "bus",
                        "frame queue is full (%llu/%llu)",
                        (unsigned long long)bus->count,
                        (unsigned long long)bus->capacity);
        return HIL_ERR_BUSY;
    }
    bus->queue[bus->tail] = *frame;
    bus->tail = (bus->tail + 1U) % bus->capacity;
    bus->count++;
    return HIL_OK;
}

static hil_frame_t bus_front(const hil_bus_t *bus)
{
    return bus->queue[bus->head];
}

static void bus_pop(hil_bus_t *bus)
{
    bus->head = (bus->head + 1U) % bus->capacity;
    bus->count--;
}

hil_status_t hil_bus_publish(hil_bus_t *bus, uint32_t id, uint8_t dlc,
                             const uint8_t *data, uint64_t timestamp_ms,
                             const hil_logger_t *logger)
{
    hil_frame_t frame;
    hil_status_t status;
    if (bus == NULL || data == NULL) {
        return HIL_ERR_NULL;
    }
    if (dlc > HIL_FRAME_DATA_SIZE) {
        return HIL_ERR_RANGE;
    }
    hil_frame_init(&frame, id, dlc, timestamp_ms, bus->next_sequence,
                   bus->next_alive_counter);
    (void)memcpy(frame.data, data, dlc);
    hil_frame_refresh_crc(&frame);
    status = bus_push(bus, &frame, logger);
    if (status == HIL_OK) {
        bus->next_sequence++;
        bus->next_alive_counter++;
        bus->total_published++;
    }
    return status;
}

hil_status_t hil_bus_inject_frame(hil_bus_t *bus, const hil_frame_t *frame,
                                  const hil_logger_t *logger)
{
    hil_frame_t copy;
    hil_status_t status;
    if (bus == NULL || frame == NULL) {
        return HIL_ERR_NULL;
    }
    copy = *frame;
    hil_frame_refresh_crc(&copy);
    status = bus_push(bus, &copy, logger);
    if (status == HIL_OK) {
        bus->total_published++;
    }
    return status;
}

hil_status_t hil_bus_poll(hil_bus_t *bus, hil_frame_t *out_frame,
                          const hil_logger_t *logger)
{
    hil_frame_t frame;
    uint64_t random_value;
    if (bus == NULL || out_frame == NULL) {
        return HIL_ERR_NULL;
    }
    if (bus->count == 0U) {
        return HIL_ERR_NOT_FOUND;
    }
    frame = bus_front(bus);
    if (bus->now_ms < frame.timestamp_ms + bus->delay_ms) {
        return HIL_ERR_TIMEOUT;
    }
    bus_pop(bus);
    random_value = bus_random_next(&bus->random_state);
    if (should_occur(bus->loss_probability_percent, random_value)) {
        bus->total_lost++;
        hil_log_message(logger, HIL_LOG_WARN, "bus",
                        "frame id=%u seq=%u dropped by injected loss",
                        frame.id, frame.sequence);
        return HIL_ERR_FAULT;
    }
    if (bus->tamper_enabled) {
        random_value = bus_random_next(&bus->random_state);
        if ((random_value & 1U) != 0U && frame.dlc > 0U) {
            size_t byte_index = (size_t)(random_value % frame.dlc);
            uint8_t bit = (uint8_t)(1U << (random_value % 8U));
            frame.data[byte_index] ^= bit;
            frame.crc = 0U;
            bus->total_tampered++;
            hil_log_message(logger, HIL_LOG_WARN, "bus",
                            "frame id=%u seq=%u tampered", frame.id,
                            frame.sequence);
        }
    }
    bus->total_delivered++;
    *out_frame = frame;
    return HIL_OK;
}

bool hil_bus_is_empty(const hil_bus_t *bus)
{
    return bus == NULL || bus->count == 0U;
}

bool hil_bus_is_full(const hil_bus_t *bus)
{
    return bus != NULL && bus->count >= bus->capacity;
}

size_t hil_bus_count(const hil_bus_t *bus)
{
    return bus != NULL ? bus->count : 0U;
}
