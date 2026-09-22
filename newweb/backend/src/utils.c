#include "web_http.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

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

static bool pair_get(const char *pairs, const char *key, char *value,
                     size_t value_size)
{
    const char *cursor = pairs != NULL ? pairs : "";
    size_t key_length = strlen(key);
    char raw[WEB_MAX_PATH];

    if (value == NULL || value_size == 0U) {
        return false;
    }
    value[0] = '\0';
    while (*cursor != '\0') {
        const char *end = strchr(cursor, '&');
        size_t length = end != NULL ? (size_t)(end - cursor) : strlen(cursor);
        if (length >= key_length + 1U && cursor[key_length] == '=' &&
            strncmp(cursor, key, key_length) == 0) {
            const char *start = cursor + key_length + 1U;
            size_t raw_length = length - key_length - 1U;
            if (raw_length >= sizeof(raw)) {
                return false;
            }
            (void)memcpy(raw, start, raw_length);
            raw[raw_length] = '\0';
            if (web_url_decode(raw, value, value_size) != 0) {
                return false;
            }
            return true;
        }
        if (end == NULL) {
            break;
        }
        cursor = end + 1;
    }
    return false;
}

bool web_query_get(const char *query, const char *key, char *value,
                   size_t value_size)
{
    return pair_get(query, key, value, value_size);
}

bool web_body_get(const char *body, const char *key, char *value,
                  size_t value_size)
{
    return pair_get(body, key, value, value_size);
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
                escape = "?";
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

bool web_write_file(const char *path, const char *data, size_t length)
{
    FILE *fp;
    size_t written;
    if (path == NULL || data == NULL) {
        return false;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        return false;
    }
    written = fwrite(data, 1U, length, fp);
    if (fclose(fp) != 0) {
        return false;
    }
    return written == length;
}

/* 逐级创建目录：Windows 的 _mkdir() 不会创建中间层级。 */
bool web_ensure_dir(const char *path)
{
    char buffer[WEB_MAX_PATH];
    char *cursor;
    size_t length;

    if (path == NULL || path[0] == '\0') {
        return false;
    }
    length = strlen(path);
    if (length >= sizeof(buffer)) {
        return false;
    }
    (void)memcpy(buffer, path, length + 1U);
    for (cursor = buffer + 1; *cursor != '\0'; cursor++) {
        if (*cursor != '/' && *cursor != '\\') {
            continue;
        }
        {
            char saved = *cursor;
            *cursor = '\0';
#if defined(_WIN32)
            (void)_mkdir(buffer);
#else
            (void)mkdir(buffer, 0755);
#endif
            *cursor = saved;
        }
    }
#if defined(_WIN32)
    (void)_mkdir(buffer);
#else
    (void)mkdir(buffer, 0755);
#endif
    return true;
}

bool web_parse_bool(const char *text, bool *value)
{
    if (text == NULL || value == NULL) {
        return false;
    }
    if (strcmp(text, "1") == 0 || strcmp(text, "true") == 0 ||
        strcmp(text, "on") == 0 || strcmp(text, "yes") == 0) {
        *value = true;
        return true;
    }
    if (strcmp(text, "0") == 0 || strcmp(text, "false") == 0 ||
        strcmp(text, "off") == 0 || strcmp(text, "no") == 0) {
        *value = false;
        return true;
    }
    return false;
}

bool web_parse_u64(const char *text, uint64_t *value)
{
    uint64_t result = 0U;
    const char *cursor = text;
    if (text == NULL || value == NULL || text[0] == '\0') {
        return false;
    }
    while (*cursor != '\0') {
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
        if (result > (UINT64_MAX - (uint64_t)(*cursor - '0')) / 10U) {
            return false;
        }
        result = result * 10U + (uint64_t)(*cursor - '0');
        cursor++;
    }
    *value = result;
    return true;
}

bool web_parse_double(const char *text, double *value)
{
    char *end = NULL;
    double parsed;
    if (text == NULL || value == NULL) {
        return false;
    }
    parsed = strtod(text, &end);
    if (end == text || end == NULL) {
        return false;
    }
    while (*end == ' ' || *end == '\t') {
        end++;
    }
    if (*end != '\0') {
        return false;
    }
    *value = parsed;
    return true;
}
