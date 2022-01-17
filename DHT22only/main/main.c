#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "DHT.h"

static const char *TAG = "DHT";
#define BLINK_GPIO 5
#define DHT_GPIO 4

void DHT_task(void *pvParameter)
{
    setDHTgpio(DHT_GPIO);
    ESP_LOGI(TAG, "Starting DHT Task\n\n");
    // configuracion del gpio para un parpadeo
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    float humedad, temperatura;

    while (1)
    {
        ESP_LOGI(TAG, "=== Reading DHT ===\n");
        int ret = readDHT();

        errorHandler(ret);

        humedad = getHumidity();
        temperatura = getTemperature();

        ESP_LOGI(TAG, "Hum: %.1f Tmp: %.1f\n", humedad, temperatura);

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

    xTaskCreate(&DHT_task, "DHT_task", 2048, NULL, 5, NULL);
}
