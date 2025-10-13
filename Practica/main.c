#include "client.h"
#include "publish.h"

#define BROKER_IP "192.168.1.1"
#define BROKER_PORT 1883
#define TOPIC "pico_pi/test"

int main() {
    // Conectar al broker
    connect_to_broker(BROKER_IP, BROKER_PORT);

    // Suscribirse al topic
    subscribe_topic(&client, TOPIC);

    // Publicar un mensaje
    publish_message(TOPIC, "Hola desde Pico Pi!");

    // Esperar mensajes y mostrarlos en Tera Term
    handle_messages(&client);

    return 0;
}

