#include "publish.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern Client client;

void publish_message(const char *topic, const char *message) {
    int topic_len = strlen(topic);
    int msg_len = strlen(message);
    unsigned char pub_packet[1024];
    int pos = 0;

    pub_packet[pos++] = 0x30; // PUBLISH, QoS 0
    pub_packet[pos++] = topic_len + msg_len + 2; // Remaining length

    // Topic
    pub_packet[pos++] = (topic_len >> 8) & 0xFF;
    pub_packet[pos++] = topic_len & 0xFF;
    memcpy(&pub_packet[pos], topic, topic_len);
    pos += topic_len;

    // Payload
    memcpy(&pub_packet[pos], message, msg_len);
    pos += msg_len;

    send(client.socket_fd, pub_packet, pos, 0);
    printf("Mensaje publicado en topic '%s': %s\n", topic, message);
}

