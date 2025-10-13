#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define MAX_TOPIC 3
#define BUFFER_SIZE 1024

typedef struct {
    int socket_fd;
    char topics[MAX_TOPIC][50];
} Client;

// Funciones del driver
int connect_to_broker(const char *ip, int port);
void subscribe_topic(Client *client, const char *topic);
void handle_messages(Client *client);

#endif

