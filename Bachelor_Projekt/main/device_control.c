/*
 * device_control.c
 *
 *  Created on: 01.11.2021
 *      Author: TiC
 */

#include "device_control.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "device_control";

static bool new_data_flag = false;
static device_control_t device_status;

void set_new_data_flag(){
	new_data_flag = true;
	ESP_LOGI(TAG, "neue Steuerdaten erhalten");
	printf("main_power: %d, ",device_status.main_power);
	printf("ion_power: %d, ",device_status.ion_power);
	printf("uv_power: %d, ",device_status.uv_power);
	printf("fan power: %d\n",device_status.fan_power);

}

void set_main_power(bool main_power){
	device_status.main_power = main_power;
	set_new_data_flag();
}
void set_ion_power(bool ion_power){
	device_status.ion_power = ion_power;
	set_new_data_flag();
}
void set_uv_power(bool uv_power){
	device_status.uv_power = uv_power;
	set_new_data_flag();
}
void set_fan_power(uint8_t fan_power){
	device_status.fan_power = fan_power;
	set_new_data_flag();
}



void set_new_data(device_control_t new_data){
	device_status.main_power = new_data.main_power;
	device_status.ion_power = new_data.ion_power;
	device_status.uv_power = new_data.uv_power;
	device_status.fan_power = new_data.fan_power;

	set_new_data_flag();
}


