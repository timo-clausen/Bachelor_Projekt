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


static const char *TAG = "json_parser";

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


	printf("JSON String: %s length: %d\n", json_string_t, string_length);
	cJSON_Delete(main_json_object); 			// es muss nur das main Objekt gelöscht werden, sub wird dann mit gelöscht
}
