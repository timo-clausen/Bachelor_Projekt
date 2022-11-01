/*
 * enviromental_sensor.c
 *
 *  Created on: 30.03.2022
 *      Author: TiC
 */


#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_spi_flash.h"


#include "sen5x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"


#include "sensirion_config.h"
#include "sensirion_i2c.h"
#include "sensirion_i2c_hal.h"



#include "driver/i2c.h"
#include "freertos/task.h"
#include "hal/i2c_hal.h"
#include "esp_log.h"
#include "cJSON.h"
#include "my_mqtt.h"
#include "enviromental_sensor.h"

static i2c_port_t i2c_port = I2C_NUM_0;


#define I2C_CONTEX_INIT_DEF(uart_num) {\
    .hal.dev = I2C_LL_GET_HW(uart_num),\
    .spinlock = portMUX_INITIALIZER_UNLOCKED,\
    .hw_enabled = false,\
}



typedef struct {
    i2c_hw_cmd_t hw_cmd;
    uint8_t *data;     /*!< data address */
    uint8_t byte_cmd;  /*!< to save cmd for one byte command mode */
} i2c_cmd_t;


typedef struct i2c_cmd_link {
    i2c_cmd_t cmd;              /*!< command in current cmd link */
    struct i2c_cmd_link *next;  /*!< next cmd link */
} i2c_cmd_link_t;

typedef struct {
    i2c_cmd_link_t *head;     /*!< head of the command link */
    i2c_cmd_link_t *cur;      /*!< last node of the command link */
    i2c_cmd_link_t *free;     /*!< the first node to free of the command link */
} i2c_cmd_desc_t;

typedef struct {
    int i2c_num;                     /*!< I2C port number */
    int mode;                        /*!< I2C mode, master or slave */
    intr_handle_t intr_handle;       /*!< I2C interrupt handle*/
    int cmd_idx;                     /*!< record current command index, for master mode */
    int status;                      /*!< record current command status, for master mode */
    int rx_cnt;                      /*!< record current read index, for master mode */
    uint8_t data_buf[SOC_I2C_FIFO_LEN ];  /*!< a buffer to store i2c fifo data */

    i2c_cmd_desc_t cmd_link;         /*!< I2C command link */
    QueueHandle_t cmd_evt_queue;     /*!< I2C command event queue */
#if CONFIG_SPIRAM_USE_MALLOC
    uint8_t *evt_queue_storage;      /*!< The buffer that will hold the items in the queue */
    int intr_alloc_flags;            /*!< Used to allocate the interrupt */
    StaticQueue_t evt_queue_buffer;  /*!< The buffer that will hold the queue structure*/
#endif
    xSemaphoreHandle cmd_mux;        /*!< semaphore to lock command process */
#ifdef CONFIG_PM_ENABLE
    esp_pm_lock_handle_t pm_lock;
#endif

    xSemaphoreHandle slv_rx_mux;     /*!< slave rx buffer mux */
    xSemaphoreHandle slv_tx_mux;     /*!< slave tx buffer mux */
    size_t rx_buf_length;            /*!< rx buffer length */
    RingbufHandle_t rx_ring_buf;     /*!< rx ringbuffer handler of slave mode */
    size_t tx_buf_length;            /*!< tx buffer length */
    RingbufHandle_t tx_ring_buf;     /*!< tx ringbuffer handler of slave mode */
} i2c_obj_t;



static i2c_obj_t *p_i2c_obj[I2C_NUM_MAX] = {0};

typedef struct {
    i2c_hal_context_t hal;      /*!< I2C hal context */
    portMUX_TYPE spinlock;
    bool hw_enabled;
#if !SOC_I2C_SUPPORT_HW_CLR_BUS
    int scl_io_num;
    int sda_io_num;
#endif
} i2c_context_t;

static i2c_context_t i2c_context[I2C_NUM_MAX] = {
    I2C_CONTEX_INIT_DEF(I2C_NUM_0),
#if I2C_NUM_MAX > 1
    I2C_CONTEX_INIT_DEF(I2C_NUM_1),
#endif
};

