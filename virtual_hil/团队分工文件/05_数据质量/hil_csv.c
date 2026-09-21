#include "hil_csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool field_needs_quotes(const char *field)
{
    return field != NULL &&
           (strchr(field, ',') != NULL || strchr(field, '"') != NULL ||
            strchr(field, '\n') != NULL || strchr(field, '\r') != NULL);
}

hil_status_t hil_csv_writer_open(hil_csv_writer_t *writer, const char *path,
                                 size_t columns, const hil_logger_t *logger)
{
    if (writer == NULL || path == NULL) {
        return HIL_ERR_NULL;
    }
    writer->fp = fopen(path, "w");
    if (writer->fp == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "csv",
                        "cannot create CSV file: %s", path);
        return HIL_ERR_IO;
    }
    writer->columns = columns;
    writer->row_started = false;
    return HIL_OK;
}


static hil_status_t write_field(FILE *fp, const char *field)
{
    bool quoted = field_needs_quotes(field);
    if (quoted && fputc('"', fp) == EOF) {
        return HIL_ERR_IO;
    }
    while (field != NULL && *field != '\0') {
        if (quoted && *field == '"' && fputc('"', fp) == EOF) {
            return HIL_ERR_IO;
        }
        if (fputc((unsigned char)*field, fp) == EOF) {
            return HIL_ERR_IO;
        }
        field++;
    }
    if (quoted && fputc('"', fp) == EOF) {
        return HIL_ERR_IO;
    }
    return HIL_OK;
}

hil_status_t hil_csv_writer_write_row(hil_csv_writer_t *writer,
                                      const char *const *fields,
                                      size_t field_count,
                                      const hil_logger_t *logger)
{
    size_t index;
    if (writer == NULL || writer->fp == NULL || fields == NULL) {
        return HIL_ERR_NULL;
    }
    if (writer->columns != 0U && field_count != writer->columns) {
        hil_log_message(logger, HIL_LOG_ERROR, "csv",
                        "expected %llu columns, got %llu",
                        (unsigned long long)writer->columns,
                        (unsigned long long)field_count);
        return HIL_ERR_FORMAT;
    }
    for (index = 0U; index < field_count; index++) {
        if (index > 0U && fputc(',', writer->fp) == EOF) {
            return HIL_ERR_IO;
        }
        if (write_field(writer->fp, fields[index]) != HIL_OK ||
            ferror(writer->fp)) {
            return HIL_ERR_IO;
        }
    }
    if (fputc('\n', writer->fp) == EOF) {
        return HIL_ERR_IO;
    }
    return HIL_OK;
}

hil_status_t hil_csv_writer_close(hil_csv_writer_t *writer,
                                  const hil_logger_t *logger)
{
    (void)logger;
    if (writer == NULL) {
        return HIL_ERR_NULL;
    }
    if (writer->fp != NULL && fclose(writer->fp) != 0) {
        writer->fp = NULL;
        return HIL_ERR_IO;
    }
    writer->fp = NULL;
    return HIL_OK;
}

static void reader_free_fields(hil_csv_reader_t *reader)
{
    size_t index;
    for (index = 0U; index < reader->field_count; index++) {
        free(reader->fields[index]);
    }
    free(reader->fields);
    reader->fields = NULL;
    reader->field_count = 0U;
    reader->field_capacity = 0U;
}

hil_status_t hil_csv_reader_open(hil_csv_reader_t *reader, const char *path,
                                 const hil_logger_t *logger)
{
    if (reader == NULL || path == NULL) {
        return HIL_ERR_NULL;
    }
    (void)memset(reader, 0, sizeof(*reader));
    reader->fp = fopen(path, "r");
    if (reader->fp == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "csv",
                        "cannot open CSV file: %s", path);
        return HIL_ERR_IO;
    }
    return HIL_OK;
}

static hil_status_t reader_reserve(hil_csv_reader_t *reader, size_t required)
{
    char **new_fields;
    size_t new_capacity;
    if (required <= reader->field_capacity) {
        return HIL_OK;
    }
    new_capacity = reader->field_capacity == 0U ? 8U : reader->field_capacity;
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2U) {
            return HIL_ERR_OVERFLOW;
        }
        new_capacity *= 2U;
    }
    if (new_capacity > SIZE_MAX / sizeof(*new_fields)) {
        return HIL_ERR_OVERFLOW;
    }
    new_fields = (char **)realloc(reader->fields,
                                  new_capacity * sizeof(*new_fields));
    if (new_fields == NULL) {
        return HIL_ERR_NOMEM;
    }
    reader->fields = new_fields;
    reader->field_capacity = new_capacity;
    return HIL_OK;
}

