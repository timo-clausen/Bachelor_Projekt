/*
 * device_control.h
 *
 *  Created on: 01.11.2021
 *      Author: TiC
 */

#include <stdint.h>
#include <stdbool.h>

typedef struct{
	bool main_power;
	bool ion_power;
	bool uv_power;
	uint8_t fan_power;
}device_control_t;

typedef struct{
	double air_temperature;
	uint32_t working_hours;
	uint32_t filter_hours;
}device_status_t;

typedef struct{
	bool wifi_connected;
	bool mqtt_connected;
}connect_state_t;


void set_new_data(device_control_t new_data);
void set_main_power(bool main_power);
void set_ion_power(bool ion_power);
void set_uv_power(bool uv_power);
void set_fan_power(uint8_t fan_power);

device_control_t get_device_control_struct();
device_status_t get_device_status_struct();
bool is_mqtt_connected();

void create_device_task();
void set_wifi_state(bool state);
void set_mqtt_state(bool state);
void set_device_status(double air_temperature, uint32_t filter_hours, uint32_t working_hours);



