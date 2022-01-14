# DHT22 y MQTT
Este proyecto realiza la lectura del sensor DHT22 y la "envia" a Thingsboard. Es una variacion del codigo de DHT22only junto con los ejemplos wifi/station y mqtt/tcp de esp-idf.

El archivo esp32panel.json es el panel que use para la lectura de los datos en Thingsboard.

## Datos importantes
- HOSTIP_MQTT: La direccion IP del ordenador donde se esta ejecutando la version local de Thingsboard
- PORT_MQTT: Puerto TCP (1883)
- ACCESS_TOKEN: El token de acceso del dispositivo, se puede obtener en la interfaz de Thingsboard
- TOPIC_MQTT: El topic o tema a donde se enviaran los datos
- EXAMPLE_ESP_WIFI_SSID: Nombre de la red WiFi a la que se conectara la ESP
- EXAMPLE_ESP_WIFI_PASS: contraseña de la red WiFi