static hil_status_t reader_add_field(hil_csv_reader_t *reader,
                                     const char *value, size_t length)
{
    char *copy;
    hil_status_t status = reader_reserve(reader, reader->field_count + 1U);
    if (status != HIL_OK) {
        return status;
    }
    if (length == SIZE_MAX || length + 1U < length) {
        return HIL_ERR_OVERFLOW;
    }
    copy = (char *)malloc(length + 1U);
    if (copy == NULL) {
        return HIL_ERR_NOMEM;
    }
    if (length > 0U) {
        (void)memcpy(copy, value, length);
    }
    copy[length] = '\0';
    reader->fields[reader->field_count++] = copy;
    return HIL_OK;
}

static bool is_blank_line(const char *line)
{
    while (*line != '\0') {
        if (*line != ' ' && *line != '\t' && *line != '\r' && *line != '\n') {
            return false;
        }
        line++;
    }
    return true;
}

hil_status_t hil_csv_reader_read_row(hil_csv_reader_t *reader,
                                     const hil_logger_t *logger)
{
    char line[HIL_LINE_MAX];
    char field[HIL_LINE_MAX];
    size_t field_length = 0U;
    size_t index = 0U;
    bool in_quotes = false;
    bool field_started = false;
    hil_status_t status;

    if (reader == NULL || reader->fp == NULL) {
        return HIL_ERR_NULL;
    }
    reader_free_fields(reader);
    do {
        if (fgets(line, (int)sizeof(line), reader->fp) == NULL) {
            return HIL_EOF;
        }
        reader->line_number++;
    } while (is_blank_line(line));

    for (index = 0U; line[index] != '\0'; index++) {
        char ch = line[index];
        if (ch == '\n' || ch == '\r') {
            break;
        }
        if (ch == '"') {
            if (in_quotes && line[index + 1U] == '"') {
                if (field_length >= sizeof(field) - 1U) {
                    reader_free_fields(reader);
                    return HIL_ERR_OVERFLOW;
                }
                field[field_length++] = '"';
                index++;
            } else {
                in_quotes = !in_quotes;
            }
            field_started = true;
            continue;
        }
        if (ch == ',' && !in_quotes) {
            status = reader_add_field(reader, field, field_length);
            if (status != HIL_OK) {
                reader_free_fields(reader);
                return status;
            }
            field_length = 0U;
            field_started = false;
            continue;
        }
        if (field_length >= sizeof(field) - 1U) {
            reader_free_fields(reader);
            return HIL_ERR_OVERFLOW;
        }
        field[field_length++] = ch;
        field_started = true;
    }
    if (in_quotes) {
        hil_log_message(logger, HIL_LOG_ERROR, "csv",
                        "line %llu has unmatched quote",
                        (unsigned long long)reader->line_number);
        reader_free_fields(reader);
        return HIL_ERR_FORMAT;
    }
    if (field_started || reader->field_count > 0U) {
        status = reader_add_field(reader, field, field_length);
        if (status != HIL_OK) {
            reader_free_fields(reader);
            return status;
        }
    }
    return HIL_OK;
}

void hil_csv_reader_close(hil_csv_reader_t *reader)
{
    if (reader == NULL) {
        return;
    }
    reader_free_fields(reader);
    if (reader->fp != NULL) {
        (void)fclose(reader->fp);
        reader->fp = NULL;
    }
}

const char *hil_csv_reader_field(const hil_csv_reader_t *reader, size_t index)
{
    if (reader == NULL || index >= reader->field_count) {
        return NULL;
    }
    return reader->fields[index];
}

size_t hil_csv_reader_field_count(const hil_csv_reader_t *reader)
{
    return reader != NULL ? reader->field_count : 0U;
}

size_t hil_csv_reader_line_number(const hil_csv_reader_t *reader)
{
    return reader != NULL ? reader->line_number : 0U;
}
