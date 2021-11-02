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



void set_new_data(device_control_t new_data);
void set_main_power(bool main_power);
void set_ion_power(bool ion_power);
void set_uv_power(bool uv_power);
void set_fan_power(uint8_t fan_power);
