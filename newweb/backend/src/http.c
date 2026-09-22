#include "web_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET web_socket_t;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int web_socket_t;
#endif

static int send_all(intptr_t raw_socket, const char *data, size_t length)
{
    web_socket_t socket = (web_socket_t)raw_socket;
    size_t sent = 0U;
    while (sent < length) {
        int result =
#if defined(_WIN32)
            send(socket, data + sent, (int)(length - sent), 0);
#else
            (int)send(socket, data + sent, length - sent, 0);
#endif
        if (result <= 0) {
            return -1;
        }
        sent += (size_t)result;
    }
    return 0;
}

static int recv_chunk(intptr_t raw_socket, char *buffer, size_t length)
{
    web_socket_t socket = (web_socket_t)raw_socket;
    size_t got = 0U;
    while (got < length) {
        int result =
#if defined(_WIN32)
            recv(socket, buffer + got, (int)(length - got), 0);
#else
            (int)recv(socket, buffer + got, length - got, 0);
#endif
        if (result <= 0) {
            return -1;
        }
        got += (size_t)result;
    }
    return 0;
}

/* 先按字节读到头部结束，再按 Content-Length 读取 POST 正文。 */
static int receive_request(intptr_t raw_socket, char *buffer, size_t size,
                           size_t *body_offset)
{
    size_t used = 0U;
    const char *length_header;
    long content_length = 0L;

    *body_offset = 0U;
    while (used + 1U < size) {
        if (recv_chunk(raw_socket, buffer + used, 1U) != 0) {
            return -1;
        }
        used++;
        if (used >= 4U && memcmp(buffer + used - 4U, "\r\n\r\n", 4U) == 0) {
            buffer[used] = '\0';
            break;
        }
    }
    if (used + 1U >= size) {
        return -1;
    }
    *body_offset = used;

    length_header = strstr(buffer, "Content-Length:");
    if (length_header == NULL) {
        length_header = strstr(buffer, "Content-length:");
    }
    if (length_header == NULL) {
        length_header = strstr(buffer, "content-length:");
    }
    if (length_header != NULL) {
        if (sscanf(length_header + 15, " %ld", &content_length) != 1) {
            content_length = 0L;
        }
    }
    if (content_length > 0L) {
        size_t need = (size_t)content_length;
        if (need > WEB_MAX_BODY) {
            need = WEB_MAX_BODY;
        }
        if (used + need + 1U >= size) {
            return -1;
        }
        if (recv_chunk(raw_socket, buffer + used, need) != 0) {
            return -1;
        }
        used += need;
        buffer[used] = '\0';
    }
    return 0;
}

static int parse_request(const char *raw, size_t body_offset,
                         web_request_t *request)
{
    const char *space1;
    const char *space2;
    char *question;
    size_t path_length;

    if (raw == NULL || request == NULL) {
        return -1;
    }
    space1 = strchr(raw, ' ');
    if (space1 == NULL) {
        return -1;
    }
    space2 = strchr(space1 + 1, ' ');
    if (space2 == NULL) {
        return -1;
    }
    if ((size_t)(space1 - raw) >= sizeof(request->method)) {
        return -1;
    }
    (void)memcpy(request->method, raw, (size_t)(space1 - raw));
    request->method[space1 - raw] = '\0';

    path_length = (size_t)(space2 - (space1 + 1));
    if (path_length >= sizeof(request->path)) {
        return -1;
    }
    (void)memcpy(request->path, space1 + 1, path_length);
    request->path[path_length] = '\0';

    request->query[0] = '\0';
    question = strchr(request->path, '?');
    if (question != NULL) {
        size_t query_length = strlen(question + 1);
        if (query_length >= sizeof(request->query)) {
            return -1;
        }
        (void)strncpy(request->query, question + 1,
                      sizeof(request->query) - 1U);
        *question = '\0';
    }

    request->body[0] = '\0';
    request->body_length = 0U;
    if (body_offset > 0U && strlen(raw) > body_offset) {
        size_t copy = strlen(raw + body_offset);
        if (copy >= sizeof(request->body)) {
            copy = sizeof(request->body) - 1U;
        }
        (void)memcpy(request->body, raw + body_offset, copy);
        request->body[copy] = '\0';
        request->body_length = copy;
    }
    return 0;
}

