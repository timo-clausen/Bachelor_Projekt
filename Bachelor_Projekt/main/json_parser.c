/*
 * json_parser.c
 *
 *  Created on: 01.11.2021
 *      Author: TiC
 */

#include "json_parser.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cJSON.h"
#include "device_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_mqtt.h"

#include "ota_update.h"


static const char *TAG = "json_parser";
static bool send_uplink_message_flag = false;

void set_send_uplink_message_flag(){
	send_uplink_message_flag = true;
}

void parse_status_command(cJSON *sub_json_object) {
	device_control_t new_data;
	new_data.main_power = cJSON_IsTrue(cJSON_GetObjectItem(sub_json_object, "main_power"));
	new_data.ion_power = cJSON_IsTrue(cJSON_GetObjectItem(sub_json_object, "ion_power"));
	new_data.uv_power = cJSON_IsTrue(cJSON_GetObjectItem(sub_json_object, "uv_power"));
	new_data.fan_power = cJSON_GetObjectItem(sub_json_object, "fan_power")->valueint;
	set_new_data(new_data);
}

void parse_single_command(cJSON *sub_json_object){
	if(cJSON_HasObjectItem(sub_json_object, "main_power")){
		set_main_power(cJSON_IsTrue( cJSON_GetObjectItem(sub_json_object, "main_power")));

	}else if(cJSON_HasObjectItem(sub_json_object, "ion_power")){
		set_ion_power(cJSON_IsTrue( cJSON_GetObjectItem(sub_json_object, "ion_power")));

	}else if(cJSON_HasObjectItem(sub_json_object, "uv_power")){
		set_uv_power(cJSON_IsTrue(cJSON_GetObjectItem(sub_json_object, "uv_power")));

	}else if(cJSON_HasObjectItem(sub_json_object, "fan_power")){
		set_fan_power(cJSON_GetObjectItem(sub_json_object, "fan_power")->valueint);
	}else if(cJSON_HasObjectItem(sub_json_object, "start_ota_update")){
		start_ota_task(cJSON_IsTrue(cJSON_GetObjectItem(sub_json_object, "start_ota_update")));
	}

}

void parse_json(char *json_string_t, int string_length){
	json_string_t[string_length] = '\0';
	cJSON *main_json_object = cJSON_Parse(json_string_t);

	if(cJSON_HasObjectItem(main_json_object, "status_command")){
		cJSON *sub_json_object = cJSON_GetObjectItem(main_json_object, "status_command");
		parse_status_command(sub_json_object);
		ESP_LOGI(TAG, "status command received");
	}else if(cJSON_HasObjectItem(main_json_object, "single_command")){
		cJSON *sub_json_object = cJSON_GetObjectItem(main_json_object, "single_command");
		parse_single_command(sub_json_object);
		ESP_LOGI(TAG, "single command received");
	}

	//printf("JSON String: %s length: %d\n", json_string_t, string_length);
	cJSON_Delete(main_json_object); 			// es muss nur das main Objekt gelöscht werden, sub wird dann mit gelöscht
	set_send_uplink_message_flag();
}

void send_uplink_message(){
	device_control_t device_control = get_device_control_struct();
	device_status_t device_status = get_device_status_struct();
	char *json_string;

	cJSON *main_json_object = cJSON_CreateObject();
	cJSON *sub_json_object = cJSON_CreateObject();
	cJSON_AddItemToObject(main_json_object, "uplink_message", sub_json_object);
	cJSON_AddItemToObject(sub_json_object, "main_power", cJSON_CreateBool(device_control.main_power));
	cJSON_AddItemToObject(sub_json_object, "ion_power", cJSON_CreateBool(device_control.ion_power));
	cJSON_AddItemToObject(sub_json_object, "uv_power", cJSON_CreateBool(device_control.uv_power));
	cJSON_AddItemToObject(sub_json_object, "fan_power", cJSON_CreateNumber(device_control.fan_power));

	cJSON_AddItemToObject(sub_json_object, "air_temerature", cJSON_CreateNumber(device_status.air_temperature));
	cJSON_AddItemToObject(sub_json_object, "filter_hours", cJSON_CreateNumber(device_status.filter_hours));
	cJSON_AddItemToObject(sub_json_object, "working_hours", cJSON_CreateNumber(device_status.working_hours));

	json_string = malloc (sizeof(cJSON_PrintUnformatted(main_json_object)));
	json_string = cJSON_PrintUnformatted(main_json_object);
	mqtt_publish(json_string);
	cJSON_Delete(main_json_object);
	ESP_LOGI(TAG, "uplink message sheduled");
	free(json_string);
	send_uplink_message_flag = false;
}

void json_main_task(void *arg){

	while(1){

		vTaskDelay(500/portTICK_RATE_MS);
		if(true == send_uplink_message_flag){
			//send_uplink_message();
		}

	}
}

void create_json_task(){
	xTaskCreate(json_main_task, "json task", 1024*3, NULL, 10, NULL);  // Stack overflow, der Task zum empfangen hat default 6kb 2kb sind zu wenig
}


