/*
 * my_mqtt.c
 *
 *  Created on: 25.10.2021
 *      Author: TiC
 */

// AWS Cloud connect

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

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "json_parser.h"
#include "device_control.h"

#define UPLOAD_TOPIC	upload_topic //"test/topic/uplink"
#define DOWNLOAD_TOPIC download_topic //"test/topic/downlink"
#define UPLOAD_DB	upload_db_topic //"db/device_nr"

static const char *TAG = "my_mqtt";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static char *basic_upload_topic = "device/nr/uplink";
static char *upload_topic;
static char *basic_download_topic = "device/nr/downlink";
static char *download_topic;
static char *basic_upload_db_topic = "db/device_nr";
static char *upload_db_topic;

//#define CONFIG_BROKER_URI "mqtts://192.168.254.136:8883" SSID
//#define CONFIG_BROKER_URI "mqtt://192.168.254.254:1883"
//#define CONFIG_BROKER_URI "mqtt://192.168.0.23:1883"




#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t hivemq_server_cert_pem_start[]   asm("_binary_hivemq_server_cert_pem_start");  // Links nur ein Variablen Name Rechts muss zur Datei passen, komischerweise nur in Text. ob Minus oder Unterstrich ist egal. im Asembler jedoch kein Minus erlaubt
#endif

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

    uint8_t mac[8];
    int msg_id;
    uint32_t free_heap_size=0, min_free_heap_size=0;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_efuse_mac_get_default(mac);
            msg_id = esp_mqtt_client_subscribe(client, DOWNLOAD_TOPIC, 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            set_mqtt_state(true);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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
		.keepalive = 60,
		.disable_clean_session = true, 			// verpasse Nachrichten werden später zugestellt, aber will man das? Und qos 0 kann nicht veröffentlicht werden
		//.disable_auto_reconnect = true,

    };


    static uint8_t chipid[6];
    esp_efuse_mac_get_default(chipid);
    char *string_chipid = (char*) malloc(sizeof(char)*25);
    sprintf(string_chipid, "/ESP32_%02x%02x%02x", chipid[3], chipid[4], chipid[5]);

    upload_topic = (char*) malloc(sizeof(char)*(strlen(string_chipid)+strlen(basic_upload_topic)+1));
    sprintf(upload_topic, "%s%s", basic_upload_topic, string_chipid);
    download_topic = (char*) malloc(sizeof(char)*(strlen(string_chipid)+strlen(basic_download_topic)+1));
    sprintf(download_topic, "%s%s", basic_download_topic, string_chipid);
    upload_db_topic = (char*) malloc(sizeof(char)*(strlen(string_chipid)+strlen(basic_upload_db_topic)+1));
    sprintf(upload_db_topic, "%s%s", basic_upload_db_topic, string_chipid);

    ESP_LOGI(TAG, "Upload Topic: %s", upload_topic);
    ESP_LOGI(TAG, "Download Topic: %s", download_topic);
    ESP_LOGI(TAG, "Upload DB Topic: %s", upload_db_topic);


    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    mqtt_client = client;
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
	static uint8_t qos = 0;
	int msg_id;
	if(NULL != mqtt_client){
		msg_id = esp_mqtt_client_publish(mqtt_client, UPLOAD_DB, data, 0, qos, 0);

	}else{
		ESP_LOGE(TAG, "MQTT Client not initialized");
	}
}

