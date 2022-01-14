# DHT22 y MQTT
Este proyecto realiza la lectura del sensor DHT22 y la "envia" a thingsboard y es una variacion del codigo de DHT22only junto con los ejemplos wifi/station y mqtt/tcp de esp-idf.

## Datos importantes
- HOSTIP_MQTT: La direccion IP del ordenador donde se esta ejecutando la version local de thingsboard
- PORT_MQTT: Puerto TCP (1883)
- ACCESS_TOKEN: El token de acceso del dispositivo, se puede obtener en la interfaz de thingsboard
- TOPIC_MQTT: El topic o tema a donde se enviaran los datos
- EXAMPLE_ESP_WIFI_SSID: Nombre de la red WiFi a la que se conectara la ESP
- EXAMPLE_ESP_WIFI_PASS: contraseña de la red WiFi