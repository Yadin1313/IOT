#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCK_PATH "mysocket.server"
#define TIMES 5

int main(int argc, char * argv[]) {
    const char * msg = "Hola desde Server\n\0";

    int socket_connection = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un server;
    memset(&server, 0, sizeof(server));

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCK_PATH);

    int ret = bind(socket_connection, (struct sockaddr *)&server, sizeof(server));
    ret = listen(socket_connection, 10);

    for(int i = 0; i < TIMES; i++) {
        int sock = accept(socket_connection, (struct sockaddr *)NULL, NULL);
        ret = send(sock, (void *)msg, strlen(msg), 0);
        ret = close(sock);
    }

    ret = close(socket_connection);
    return 0;
}