static int send_response(intptr_t socket, int status, const char *content_type,
                         const char *body, size_t body_length)
{
    char header[1024];
    int written;
    written = snprintf(header, sizeof(header),
                       "HTTP/1.1 %d %s\r\n"
                       "Content-Type: %s; charset=utf-8\r\n"
                       "Content-Length: %llu\r\n"
                       "Connection: close\r\n"
                       "Cache-Control: no-store\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Headers: Content-Type\r\n"
                       "\r\n",
                       status, status == 200 ? "OK" : "Error", content_type,
                       (unsigned long long)body_length);
    if (written < 0 || (size_t)written >= sizeof(header)) {
        return -1;
    }
    if (send_all(socket, header, (size_t)written) != 0) {
        return -1;
    }
    return send_all(socket, body, body_length);
}

const char *web_content_type(const char *path)
{
    const char *dot;
    if (path == NULL) {
        return "application/octet-stream";
    }
    dot = strrchr(path, '.');
    if (dot == NULL) {
        return "application/octet-stream";
    }
    if (strcmp(dot, ".html") == 0) {
        return "text/html";
    }
    if (strcmp(dot, ".css") == 0) {
        return "text/css";
    }
    if (strcmp(dot, ".js") == 0) {
        return "application/javascript";
    }
    if (strcmp(dot, ".json") == 0) {
        return "application/json";
    }
    if (strcmp(dot, ".md") == 0) {
        return "text/markdown";
    }
    if (strcmp(dot, ".csv") == 0) {
        return "text/csv";
    }
    return "text/plain";
}

static int path_is_safe_static(const char *path)
{
    const char *cursor;
    if (path == NULL || path[0] != '/' || strstr(path, "..") != NULL) {
        return -1;
    }
    for (cursor = path; *cursor != '\0'; cursor++) {
        unsigned char ch = (unsigned char)*cursor;
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '/' || ch == '.' ||
              ch == '_' || ch == '-')) {
            return -1;
        }
    }
    return 0;
}

static int serve_static(intptr_t socket, const char *request_path)
{
    char decoded[WEB_MAX_PATH];
    char file_path[WEB_MAX_PATH];
    char *body;
    long body_length;
    const char *content_type;

    if (web_url_decode(request_path, decoded, sizeof(decoded)) != 0) {
        return -1;
    }
    if (strcmp(decoded, "/") == 0) {
        (void)strncpy(decoded, "/index.html", sizeof(decoded) - 1U);
    }
    if (path_is_safe_static(decoded) != 0) {
        return -1;
    }
    if (snprintf(file_path, sizeof(file_path), "frontend%s", decoded) < 0) {
        return -1;
    }
    body = (char *)malloc(WEB_MAX_RESPONSE);
    if (body == NULL) {
        return -1;
    }
    body_length = web_read_file(file_path, body, WEB_MAX_RESPONSE);
    if (body_length < 0L) {
        free(body);
        return -1;
    }
    content_type = web_content_type(file_path);
    if (send_response(socket, 200, content_type, body,
                      (size_t)body_length) != 0) {
        free(body);
        return -1;
    }
    free(body);
    return 0;
}

static int send_api(intptr_t socket, const web_request_t *request)
{
    char *body = (char *)malloc(WEB_MAX_RESPONSE);
    int status;
    if (body == NULL) {
        return -1;
    }
    status = web_api_dispatch(request, body, WEB_MAX_RESPONSE);
    if (status < 0) {
        status = 500;
        (void)strncpy(body, "{\"ok\":false,\"error\":\"internal error\"}",
                      WEB_MAX_RESPONSE - 1U);
    }
    if (send_response(socket, status, "application/json", body,
                      strlen(body)) != 0) {
        free(body);
        return -1;
    }
    free(body);
    return 0;
}

void web_handle_client(intptr_t client_socket)
{
    char *raw = (char *)malloc(WEB_MAX_REQUEST);
    web_request_t request;
    size_t body_offset = 0U;

    if (raw == NULL) {
        return;
    }
    if (receive_request(client_socket, raw, WEB_MAX_REQUEST, &body_offset) !=
        0) {
        free(raw);
        return;
    }
    if (parse_request(raw, body_offset, &request) != 0) {
        (void)send_response(client_socket, 400, "text/plain", "bad request",
                            11U);
        free(raw);
        return;
    }
    if (strncmp(request.path, "/api/", 5U) == 0) {
        (void)send_api(client_socket, &request);
    } else if (strcmp(request.method, "GET") == 0) {
        if (serve_static(client_socket, request.path) != 0) {
            (void)send_response(client_socket, 404, "text/plain", "not found",
                                9U);
        }
    } else {
        (void)send_response(client_socket, 405, "text/plain",
                            "method not allowed", 18U);
    }
    free(raw);
}
