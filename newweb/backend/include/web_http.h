#ifndef WEB_HTTP_H
#define WEB_HTTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WEB_MAX_REQUEST 65536
#define WEB_MAX_PATH 1024
#define WEB_MAX_QUERY 4096
#define WEB_MAX_BODY 32768
#define WEB_MAX_RESPONSE 65536
#define WEB_DEFAULT_PORT 8090

typedef struct web_request {
    char method[16];
    char path[WEB_MAX_PATH];
    char query[WEB_MAX_QUERY];
    char body[WEB_MAX_BODY];
    size_t body_length;
} web_request_t;

void web_handle_client(intptr_t client_socket);
const char *web_content_type(const char *path);
int web_api_dispatch(const web_request_t *request, char *body,
                     size_t body_size);
int web_url_decode(const char *input, char *output, size_t output_size);
bool web_query_get(const char *query, const char *key, char *value,
                   size_t value_size);
bool web_body_get(const char *body, const char *key, char *value,
                  size_t value_size);
void web_json_escape(const char *text, char *output, size_t output_size);
long web_read_file(const char *path, char *buffer, size_t buffer_size);
bool web_write_file(const char *path, const char *data, size_t length);
bool web_ensure_dir(const char *path);
bool web_parse_bool(const char *text, bool *value);
bool web_parse_u64(const char *text, uint64_t *value);
bool web_parse_double(const char *text, double *value);

#endif
