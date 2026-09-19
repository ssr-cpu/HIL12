#ifndef HIL_FRAME_H
#define HIL_FRAME_H

#include <stdint.h>

#include "hil_common.h"

#define HIL_FRAME_DATA_SIZE 8U
#define HIL_FRAME_TEXT_SIZE 96U

typedef struct hil_frame {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[HIL_FRAME_DATA_SIZE];
    uint64_t timestamp_ms;
    uint32_t sequence;
    uint8_t alive_counter;
    uint8_t crc;
} hil_frame_t;

void hil_frame_init(hil_frame_t *frame, uint32_t id, uint8_t dlc,
                    uint64_t timestamp_ms, uint32_t sequence,
                    uint8_t alive_counter);
uint8_t hil_frame_calculate_crc(const hil_frame_t *frame);
void hil_frame_refresh_crc(hil_frame_t *frame);
bool hil_frame_is_crc_valid(const hil_frame_t *frame);
hil_status_t hil_frame_set_payload(hil_frame_t *frame, size_t offset,
                                   const uint8_t *bytes, size_t length);
hil_status_t hil_frame_get_payload(const hil_frame_t *frame, size_t offset,
                                   uint8_t *bytes, size_t length);
hil_status_t hil_frame_encode_double(hil_frame_t *frame, size_t offset,
                                     double value, double scale,
                                     double offset_value);
hil_status_t hil_frame_decode_double(const hil_frame_t *frame, size_t offset,
                                     double scale, double offset_value,
                                     double *value);
hil_status_t hil_frame_to_string(const hil_frame_t *frame, char *buffer,
                                 size_t buffer_size);
hil_status_t hil_frame_from_string(hil_frame_t *frame, const char *text);

#endif
