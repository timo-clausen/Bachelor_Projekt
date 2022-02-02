#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <stdint.h>
#include "driver/touch_sensor.h"

#include "esp_wifi.h"
#include <esp_wifi_types.h>

#include "my_wifi.h"
#include "my_mqtt.h"
#include "device_control.h"
#include "json_parser.h"
#include "measurements.h"
#include "ota_update.h"
#include "touch_input.h"


#include "driver/touch_pad.h"

static const char *TAG = "my_main";

void set_log_levels(){

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

}


void app_main(void)
{

	create_device_task();
	set_log_levels();
	//Initialize NVS aus wifi_connect netconn
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_LOGE(TAG, "irgendwas mit NVS");
		ESP_ERROR_CHECK(nvs_flash_erase());
	    ret = nvs_flash_init();
	}


	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_touch_interface();
    create_measurements_task();
	wifi_init_sta();
    mqtt_app_start();
    create_json_task();

    verifi_new_ota_firmware();

    while (true) {

        uint32_t free_heap_size=0, min_free_heap_size=0;
        free_heap_size = esp_get_free_heap_size();
        min_free_heap_size = esp_get_minimum_free_heap_size();
        printf("aus der main free heap size = %d \t  min_free_heap_size = %d \n",free_heap_size,min_free_heap_size);
        vTaskDelay(300000 / portTICK_PERIOD_MS);


    }
}

