/*
 * my_mqtt.c
 *
 *  Created on: 25.10.2021
 *      Author: TiC
 */

#include "my_mqtt.h"
#include <stdio.h>



#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h "

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "json_parser.h"
#include "device_control.h"

#define UPLOAD_TOPIC "test/topic/uplink"
#define DOWNLOAD_TOPIC "test/topic/downlink"
#define UPLOAD_DB	"db/device_nr"

static const char *TAG = "my_mqtt";
static esp_mqtt_client_handle_t mqtt_client = NULL;


//#define CONFIG_BROKER_URI "mqtts://192.168.254.136:8883" SSID
//#define CONFIG_BROKER_URI "mqtt://192.168.254.254:1883"


#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t hivemq_server_cert_pem_start[]   asm("_binary_hivemq_server_cert_pem_start");  // Links nur ein Variablen Name Rechts muss zur Datei passen, komischerweise nur in Text. ob Minus oder Unterstrich ist egal. im Asembler jedoch kein Minus erlaubt
#endif
//extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

//extern const uint8_t hivemq_mqtt_client_cert_2_pem[]   asm("_binary_mqtt_client_cert_2_pem_start");
//extern const uint8_t hivemq_mqtt_client_key_2_pem[]   asm("_binary_mqtt_client_key_2_pem_start");

// Note: this function is for testing purposes only publishing part of the active partition
//       (to be checked against the original binary)
//



static void send_binary(esp_mqtt_client_handle_t client)
{
    spi_flash_mmap_handle_t out_handle;
    const void *binary_address;
    const esp_partition_t* partition = esp_ota_get_running_partition();
    esp_partition_mmap(partition, 0, partition->size, SPI_FLASH_MMAP_DATA, &binary_address, &out_handle);
    // sending only the configured portion of the partition (if it's less than the partition size)
    int binary_size = MIN(CONFIG_BROKER_BIN_SIZE_TO_SEND,partition->size);
    int msg_id = esp_mqtt_client_publish(client, "/topic/binary", binary_address, binary_size, 0, 0);
    ESP_LOGI(TAG, "binary sent with msg_id=%d", msg_id);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    mqtt_client = client;

    int msg_id;
    uint32_t free_heap_size=0, min_free_heap_size=0;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            msg_id = esp_mqtt_client_subscribe(client, DOWNLOAD_TOPIC, 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            //msg_id = esp_mqtt_client_publish(client, "test/topic", "data_3", 0, 1, 0);
            set_mqtt_state(true);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            //esp_mqtt_client_stop(client);
            //mqtt_app_start();

            free_heap_size = esp_get_free_heap_size();
            min_free_heap_size = esp_get_minimum_free_heap_size();
            printf("\n free heap size = %d \t  min_free_heap_size = %d \n",free_heap_size,min_free_heap_size);
            set_mqtt_state(false);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            if (strncmp(event->data, "send binary please", event->data_len) == 0) {
                ESP_LOGI(TAG, "Sending the binary");
                send_binary(client);
            }
            parse_json(event->data, event->data_len);

            //free_heap_size = esp_get_free_heap_size();
            //min_free_heap_size = esp_get_minimum_free_heap_size();
            //printf("\n free heap size = %d \t  min_free_heap_size = %d \n",free_heap_size,min_free_heap_size);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                                                                strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .cert_pem = (const char *)hivemq_server_cert_pem_start,
		//.client_cert_pem = (const char *)hivemq_mqtt_client_cert_2_pem,
		//.client_key_pem = (const char *)hivemq_mqtt_client_key_2_pem,
		//.clientkey_password = "testPass",
		//.clientkey_password_len = 8,
		.username = "esp32_2",
		.password = "esp32_2pw",
		.reconnect_timeout_ms = 2000,
		.keepalive = 160,
		.disable_clean_session = true, 			// verpasse Nachrichten werden später zugestellt, aber will man das? Und qos 0 kann nicht veröffentlicht werden
		//.disable_auto_reconnect = true,

    };
    //printf((const char *)hivemq_server_cert_pem_start);
    //printf(" das sind die daten\n");


    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_publish_status(const char *data){
	static uint8_t qos = 1;
	int msg_id;
	if(NULL != mqtt_client){
		msg_id = esp_mqtt_client_publish(mqtt_client, UPLOAD_TOPIC, data, 0, qos, 0);

	}else{
		ESP_LOGE(TAG, "MQTT Client not initialized");
	}
}

void mqtt_publish_db(const char *data){
	static uint8_t qos = 1;				// qos 0 geht nicht bei clean session
	int msg_id;
	if(NULL != mqtt_client){
		msg_id = esp_mqtt_client_publish(mqtt_client, UPLOAD_DB, data, 0, qos, 0);

	}else{
		ESP_LOGE(TAG, "MQTT Client not initialized");
	}
}

