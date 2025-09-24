#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCK_PATH "mysocket.server"
#define TIMES 5
#define SIZE 100

int main(int argc, char * argv[]) {
    char buff[100];

    int socket_connection = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un server;
    memset(&server, 0, sizeof(server));

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCK_PATH);

    int ret = connect(socket_connection, (struct sockaddr *)&server, sizeof(server));

    ret = recv(socket_connection, (void *)buff, SIZE, 0);
    printf("%s", buff);

    ret = close(socket_connection);
    return 0;
}

