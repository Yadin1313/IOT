#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 1883
#define MAX_CLIENTS 5
#define MAX_TOPIC 3
#define BUFFER_SIZE 1024

typedef struct {
    int socket_fd;
    char topics[MAX_TOPIC][50];
    int active;
} Client;

Client clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message(int client_fd, const char *msg) {
    write(client_fd, msg, strlen(msg));
    write(client_fd, "\n", 1);
}

void publish_message(const char *topic, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            for (int j = 0; j < MAX_TOPIC; j++) {
                if (strcmp(clients[i].topics[j], topic) == 0) {
                    send_message(clients[i].socket_fd, message);
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    printf("Published on topic %s: %s\n", topic, message);
}

void subscribe_topic(Client *client, const char *topic) {
    for (int i = 0; i < MAX_TOPIC; i++) {
        if (strlen(client->topics[i]) == 0) {
            strncpy(client->topics[i], topic, 50);
            printf("Client subscribed to %s\n", topic);
            break;
        }
    }
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];

    send_message(client_fd, "ConnAck: Accepted"); // Simula ConnAck

    while (1) {
        int n = read(client_fd, buffer, BUFFER_SIZE-1);
        if (n <= 0) break;
        buffer[n] = '\0';

        if (strncmp(buffer, "PING", 4) == 0) {
            send_message(client_fd, "PONG");
        } else if (strncmp(buffer, "SUB ", 4) == 0) {
            char topic[50];
            sscanf(buffer + 4, "%s", topic);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket_fd == client_fd) {
                    subscribe_topic(&clients[i], topic);
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        } else if (strncmp(buffer, "PUB ", 4) == 0) {
            char topic[50], message[BUFFER_SIZE];
            sscanf(buffer + 4, "%s %[^\n]", topic, message);
            publish_message(topic, message);
        }
    }

    // Cerrar cliente
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd == client_fd) {
            clients[i].active = 0;
            close(clients[i].socket_fd);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("Client disconnected\n");
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    memset(clients, 0, sizeof(clients));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("MQTT Test Server running on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                slot = i;
                clients[i].socket_fd = new_socket;
                clients[i].active = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (slot != -1) {
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, &new_socket);
            pthread_detach(tid);
        } else {
            send_message(new_socket, "Server full");
            close(new_socket);
        }
    }

    close(server_fd);
    return 0;
}
