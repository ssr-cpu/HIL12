#include "web_server.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int web_url_decode(const char *input, char *output, size_t output_size)
{
    size_t in_index = 0U;
    size_t out_index = 0U;
    if (input == NULL || output == NULL || output_size == 0U) {
        return -1;
    }
    while (input[in_index] != '\0') {
        unsigned char ch = (unsigned char)input[in_index];
        if (ch == '+') {
            ch = ' ';
        } else if (ch == '%') {
            char high = input[in_index + 1U];
            char low = input[in_index + 2U];
            unsigned int value;
            if (high == '\0' || low == '\0' ||
                sscanf(input + in_index + 1U, "%2x", &value) != 1) {
                return -1;
            }
            ch = (unsigned char)value;
            in_index += 2U;
        }
        if (out_index + 1U >= output_size) {
            return -1;
        }
        output[out_index++] = (char)ch;
        in_index++;
    }
    output[out_index] = '\0';
    return 0;
}

bool web_query_get(const char *query, const char *key, char *value,
                   size_t value_size)
{
    const char *cursor = query != NULL ? query : "";
    size_t key_length = strlen(key);
    if (value == NULL || value_size == 0U) {
        return false;
    }
    value[0] = '\0';
    while (*cursor != '\0') {
        const char *end = strchr(cursor, '&');
        size_t length = end != NULL ? (size_t)(end - cursor) : strlen(cursor);
        if (length >= key_length + 1U && cursor[key_length] == '=' &&
            strncmp(cursor, key, key_length) == 0) {
            const char *raw = cursor + key_length + 1U;
            size_t raw_length = length - key_length - 1U;
            if (raw_length >= WEB_MAX_PATH) {
                return false;
            }
            if (raw_length + 1U > value_size) {
                return false;
            }
            (void)memcpy(value, raw, raw_length);
            value[raw_length] = '\0';
            return web_url_decode(value, value, value_size) == 0;
        }
        if (end == NULL) {
            break;
        }
        cursor = end + 1;
    }
    return false;
}

bool web_is_safe_command_arg(const char *text)
{
    const char *cursor;
    if (text == NULL || text[0] == '\0') {
        return false;
    }
    if (strchr(text, '"') != NULL || strchr(text, '&') != NULL ||
        strchr(text, '|') != NULL || strchr(text, ';') != NULL ||
        strchr(text, '<') != NULL || strchr(text, '>') != NULL ||
        strchr(text, '\n') != NULL || strchr(text, '\r') != NULL) {
        return false;
    }
    for (cursor = text; *cursor != '\0'; cursor++) {
        if (!(isalnum((unsigned char)*cursor) || *cursor == '_' ||
              *cursor == '-' || *cursor == '.' || *cursor == '/' ||
              *cursor == '\\' || *cursor == ':')) {
            return false;
        }
    }
    return true;
}

void web_json_escape(const char *text, char *output, size_t output_size)
{
    size_t in_index = 0U;
    size_t out_index = 0U;
    if (output == NULL || output_size == 0U) {
        return;
    }
    if (text == NULL) {
        text = "";
    }
    while (text[in_index] != '\0' && out_index + 3U < output_size) {
        unsigned char ch = (unsigned char)text[in_index];
        const char *escape = NULL;
        switch (ch) {
        case '"':
            escape = "\\\"";
            break;
        case '\\':
            escape = "\\\\";
            break;
        case '\n':
            escape = "\\n";
            break;
        case '\r':
            escape = "\\r";
            break;
        case '\t':
            escape = "\\t";
            break;
        default:
            if (ch < 0x20U) {
                escape = "\\u0000";
            } else {
                output[out_index++] = (char)ch;
            }
            break;
        }
        if (escape != NULL) {
            size_t escape_length = strlen(escape);
            if (out_index + escape_length >= output_size) {
                break;
            }
            (void)memcpy(output + out_index, escape, escape_length);
            out_index += escape_length;
        }
        in_index++;
    }
    output[out_index] = '\0';
}

long web_read_file(const char *path, char *buffer, size_t buffer_size)
{
    FILE *fp;
    long size;
    size_t read_count;
    if (path == NULL || buffer == NULL || buffer_size == 0U) {
        return -1L;
    }
    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1L;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fclose(fp);
        return -1L;
    }
    size = ftell(fp);
    if (size < 0L || fseek(fp, 0L, SEEK_SET) != 0) {
        (void)fclose(fp);
        return -1L;
    }
    if ((unsigned long)size >= buffer_size) {
        (void)fclose(fp);
        return -1L;
    }
    read_count = fread(buffer, 1U, (size_t)size, fp);
    (void)fclose(fp);
    if (read_count != (size_t)size) {
        return -1L;
    }
    buffer[read_count] = '\0';
    return size;
}
