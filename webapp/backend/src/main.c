#include "web_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void)
{
#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        (void)fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#else
    (void)signal(SIGPIPE, SIG_IGN);
#endif

    web_socket_t server = socket(AF_INET, SOCK_STREAM, 0);
#if defined(_WIN32)
    if (server == INVALID_SOCKET) {
#else
    if (server < 0) {
#endif
        (void)fprintf(stderr, "socket failed\n");
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
        address.sin_port = htons(WEB_DEFAULT_PORT);
        if (bind(server, (struct sockaddr *)&address, sizeof(address)) != 0) {
            (void)fprintf(stderr, "bind failed on port %d\n",
                          WEB_DEFAULT_PORT);
#if defined(_WIN32)
            closesocket(server);
            WSACleanup();
#else
            close(server);
#endif
            return 1;
        }
    }

    if (listen(server, 8) != 0) {
        (void)fprintf(stderr, "listen failed\n");
        close_socket((intptr_t)server);
        return 1;
    }

    (void)printf("virtual_hil web server running at http://127.0.0.1:%d\n",
                 WEB_DEFAULT_PORT);
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

#if defined(_WIN32)
    closesocket(server);
    WSACleanup();
#else
    close(server);
#endif
    return 0;
}
