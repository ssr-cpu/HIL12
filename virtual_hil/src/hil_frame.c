#include "hil_frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hil_frame_init(hil_frame_t *frame, uint32_t id, uint8_t dlc,
                    uint64_t timestamp_ms, uint32_t sequence,
                    uint8_t alive_counter)
{
    if (frame == NULL) {
        return;
    }
    (void)memset(frame, 0, sizeof(*frame));
    frame->id = id;
    frame->dlc = dlc > HIL_FRAME_DATA_SIZE ? HIL_FRAME_DATA_SIZE : dlc;
    frame->timestamp_ms = timestamp_ms;
    frame->sequence = sequence;
    frame->alive_counter = alive_counter;
    hil_frame_refresh_crc(frame);
}

uint8_t hil_frame_calculate_crc(const hil_frame_t *frame)
{
    uint8_t crc = 0xA5U;
    size_t index;
    if (frame == NULL) {
        return 0U;
    }
    crc ^= (uint8_t)(frame->id & 0xFFU);
    crc ^= (uint8_t)((frame->id >> 8U) & 0xFFU);
    crc ^= (uint8_t)((frame->id >> 16U) & 0xFFU);
    crc ^= (uint8_t)((frame->id >> 24U) & 0xFFU);
    crc ^= frame->dlc;
    crc ^= (uint8_t)(frame->timestamp_ms & 0xFFU);
    crc ^= (uint8_t)((frame->timestamp_ms >> 8U) & 0xFFU);
    crc ^= (uint8_t)((frame->timestamp_ms >> 16U) & 0xFFU);
    crc ^= (uint8_t)((frame->timestamp_ms >> 24U) & 0xFFU);
    crc ^= (uint8_t)(frame->sequence & 0xFFU);
    crc ^= (uint8_t)((frame->sequence >> 8U) & 0xFFU);
    crc ^= (uint8_t)((frame->sequence >> 16U) & 0xFFU);
    crc ^= (uint8_t)((frame->sequence >> 24U) & 0xFFU);
    crc ^= frame->alive_counter;
    for (index = 0U; index < frame->dlc; index++) {
        crc ^= frame->data[index];
    }
    return crc;
}

void hil_frame_refresh_crc(hil_frame_t *frame)
{
    if (frame != NULL) {
        frame->crc = hil_frame_calculate_crc(frame);
    }
}

bool hil_frame_is_crc_valid(const hil_frame_t *frame)
{
    return frame != NULL && frame->crc == hil_frame_calculate_crc(frame);
}

hil_status_t hil_frame_set_payload(hil_frame_t *frame, size_t offset,
                                   const uint8_t *bytes, size_t length)
{
    if (frame == NULL || bytes == NULL) {
        return HIL_ERR_NULL;
    }
    if (offset > HIL_FRAME_DATA_SIZE || length > HIL_FRAME_DATA_SIZE - offset) {
        return HIL_ERR_OVERFLOW;
    }
    (void)memcpy(frame->data + offset, bytes, length);
    if (offset + length > frame->dlc) {
        frame->dlc = (uint8_t)(offset + length);
    }
    hil_frame_refresh_crc(frame);
    return HIL_OK;
}

hil_status_t hil_frame_get_payload(const hil_frame_t *frame, size_t offset,
                                   uint8_t *bytes, size_t length)
{
    if (frame == NULL || bytes == NULL) {
        return HIL_ERR_NULL;
    }
    if (offset > frame->dlc || length > frame->dlc - offset) {
        return HIL_ERR_OVERFLOW;
    }
    (void)memcpy(bytes, frame->data + offset, length);
    return HIL_OK;
}

hil_status_t hil_frame_encode_double(hil_frame_t *frame, size_t offset,
                                     double value, double scale,
                                     double offset_value)
{
    double scaled;
    int32_t raw;
    uint8_t bytes[2];
    if (frame == NULL || scale == 0.0 || !hil_double_is_finite(value) ||
        !hil_double_is_finite(scale) || !hil_double_is_finite(offset_value)) {
        return HIL_ERR_VALUE;
    }
    scaled = (value - offset_value) / scale;
    if (scaled < (double)INT16_MIN) {
        raw = INT16_MIN;
    } else if (scaled > (double)INT16_MAX) {
        raw = INT16_MAX;
    } else {
        raw = (int32_t)scaled;
    }
    bytes[0] = (uint8_t)(raw & 0xFF);
    bytes[1] = (uint8_t)((raw >> 8) & 0xFF);
    return hil_frame_set_payload(frame, offset, bytes, sizeof(bytes));
}

