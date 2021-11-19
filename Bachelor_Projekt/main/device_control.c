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
#include "driver/gpio.h"

#define IO_UV_PIN 15
#define IO_IONIC_PIN 2
#define IO_MAIN_PIN 4
#define IO_MODE_LED1_PIN 25
#define IO_MODE_LED2_PIN 26
#define IO_MODE_LED3_PIN 27
#define IO_MODE_LED4_PIN 14
#define IO_MODE_LED5_PIN 12

static TaskHandle_t led_task_handl;
static bool new_data_flag = false;
static device_control_t device_control ={.main_power = true};
static device_status_t device_status;
static connect_state_t connect_state = {.wifi_connected = false,
										.mqtt_connected = false};

static const char *TAG = "device_control";


void set_new_data_flag() {
	new_data_flag = true;
	//ESP_LOGI(TAG, "neue Steuerdaten erhalten");
	printf("main_power: %d, ", device_control.main_power);
	printf("ion_power: %d, ", device_control.ion_power);
	printf("uv_power: %d, ", device_control.uv_power);
	printf("fan power: %d\n", device_control.fan_power);

}

void set_main_power(bool main_power) {
	device_control.main_power = main_power;
	set_new_data_flag();
}
void set_ion_power(bool ion_power) {
	device_control.ion_power = ion_power;
	set_new_data_flag();
}
void set_uv_power(bool uv_power) {
	device_control.uv_power = uv_power;
	set_new_data_flag();
}
void set_fan_power(uint8_t fan_power) {
	device_control.fan_power = fan_power;
	set_new_data_flag();
}

void set_new_data(device_control_t new_data) {
	device_control.main_power = new_data.main_power;
	device_control.ion_power = new_data.ion_power;
	device_control.uv_power = new_data.uv_power;
	device_control.fan_power = new_data.fan_power;

	set_new_data_flag();
}

device_control_t get_device_control_struct() {
	return device_control;
}

device_status_t get_device_status_struct() {
	return device_status;
}
void mode_leds_off() {
	gpio_set_level(IO_MODE_LED5_PIN, false);
	gpio_set_level(IO_MODE_LED4_PIN, false);
	gpio_set_level(IO_MODE_LED3_PIN, false);
	gpio_set_level(IO_MODE_LED2_PIN, false);
	gpio_set_level(IO_MODE_LED1_PIN, false);
}

void switch_off() {
	gpio_set_level(IO_IONIC_PIN, false);
	gpio_set_level(IO_UV_PIN, false);
	gpio_set_level(IO_MAIN_PIN, false);
	mode_leds_off();
}

void prerare_io_ports() {

	gpio_reset_pin(IO_IONIC_PIN);
	gpio_reset_pin(IO_UV_PIN);
	gpio_reset_pin(IO_MAIN_PIN);
	gpio_reset_pin(IO_MODE_LED1_PIN);
	gpio_reset_pin(IO_MODE_LED2_PIN);
	gpio_reset_pin(IO_MODE_LED3_PIN);
	gpio_reset_pin(IO_MODE_LED4_PIN);
	gpio_reset_pin(IO_MODE_LED5_PIN);

	gpio_set_direction(IO_IONIC_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_UV_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_MAIN_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_MODE_LED1_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_MODE_LED2_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_MODE_LED3_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_MODE_LED4_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IO_MODE_LED5_PIN, GPIO_MODE_OUTPUT);

}

void adjust_io_ports() {

	//ESP_LOGI(TAG, "adjust oi Prots");
	new_data_flag = false;
	if (false == device_control.main_power) {
		switch_off();
	} else {
		gpio_set_level(IO_IONIC_PIN, device_control.ion_power);
		gpio_set_level(IO_UV_PIN, device_control.uv_power);
		gpio_set_level(IO_MAIN_PIN, device_control.main_power);

		mode_leds_off();
		switch (device_control.fan_power) {
			case 5:
				gpio_set_level(IO_MODE_LED5_PIN, true);
				// fall through
				/* no break */
			case 4:
				gpio_set_level(IO_MODE_LED4_PIN, true);
				// fall through
				/* no break */
			case 3:
				gpio_set_level(IO_MODE_LED3_PIN, true);
				// fall through
				/* no break */
			case 2:
				gpio_set_level(IO_MODE_LED2_PIN, true);
				// fall through
				/* no break */
			case 1:
				gpio_set_level(IO_MODE_LED1_PIN, true);
				// fall through
				/* no break */
			default:
				break;
		}
	}
}

void led_task(void *arg){
	uint16_t delay = 150;
	bool led_b = true;
	while(1){
		if(false == connect_state.wifi_connected){
			gpio_set_level(IO_MAIN_PIN, led_b);
			delay = 150;
		}else if(false == connect_state.mqtt_connected){
			gpio_set_level(IO_MAIN_PIN, led_b);
			delay = 600;
		}else{
			gpio_set_level(IO_MAIN_PIN, device_control.main_power);
		}
		vTaskDelay(delay / portTICK_RATE_MS);
		led_b = !led_b;
	}
}


void set_wifi_state(bool state){
	connect_state.wifi_connected = state;
}
void set_mqtt_state(bool state){
	connect_state.mqtt_connected = state;
}


void device_main_task(void *arg) {



	while (1) {
		vTaskDelay(10 / portTICK_RATE_MS);
		//ESP_LOGI(TAG, "device task");
		device_status.air_temperature = 21.7;
		device_status.working_hours++;
		device_status.filter_hours = 26;
		if (true == new_data_flag) {
			adjust_io_ports();
		}
	}
}

void create_device_task() {
	prerare_io_ports();
	xTaskCreate(device_main_task, "device task", 1024 * 2, NULL, 10, NULL);
	xTaskCreate(led_task, "LED Task", 1024 * 2, NULL, 10, &led_task_handl);
}