void send_status_db_air_data(
					uint16_t mass_concentration_pm1p0,
					uint16_t mass_concentration_pm2p5,
					uint16_t mass_concentration_pm4p0,
					uint16_t mass_concentration_pm10p0,
					int16_t ambient_humidity,
					int16_t ambient_temperature,
					int16_t voc_index){

	static char *device_id = "ESP_AIR";
	char *json_string;
	cJSON *main_json_object = cJSON_CreateObject();

	//mass_concentration_pm1p0 = 10;
	cJSON_AddItemToObject(main_json_object, "Device_ID", cJSON_CreateString(device_id));
	cJSON_AddItemToObject(main_json_object, "pm1p0", cJSON_CreateNumber((double)(mass_concentration_pm1p0/ 10.0f) + 0.0000000001));
	cJSON_AddItemToObject(main_json_object, "pm2p5", cJSON_CreateNumber((double)(mass_concentration_pm2p5/ 10.0f) + 0.0000000001));
	cJSON_AddItemToObject(main_json_object, "pm4p0", cJSON_CreateNumber((double)(mass_concentration_pm4p0/ 10.0f) + 0.0000000001));
	cJSON_AddItemToObject(main_json_object, "pm10p0", cJSON_CreateNumber((double)(mass_concentration_pm10p0/ 10.0f) + 0.0000000001));
	cJSON_AddItemToObject(main_json_object, "ambient_humidity", cJSON_CreateNumber(ambient_humidity/100.0f + 0.0000000001));
	cJSON_AddItemToObject(main_json_object, "ambient_temperature", cJSON_CreateNumber(ambient_temperature/200.0f + 0.0000000001));
	cJSON_AddItemToObject(main_json_object, "voc_index", cJSON_CreateNumber(voc_index/ 10.0f));

	//cJSON_AddItemToObject(sub_json_object, "air_temperature", cJSON_CreateNumber(device_status.air_temperature));
	//cJSON_AddItemToObject(sub_json_object, "filter_hours", cJSON_CreateNumber(device_status.filter_hours));
	//cJSON_AddItemToObject(sub_json_object, "working_hours", cJSON_CreateNumber(device_status.working_hours));

	json_string = malloc (sizeof(cJSON_PrintUnformatted(main_json_object)));
	json_string = cJSON_PrintUnformatted(main_json_object);
	mqtt_publish_enviromental_sensor_data(json_string);
	printf (json_string);
	printf("\n");
	cJSON_Delete(main_json_object);
	free(json_string);

}


void sensor_task(){
	int16_t error = 0;

	uint16_t mass_concentration_pm1p0;
	uint16_t mass_concentration_pm2p5;
	uint16_t mass_concentration_pm4p0;
	uint16_t mass_concentration_pm10p0;
	int16_t ambient_humidity;
	int16_t ambient_temperature;
	int16_t voc_index;
	int16_t nox_index;



vTaskDelay(20000 / portTICK_PERIOD_MS);

	while(1){
		error = sen5x_read_measured_values(
			&mass_concentration_pm1p0, &mass_concentration_pm2p5,
			&mass_concentration_pm4p0, &mass_concentration_pm10p0,
			&ambient_humidity, &ambient_temperature, &voc_index, &nox_index);

		ESP_LOGI("", "-------------------------------------");
		if (error) {
			printf("Error executing sen5x_read_measured_values(): %i\n", error);
		} else {
			send_status_db_air_data(
							mass_concentration_pm1p0,
							mass_concentration_pm2p5,
							mass_concentration_pm4p0,
							mass_concentration_pm10p0,
							ambient_humidity,
							ambient_temperature,
							voc_index );

			printf("Mass concentration pm1p0: %.1f 痢/m許n", mass_concentration_pm1p0 / 10.0f);
			printf("Mass concentration pm2p5: %.1f 痢/m許n", mass_concentration_pm2p5 / 10.0f);
			printf("Mass concentration pm4p0: %.1f 痢/m許n", mass_concentration_pm4p0 / 10.0f);
			printf("Mass concentration pm10p0: %.1f 痢/m許n", mass_concentration_pm10p0 / 10.0f);
			printf("Ambient humidity: %.1f %%RH\n", ambient_humidity / 100.0f);
			printf("Ambient temperature: %.1f 蚓\n", ambient_temperature / 200.0f);
			printf("Voc index: %.1f\n", (float)(voc_index / 10.0f));
		}
		vTaskDelay(60000 / portTICK_PERIOD_MS);
	}
}


void sensor_init(void)
{

	int16_t error = 0;

	sensirion_i2c_hal_init();
	error = sen5x_device_reset();
	if (error) {
		printf("Error executing sen5x_device_reset(): %i\n", error);
	}

	sen5x_start_measurement();
	printf("Start environmental Measurement \n\n");

	vTaskDelay(55 / portTICK_PERIOD_MS);
	sen5x_start_fan_cleaning();

	// Read Measurement
	sensirion_i2c_hal_sleep_usec(1000000);

	xTaskCreate((void*)sensor_task, "sensor Task", 1024*6, (void*)NULL, 10, NULL);




/*	while (1){
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		error = sen5x_read_measured_values(
			&mass_concentration_pm1p0, &mass_concentration_pm2p5,
			&mass_concentration_pm4p0, &mass_concentration_pm10p0,
			&ambient_humidity, &ambient_temperature, &voc_index, &nox_index);

		ESP_LOGI("", "-------------------------------------");
		if (error) {
			printf("Error executing sen5x_read_measured_values(): %i\n", error);
		} else {
			printf("Mass concentration pm1p0: %.1f 痢/m許n",
					mass_concentration_pm1p0 / 10.0f);
			printf("Mass concentration pm2p5: %.1f 痢/m許n",
				   mass_concentration_pm2p5 / 10.0f);
			printf("Mass concentration pm4p0: %.1f 痢/m許n",
				   mass_concentration_pm4p0 / 10.0f);
			printf("Mass concentration pm10p0: %.1f 痢/m許n",
				   mass_concentration_pm10p0 / 10.0f);
			printf("Ambient humidity: %.1f %%RH\n", ambient_humidity / 100.0f);
			printf("Ambient temperature: %.1f 蚓\n",
				   ambient_temperature / 200.0f);
			printf("Voc index: %.1f\n", voc_index / 10.0f);
			if (nox_index == 0x7fff) {
				printf("Nox index: n/a\n");
			} else {
				printf("Nox index: %.1f\n", nox_index / 10.0f);
			}
		}
	}*/
}