hil_status_t hil_frame_decode_double(const hil_frame_t *frame, size_t offset,
                                     double scale, double offset_value,
                                     double *value)
{
    uint8_t bytes[2];
    int16_t raw;
    hil_status_t status;
    if (frame == NULL || value == NULL || scale == 0.0 ||
        !hil_double_is_finite(scale) || !hil_double_is_finite(offset_value)) {
        return HIL_ERR_VALUE;
    }
    status = hil_frame_get_payload(frame, offset, bytes, sizeof(bytes));
    if (status != HIL_OK) {
        return status;
    }
    raw = (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
    *value = (double)raw * scale + offset_value;
    return HIL_OK;
}

hil_status_t hil_frame_to_string(const hil_frame_t *frame, char *buffer,
                                 size_t buffer_size)
{
    int written;
    char hex[2U * HIL_FRAME_DATA_SIZE + 1U];
    size_t index;
    if (frame == NULL || buffer == NULL || buffer_size == 0U) {
        return HIL_ERR_NULL;
    }
    for (index = 0U; index < frame->dlc; index++) {
        (void)snprintf(hex + index * 2U, sizeof(hex) - index * 2U, "%02X",
                       frame->data[index]);
    }
    hex[sizeof(hex) - 1U] = '\0';
    written = snprintf(buffer, buffer_size,
                       "%u:%u:%llu:%u:%u:%u:%s", frame->id, frame->dlc,
                       (unsigned long long)frame->timestamp_ms,
                       frame->sequence, frame->alive_counter, frame->crc, hex);
    if (written < 0 || (size_t)written >= buffer_size) {
        return HIL_ERR_OVERFLOW;
    }
    return HIL_OK;
}

static bool parse_hex_byte(const char *text, uint8_t *value)
{
    int high;
    int low;
    if (text == NULL || value == NULL || text[0] == '\0' || text[1] == '\0') {
        return false;
    }
    high = text[0];
    low = text[1];
    if (high >= '0' && high <= '9') {
        high -= '0';
    } else if (high >= 'A' && high <= 'F') {
        high = high - 'A' + 10;
    } else if (high >= 'a' && high <= 'f') {
        high = high - 'a' + 10;
    } else {
        return false;
    }
    if (low >= '0' && low <= '9') {
        low -= '0';
    } else if (low >= 'A' && low <= 'F') {
        low = low - 'A' + 10;
    } else if (low >= 'a' && low <= 'f') {
        low = low - 'a' + 10;
    } else {
        return false;
    }
    *value = (uint8_t)((high << 4) | low);
    return true;
}

hil_status_t hil_frame_from_string(hil_frame_t *frame, const char *text)
{
    unsigned int id;
    unsigned int dlc;
    unsigned long long timestamp;
    unsigned int sequence;
    unsigned int alive;
    unsigned int crc;
    char hex[2U * HIL_FRAME_DATA_SIZE + 1U];
    size_t index;
    if (frame == NULL || text == NULL) {
        return HIL_ERR_NULL;
    }
    if (sscanf(text, "%u:%u:%llu:%u:%u:%u:%32s", &id, &dlc, &timestamp,
               &sequence, &alive, &crc, hex) != 7) {
        return HIL_ERR_FORMAT;
    }
    if (dlc > HIL_FRAME_DATA_SIZE || id > UINT32_MAX ||
        sequence > UINT32_MAX || alive > UINT8_MAX || crc > UINT8_MAX) {
        return HIL_ERR_RANGE;
    }
    if (strlen(hex) != 2U * dlc) {
        return HIL_ERR_FORMAT;
    }
    (void)memset(frame, 0, sizeof(*frame));
    frame->id = (uint32_t)id;
    frame->dlc = (uint8_t)dlc;
    frame->timestamp_ms = (uint64_t)timestamp;
    frame->sequence = (uint32_t)sequence;
    frame->alive_counter = (uint8_t)alive;
    frame->crc = (uint8_t)crc;
    for (index = 0U; index < (size_t)dlc; index++) {
        if (!parse_hex_byte(hex + index * 2U, &frame->data[index])) {
            return HIL_ERR_FORMAT;
        }
    }
    return HIL_OK;
}
