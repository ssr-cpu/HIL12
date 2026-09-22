#include "hil_web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "web_http.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET web_socket_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int web_socket_t;
#endif

static void close_socket(intptr_t socket)
{
#if defined(_WIN32)
    closesocket((SOCKET)socket);
#else
    close((int)socket);
#endif
}

int main(int argc, char **argv)
{
    hil_web_session_t session;
    unsigned int port = WEB_DEFAULT_PORT;
    const char *output_dir = "data/out";
    int index;

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--port") == 0 && index + 1 < argc) {
            unsigned int parsed = 0U;
            if (sscanf(argv[index + 1], "%u", &parsed) == 1 &&
                parsed > 0U && parsed < 65536U) {
                port = parsed;
            }
            index++;
        } else if (strcmp(argv[index], "--out") == 0 && index + 1 < argc) {
            output_dir = argv[index + 1];
            index++;
        }
    }

#if defined(_WIN32)
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            (void)fprintf(stderr, "WSAStartup failed\n");
            return 1;
        }
    }
#else
    (void)signal(SIGPIPE, SIG_IGN);
#endif

    hil_web_session_init(&session);
    if (hil_web_session_start(&session, output_dir) != HIL_OK) {
        (void)fprintf(stderr, "cannot start HIL session (output dir: %s)\n",
                      output_dir);
        hil_web_session_deinit(&session);
        return 1;
    }
    hil_web_api_set_session(&session);

    {
        web_socket_t server = socket(AF_INET, SOCK_STREAM, 0);
#if defined(_WIN32)
        if (server == INVALID_SOCKET) {
#else
        if (server < 0) {
#endif
            (void)fprintf(stderr, "socket failed\n");
            hil_web_session_deinit(&session);
            return 1;
        }
        {
            int enable = 1;
            (void)setsockopt(server, SOL_SOCKET, SO_REUSEADDR,
#if defined(_WIN32)
                             (const char *)&enable,
#else
                             &enable,
#endif
                             sizeof(enable));
        }
        {
            struct sockaddr_in address;
            (void)memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = htons((unsigned short)port);
            if (bind(server, (struct sockaddr *)&address,
                     sizeof(address)) != 0) {
                (void)fprintf(stderr, "bind failed on port %u\n", port);
                close_socket((intptr_t)server);
                hil_web_session_deinit(&session);
                return 1;
            }
        }
        if (listen(server, 8) != 0) {
            (void)fprintf(stderr, "listen failed\n");
            close_socket((intptr_t)server);
            hil_web_session_deinit(&session);
            return 1;
        }

        (void)printf("virtual HIL web console: http://127.0.0.1:%u\n", port);
        (void)printf("data output directory: %s\n", session.config.output_dir);
        (void)printf("Press Ctrl+C to stop.\n");
        (void)fflush(stdout);

        for (;;) {
            web_socket_t client = accept(server, NULL, NULL);
#if defined(_WIN32)
            if (client == INVALID_SOCKET) {
#else
            if (client < 0) {
#endif
                continue;
            }
            web_handle_client((intptr_t)client);
            close_socket((intptr_t)client);
        }
    }

    hil_web_session_deinit(&session);
    return 0;
}
