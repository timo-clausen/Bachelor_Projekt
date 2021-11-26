/*
 * touch_input.c
 *
 *  Created on: 24.11.2021
 *      Author: TiC
 */

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "driver/touch_pad.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "touch_input.h"
#include "device_control.h"

#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)			// scheint nur für die read anweisung zu gelten, nicht für interrupt
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_FAKTOR   (0.8)
#define TOUCH_PAD_FAN1			TOUCH_PAD_NUM0
#define TOUCH_PAD_FAN2			TOUCH_PAD_NUM2
#define TOUCH_PAD_FAN3			TOUCH_PAD_NUM3
#define TOUCH_PAD_FAN4			TOUCH_PAD_NUM4
#define TOUCH_PAD_FAN5			TOUCH_PAD_NUM5
#define TOUCH_PAD_MAIN_POWER	TOUCH_PAD_NUM6
#define TOUCH_PAD_UV_POWER		TOUCH_PAD_NUM7
#define TOUCH_PAD_ION_POWER		TOUCH_PAD_NUM8

static const char *TAG = "touch_input";
static uint32_t s_pad_init_val[TOUCH_PAD_MAX];
static bool s_pad_activated[TOUCH_PAD_MAX];
static bool pad_released = true;
static uint16_t testValue;
static uint8_t used_touch_pads[] = {TOUCH_PAD_FAN1,
									TOUCH_PAD_FAN2,
									TOUCH_PAD_FAN3,
									TOUCH_PAD_FAN4,
									TOUCH_PAD_FAN5,
									TOUCH_PAD_MAIN_POWER,
									TOUCH_PAD_UV_POWER,
									TOUCH_PAD_ION_POWER};
void init_touch_pads(){
	for(int i = 0; i < sizeof(used_touch_pads); i++){
		ESP_ERROR_CHECK(touch_pad_config(used_touch_pads[i], TOUCH_THRESH_NO_USE));
	}
}

void set_tresholds(){
	uint16_t touch_value;
	for(int i = 0; i < sizeof(used_touch_pads); i++){
		touch_pad_read_filtered(used_touch_pads[i], &touch_value);
		 s_pad_init_val[used_touch_pads[i]]=touch_value;
		 ESP_LOGD(TAG, "pad: [%d]  touch_value: %d",used_touch_pads[i], touch_value);
		 ESP_ERROR_CHECK(touch_pad_set_thresh(used_touch_pads[i], touch_value * TOUCH_THRESH_FAKTOR)); //(i, touch_value * 2 / 3));
	}
}

bool pad_is_aktiv(uint8_t pad_nr){
	uint16_t touch_value1;
	uint16_t touch_value2;
	uint16_t threshold;
	bool flag1, flag2;
	touch_pad_read_filtered(pad_nr, &touch_value1);
	touch_pad_get_thresh(pad_nr, &threshold);

	flag1 = touch_value1 < (threshold);
	vTaskDelay(50/portTICK_PERIOD_MS);
	touch_pad_read_filtered(pad_nr, &touch_value2);
	flag2 = touch_value2 < (threshold);
	ESP_LOGW(TAG, "threshold: %d, v1: %d, v2: %d, vT: %d, f1: %d, f2: %d", threshold, touch_value1, touch_value2, testValue, flag1, flag2);
	return flag1 || flag2;


}

void set_command(void *arg){
	uint8_t pushed_pad = (uint8_t)arg;
	device_control_t current_state;
	current_state = get_device_control_struct();

	switch(pushed_pad){
		case(TOUCH_PAD_FAN1):
			ESP_LOGI(TAG, "case 1");
			break;
		case(TOUCH_PAD_FAN2):
			ESP_LOGI(TAG, "case 2");
			break;
		case(TOUCH_PAD_FAN3):
			ESP_LOGI(TAG, "case 3");
			break;
		case(TOUCH_PAD_FAN4):
			ESP_LOGI(TAG, "case 4");
			break;
		case(TOUCH_PAD_FAN5):
			ESP_LOGI(TAG, "case 5");
			break;
		case(TOUCH_PAD_MAIN_POWER):
			ESP_LOGI(TAG, "case main");
			break;
		case(TOUCH_PAD_UV_POWER):
			ESP_LOGI(TAG, "case uv");
			break;
		case(TOUCH_PAD_ION_POWER):
			ESP_LOGI(TAG, "case ion");
			break;
		default:
			break;
	}

	while(pad_is_aktiv(pushed_pad)){
		vTaskDelay(20/portTICK_PERIOD_MS);
	}

	touch_pad_intr_clear();
	touch_pad_clear_status();
	//vTaskDelay(50/portTICK_PERIOD_MS);
	touch_pad_intr_enable();
	gpio_set_level(26, false);
	vTaskDelete(xTaskGetCurrentTaskHandle());
}


void touch_pad_isr(void *arg){
	touch_pad_read_filtered(7, &testValue);
	gpio_set_level(26, true);
	touch_pad_intr_disable();
	//touch_pad_intr_clear();
	uint32_t pad_intr = touch_pad_get_status();
	touch_pad_clear_status();
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        if ((pad_intr >> i) & 0x01) {
            //s_pad_activated[i] = true;

        	xTaskCreate(set_command, "touch_set_command", 1024*2, (void*)i, 10, NULL);
        }
    }
}







void init_touch_interface(){
	ESP_ERROR_CHECK(touch_pad_init());
	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
	init_touch_pads();
	vTaskDelay(1000/portTICK_RATE_MS);
	set_tresholds();
	touch_pad_isr_register(touch_pad_isr, NULL);
	touch_pad_intr_enable();

	gpio_reset_pin(26);
	gpio_set_direction(26, GPIO_MODE_OUTPUT);
}

/*
void read_touch(void *arg){
	uint16_t value = 0;
	vTaskDelay(1000/portTICK_RATE_MS);
	set_tresholds();
	touch_pad_isr_register(touch_pad_isr, NULL);
	touch_pad_intr_enable();

	while(1){
		vTaskDelay(100/portTICK_RATE_MS);
		for (uint8_t i = 0; i < TOUCH_PAD_MAX; i++) {
			if (s_pad_activated[i] == true) {
				set_command(i);
				pad_released = false;
				touch_pad_read_filtered(i, &value);
				ESP_LOGI(TAG, "value: %d; init val: %d", value, s_pad_init_val[i]);
				ESP_LOGI(TAG, "T%d activated!", i);
				s_pad_activated[i] = false;
			}
		}
	}
}


void create_touch_task(void){

	ESP_ERROR_CHECK(touch_pad_init());
	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
	init_touch_pads();

	xTaskCreate(read_touch, "read_touch", 1024*2, NULL, 10, NULL);

}
*/
