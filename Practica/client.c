#include "client.h"
#include "publish.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

Client client; // Cliente global

// Conectarse al broker
int connect_to_broker(const char *ip, int port) {
    struct sockaddr_in server_addr;

    client.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client.socket_fd < 0) {
        perror("socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(client.socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return -1;
    }

    printf("Conectado al broker %s:%d\n", ip, port);

    // Enviar CONNECT packet MQTT básico
    unsigned char connect_packet[50];
    int pos = 0;
    connect_packet[pos++] = 0x10; // Fixed header CONNECT
    connect_packet[pos++] = 16;   // Remaining length

    // Protocol Name "MQTT"
    connect_packet[pos++] = 0x00; connect_packet[pos++] = 0x04;
    connect_packet[pos++] = 'M'; connect_packet[pos++] = 'Q';
    connect_packet[pos++] = 'T'; connect_packet[pos++] = 'T';

    connect_packet[pos++] = 0x04; // Protocol level
    connect_packet[pos++] = 0x02; // Connect flags: Clean session
    connect_packet[pos++] = 0x00; connect_packet[pos++] = 60; // Keep alive

    // Payload: Client ID "PicoClient"
    const char *client_id = "PicoClient";
    int id_len = strlen(client_id);
    connect_packet[pos++] = (id_len >> 8) & 0xFF;
    connect_packet[pos++] = id_len & 0xFF;
    memcpy(&connect_packet[pos], client_id, id_len);
    pos += id_len;

    send(client.socket_fd, connect_packet, pos, 0);
    printf("Paquete CONNECT enviado\n");

    // Recibir CONNACK
    unsigned char response[4];
    int n = recv(client.socket_fd, response, 4, 0);
    if (n > 0) {
        printf("Recibido CONNACK: ");
        for (int i = 0; i < n; i++) printf("%02X ", response[i]);
        printf("\n");
    }

    return client.socket_fd;
}

// Suscribirse a un topic
void subscribe_topic(Client *client, const char *topic) {
    strncpy(client->topics[0], topic, 50);

    // Construir SUBSCRIBE packet básico
    unsigned char sub_packet[100];
    int pos = 0;
    sub_packet[pos++] = 0x82; // SUBSCRIBE
    int topic_len = strlen(topic);
    sub_packet[pos++] = 2 + 2 + topic_len + 1; // Remaining length

    // Packet ID
    sub_packet[pos++] = 0x00;
    sub_packet[pos++] = 0x01;

    // Topic
    sub_packet[pos++] = (topic_len >> 8) & 0xFF;
    sub_packet[pos++] = topic_len & 0xFF;
    memcpy(&sub_packet[pos], topic, topic_len);
    pos += topic_len;

    sub_packet[pos++] = 0x00; // QoS 0

    send(client->socket_fd, sub_packet, pos, 0);
    printf("Suscrito al topic: %s\n", topic);
}

// Leer mensajes del broker
void handle_messages(Client *client) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int n = recv(client->socket_fd, buffer, BUFFER_SIZE-1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        printf("Mensaje recibido: %s\n", buffer);
    }
}

