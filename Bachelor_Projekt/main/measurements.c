/*
 * measurements.c
 *
 *  Created on: 11.11.2021
 *      Author: TiC
 */


#include "measurements.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "device_control.h"
#include "json_parser.h"

static const char *TAG = "measurements";

/* ADC1 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;

static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;

static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;


static void check_efuse(void)
{

    //Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}


static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

double measure_air_temperature(){
	uint32_t adc_reading = 0;
	//Multisampling
	for (int i = 0; i < NO_OF_SAMPLES; i++) {
		adc_reading += adc1_get_raw((adc1_channel_t)channel);
	}
	adc_reading /= NO_OF_SAMPLES;
	//Convert adc_reading to voltage in mV
	uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars)-60;  //-60  draußen ohne
	double temperature = (voltage*28.9)/1000-22.5;
	printf("Raw: %d\tVoltage: %dmV\t T: %.2f C\n", adc_reading, voltage, temperature);
	return temperature;
}

void measurements_task(void)
{
	static float temperature;
	static uint32_t working_hours;
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC

    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    vTaskDelay(pdMS_TO_TICKS(5000));
    //Continuously sample ADC1
    while (1) {

    	temperature = measure_air_temperature();
    	working_hours = get_device_status_struct().working_hours;
        set_device_status(temperature, 60, ++working_hours);			// jetzt natürlich min
        set_send_status_db_flag();
        vTaskDelay(pdMS_TO_TICKS(60000));

    }
}


void create_measurements_task(){
	xTaskCreate(measurements_task, "measurements_task", 1024*2, NULL, 10, NULL);

}

