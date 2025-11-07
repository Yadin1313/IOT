# Reporte de Práctica: Proyecto Cliente MQTT

## Introducción
En esta práctica se desarrolló un cliente MQTT con el fin de experimentar y entender el funcionamiento de este protocolo de mensajería ligero y eficiente. MQTT, es ampliamente utilizado en aplicaciones de Internet de las Cosas (IoT) debido a su bajo consumo de ancho de banda y su capacidad para operar en contextos de red inestables.

## Proceso de Aprendizaje
El proyecto comenzó con una revisión de la documentación y los principios fundamentales de MQTT, incluyendo su arquitectura de cliente/servidor y los conceptos de publicación/suscripción. A medida que avanzaba la practica, comprendimos la importancia de elegir un broker MQTT adecuado, y se decidió utilizar. Hacer y configurar el broker resultó ser uno de los primeros desafíos; sin embargo, logramos superarlo. Despues se pudo observar cómo los mensajes eran intercambiados.

Las imagenes de lo realizado se pueden ver en este repositorio tanto de cliente como la de servidor.

## Desafíos Encontrados
Uno de los principales retos fue la implementación de la conexión, ya que la comunicación intermitente a veces resultaba en pérdidas de paquetes. Esto llevó a implementar un mecanismo robusto de reconexión, donde se utilizó un manejo adecuado de errores. Además, enfrentamos dificultades en la gestión de la latencia entre el cliente y el servidor, lo que requirió ajustes en las configuraciones del cliente MQTT para optimizar el rendimiento.

## Conclusiones 
Al finalizar esta práctica logramos entender cómo funciona realmente el protocolo MQTT y por qué es tan usado. La parte de publicar y suscribirse a topics fue más sencilla de lo que pensabamos, pero conectar el cliente de forma estable nos costó más trabajo.

Lo que más rescatamos de esta práctica es que ahora entendemos mejor cómo se comunican los dispositivos en tiempo real. Trabajar con la arquitectura cliente/servidor nos ayudó a ver cómo funciona el flujo de mensajes y la importancia de tener una conexión confiable.
