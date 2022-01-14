#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

///////////////// includes para la parte de gpios y el dht
#include "driver/gpio.h"

#include "DHT.h"
///////////////// fin de includes gpio y dht

///////////////// includes para la parte de la conexion a la red
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sys.h"

///////////////// fin de includes para la conexion a la red

static const char *TAG = "dhtYmqtt";

#define HOSTIP_MQTT     "192.168.1.xx"          // IP address of the MQTT broker
#define PORT_MQTT       1883
#define ACCESS_TOKEN    "NB8gd8mglmC6rClVUj1G"
#define TOPIC_MQTT      "v1/devices/me/telemetry"

///////////////// seccion para la parte de la conexion a la red (Fin en linea 154)

#define EXAMPLE_ESP_WIFI_SSID      "name"       // The name of the Wi-Fi network you want to connect to
#define EXAMPLE_ESP_WIFI_PASS      "password"   // The password required to connect to Wi-Fi
#define EXAMPLE_ESP_MAXIMUM_RETRY  5            // The number of times the ESP will attempt to connect to the Wi-Fi network

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

///////////////// WIFI END ///////////////////////////////

#define BLINK_GPIO 5

void DHT_task(void *pvParameter)
{

    setDHTgpio(4);
    ESP_LOGI(TAG, "Starting DHT Task\n\n");

    // configuracion del gpio para un parpadeo
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    float humedad, temperatura;
    int msg_id;
    char msgt[20];
    char msgh[20];

    //configuracion del mqtt client
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = HOSTIP_MQTT,             // IP address of the MQTT broker
        .port = PORT_MQTT,
        .username = ACCESS_TOKEN,        // ACCESS_TOKEN
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);


    while (1)
    {
        ESP_LOGI(TAG, "=== Reading DHT ===\n");
        int ret = readDHT();

        errorHandler(ret);

        humedad = getHumidity();
        temperatura = getTemperature();

        ESP_LOGI(TAG, "Hum: %.1f Tmp: %.1f\n", humedad, temperatura);

        sprintf(msgt, "{temperature:%.1f}", temperatura);
        sprintf(msgh, "{humidity:%.1f}", humedad);

        msg_id = esp_mqtt_client_publish(client, TOPIC_MQTT, msgt, 0, 1, 0);
        msg_id = esp_mqtt_client_publish(client, TOPIC_MQTT, msgh, 0, 1, 0);

        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !!
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(2000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
    }
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreate(&DHT_task, "DHT_task", 2048, NULL, 5, NULL);
}