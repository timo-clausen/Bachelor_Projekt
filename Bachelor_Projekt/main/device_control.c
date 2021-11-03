/*
 * device_control.c
 *
 *  Created on: 01.11.2021
 *      Author: TiC
 */

#include "device_control.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static const char *TAG = "device_control";

static bool new_data_flag = false;
static device_control_t device_control;
static device_status_t device_status;

void set_new_data_flag(){
	new_data_flag = true;
	ESP_LOGI(TAG, "neue Steuerdaten erhalten");
	printf("main_power: %d, ",device_control.main_power);
	printf("ion_power: %d, ",device_control.ion_power);
	printf("uv_power: %d, ",device_control.uv_power);
	printf("fan power: %d\n",device_control.fan_power);

}

void set_main_power(bool main_power){
	device_control.main_power = main_power;
	set_new_data_flag();
}
void set_ion_power(bool ion_power){
	device_control.ion_power = ion_power;
	set_new_data_flag();
}
void set_uv_power(bool uv_power){
	device_control.uv_power = uv_power;
	set_new_data_flag();
}
void set_fan_power(uint8_t fan_power){
	device_control.fan_power = fan_power;
	set_new_data_flag();
}

void set_new_data(device_control_t new_data){
	device_control.main_power = new_data.main_power;
	device_control.ion_power = new_data.ion_power;
	device_control.uv_power = new_data.uv_power;
	device_control.fan_power = new_data.fan_power;

	set_new_data_flag();
}

device_control_t get_device_control_struct(){
	return device_control;
}

device_status_t get_device_status_struct(){
	return device_status;
}

void device_main_task(void *arg){
	while(1){
		vTaskDelay(10/portTICK_RATE_MS);
		//ESP_LOGI(TAG, "device task");
		device_status.air_temperature = 21.5;
		device_status.working_hours ++;			//werden komischer weise mit jedem publish um 5 erhöt
		device_status.filter_hours = 26;
	}
}

void create_device_task(){
	xTaskCreate(device_main_task, "device task", 1024*2, NULL, 10, NULL);
}

