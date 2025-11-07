# Reporte de Práctica: Proyecto Cliente MQTT

## Introducción
En esta práctica se desarrolló un cliente MQTT con el fin de experimentar y entender el funcionamiento de este protocolo de mensajería ligero y eficiente. MQTT, que significa Message Queuing Telemetry Transport, es ampliamente utilizado en aplicaciones de Internet de las Cosas (IoT) debido a su bajo consumo de ancho de banda y su capacidad para operar en contextos de red inestables.

## Proceso de Aprendizaje
El proyecto comenzó con una revisión de la documentación y los principios fundamentales de MQTT, incluyendo su arquitectura de cliente/servidor y los conceptos de publicación/suscripción. A medida que avanzaba el proyecto, comprendí la importancia de elegir un broker MQTT adecuado, y se decidió utilizar [broker desarrollador]. Instalar y configurar el broker resultó ser uno de los primeros desafíos; sin embargo, con la ayuda de foros y tutoriales, logré superarlo. La teoría se aplicó a la práctica, y así pude observar cómo los mensajes eran intercambiados incluso con condiciones de red adversas.

## Desafíos Encontrados
Uno de los principales retos fue la implementación de la conexión, ya que la comunicación intermitente a veces resultaba en pérdidas de paquetes. Esto llevó a implementar un mecanismo robusto de reconexión, donde se utilizó un manejo adecuado de errores y se siguieron buenas prácticas de programación. Además, enfrenté dificultades en la gestión de la latencia entre el cliente y el servidor, lo que requirió ajustes en las configuraciones del cliente MQTT para optimizar el rendimiento.

## Implementación Técnica
La implementación se realizó utilizando módulos y drivers específicos que permiten la comunicación con sensores y otros dispositivos. La elección de los drivers adecuados fue crucial, y se analizaron varias opciones antes de decidirse por las más eficientes en términos de consumo de energía y compatibilidad de hardware. Durante el desarrollo, se prestó especial atención a la estructura del código y la eficiencia, asegurando que el cliente pudiera manejar múltiples conexiones y publicar mensajes en diferentes tópicos simultáneamente.

## Conclusiones 
En conclusión, esta práctica no solo me permitió fortalecer mis conocimientos en MQTT, sino que también enfatizó la importancia de la resiliencia en el diseño de sistemas de IoT. Cada desafío enfrentado se convirtió en una oportunidad para aprender y mejorar la implementación. A medida que el proyecto avanzaba, sentí un creciente sentido de logro al ver cómo cada componente del sistema interaccionaba entre sí.

Este informe representa un paso importante en mi formación como desarrollador en el mundo del IoT, equipándome con las habilidades y conocimientos necesarios para futuras instalaciones y proyectos.