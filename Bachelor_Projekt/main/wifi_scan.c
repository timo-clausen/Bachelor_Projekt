/*
 * wifi_scan.c
 *
 *  Created on: 17.02.2022
 *      Author: TiC
 */

#include "esp_wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "wifi_scan.h"

static const char *TAG = "wifi_scan";

void scan_wifi_ap(){
	 ESP_LOGI(TAG, "wifi scan started");

	 wifi_scan_config_t scan_config = {
			 .scan_type = WIFI_SCAN_TYPE_PASSIVE,
			 .scan_time.passive = 1500
			 };
	 wifi_ap_record_t ap_record [10];
	 uint16_t ap_count = 22;
	 uint16_t number = 10;
	 memset(ap_record, 0, sizeof(ap_record));

	 esp_wifi_scan_start(NULL, true);
	 ESP_LOGI(TAG, "wifi scan !!!!!!!!!!!!!!!!!!!!");
	 //vTaskDelay(10000/portTICK_PERIOD_MS);
	 //ESP_ERROR_CHECK(esp_wifi_scan_stop());
	 //ESP_LOGI(TAG, "wifi scan stoped");
	 ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_record));
	 ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	 ESP_LOGW(TAG, "anzahl ap: %d", ap_count);
	 for(int i=0; i<ap_count; i++){
		 //printf("ap %d: %s BSSID: %s pri: %s 2nd: %s rssi: %s auth: %s p_c: %u g_c: %s ant: %s b: %b g: %b n: %b lr: %b wps: %b resp: %b, init: %b C: %s\n", i+1, ap_record[i].ssid, ap_record[i].bssid, ap_record[i].primary, ap_record[i].second, ap_record[i].rssi, ap_record[i].authmode, ap_record[i].pairwise_cipher, ap_record[i].group_cipher, ap_record[i].ant, ap_record[i].phy_11b, ap_record[i].phy_11g, ap_record[i].phy_11n, ap_record[i].phy_lr, ap_record[i].wps, ap_record[i].ftm_responder, ap_record[i].ftm_initiator, ap_record[i].country);
	 }

}
