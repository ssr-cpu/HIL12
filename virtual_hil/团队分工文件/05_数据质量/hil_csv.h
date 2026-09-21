#ifndef HIL_CSV_H
#define HIL_CSV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "hil_common.h"
#include "hil_logger.h"

typedef struct hil_csv_writer {
    FILE *fp;
    size_t columns;
    bool row_started;
} hil_csv_writer_t;

hil_status_t hil_csv_writer_open(hil_csv_writer_t *writer, const char *path,
                                 size_t columns, const hil_logger_t *logger);
hil_status_t hil_csv_writer_write_row(hil_csv_writer_t *writer,
                                      const char *const *fields,
                                      size_t field_count,
                                      const hil_logger_t *logger);
hil_status_t hil_csv_writer_close(hil_csv_writer_t *writer,
                                  const hil_logger_t *logger);

typedef struct hil_csv_reader {
    FILE *fp;
    char **fields;
    size_t field_count;
    size_t field_capacity;
    size_t line_number;
} hil_csv_reader_t;

hil_status_t hil_csv_reader_open(hil_csv_reader_t *reader, const char *path,
                                 const hil_logger_t *logger);
hil_status_t hil_csv_reader_read_row(hil_csv_reader_t *reader,
                                     const hil_logger_t *logger);
void hil_csv_reader_close(hil_csv_reader_t *reader);
const char *hil_csv_reader_field(const hil_csv_reader_t *reader, size_t index);
size_t hil_csv_reader_field_count(const hil_csv_reader_t *reader);
size_t hil_csv_reader_line_number(const hil_csv_reader_t *reader);

#endif
