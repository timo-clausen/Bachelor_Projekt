/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "include/sensirion_i2c_hal.h"
#include "include/sensirion_common.h"
#include "include/sensirion_config.h"

#include "driver/i2c.h"
#include "freertos/task.h"
#include "hal/i2c_hal.h"

#include "esp_log.h"



#define I2C_MASTER_TX_BUF_DISABLE 	0 					/*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 	0 					/*!< I2C master doesn't need buffer */
#define I2C_MASTER_FREQ_HZ 			10000			    /*!< I2C master clock frequency */
#define WRITE_BIT 					I2C_MASTER_WRITE	/*!< I2C master write */
#define READ_BIT 					I2C_MASTER_READ		/*!< I2C master read */
#define ACK_CHECK_EN 				0x1            		/*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 				0x0           		/*!< I2C master will not check ack from slave */
#define ACK_VAL 					0x0                 /*!< I2C ack value */
#define NACK_VAL 					0x1                	/*!< I2C nack value */
#define LAST_NACK_VAL				0x2					/*!< I2C last nack value*/

static gpio_num_t i2c_gpio_sda = 38; //33;
static gpio_num_t i2c_gpio_scl = 37;  //22;
//static uint32_t i2c_frequency = 100000;
static i2c_port_t i2c_port = I2C_NUM_0;




/**************************************************************************************************************/
/*										defines from driver/i2c.c											  */
/**************************************************************************************************************/
#define I2C_CONTEX_INIT_DEF(uart_num) {\
    .hal.dev = I2C_LL_GET_HW(uart_num),\
    .spinlock = portMUX_INITIALIZER_UNLOCKED,\
    .hw_enabled = false,\
}

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





/*
 * INSTRUCTIONS
 * ============
 *
 * Implement all functions where they are marked as IMPLEMENT.
 * Follow the function specification in the comments.
 */

/**
 * Select the current i2c bus by index.
 * All following i2c operations will be directed at that bus.
 *
 * THE IMPLEMENTATION IS OPTIONAL ON SINGLE-BUS SETUPS (all sensors on the same
 * bus)
 *
 * @param bus_idx   Bus index to select
 * @returns         0 on success, an error code otherwise
 */
int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx) {
    /* TODO:IMPLEMENT or leave empty if all sensors are located on one single
     * bus
     */
    return NOT_IMPLEMENTED_ERROR;
}

/**
 * Initialize all hard- and software components that are needed for the I2C
 * communication.
 */
void sensirion_i2c_hal_init(void) {

	// Using default configuration
	i2c_config_t config = {
    		.mode = I2C_MODE_MASTER,
			.sda_io_num = i2c_gpio_sda,
			.scl_io_num = i2c_gpio_scl,
			.sda_pullup_en = GPIO_PULLUP_ENABLE, 	//
			.scl_pullup_en = GPIO_PULLUP_ENABLE,
			.master.clk_speed = I2C_MASTER_FREQ_HZ,
			.clk_flags = I2C_SCLK_APB,
    };

	i2c_param_config(i2c_port, & config);

	i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

	//i2c_hal_set_fifo_mode(&(i2c_context[i2c_port].hal),0);

	return;
}

/**
 * Release all resources initialized by sensirion_i2c_hal_init().
 */
void sensirion_i2c_hal_free(void) {

	i2c_driver_delete(i2c_port);

	return;
}

/**
 * Execute one read transaction on the I2C bus, reading a given number of bytes.
 * If the device does not acknowledge the read command, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to read from
 * @param data    pointer to the buffer where the data is to be stored
 * @param count   number of bytes to read from I2C and store in the buffer
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint16_t count) {



	esp_err_t err_code;
	int8_t err_val;


//	err_code = i2c_reset_rx_fifo(i2c_port);
//
//	printf("Reset rx fifo error code: %X \n", err_code);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd, data, (count-1), ACK_VAL);
	i2c_master_read_byte(cmd, data + (sizeof(uint8_t))*(count-1), NACK_VAL);

//	for(uint8_t i = 0; i<(count-1); i++)
//	{
//		i2c_master_read_byte(cmd, data++, ACK_VAL);
//	}
//
//	i2c_master_read_byte(cmd, data, NACK_VAL);


	i2c_master_stop(cmd);
	err_code = i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	ESP_LOGI("I2C", "err_code: %d", err_code);

	switch(err_code)
	{
		case ESP_OK:
			err_val = NO_ERROR;							// sensirion common define
			break;
		case ESP_ERR_INVALID_ARG:
			err_val = NOT_IMPLEMENTED_ERROR;			// sensirion common define
			break;
		default:
			err_val = NOT_IMPLEMENTED_ERROR;			// sensirion common define
			break;
	}


/*
	uint16_t count_copy = count;
	for(uint8_t i = 1; i<=count_copy; i++)
	{
		printf(" %X", data[i-1]);
		if (i%3 == 0)
		{
			printf("\n");
		}
	}*/
	return err_val;
}

/**
 * Execute one write transaction on the I2C bus, sending a given number of
 * bytes. The bytes in the supplied buffer must be sent to the given address. If
 * the slave device does not acknowledge any of the bytes, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to write to
 * @param data    pointer to the buffer containing the data to write
 * @param count   number of bytes to read from the buffer and send over I2C
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data,
                               uint16_t count) {


    esp_err_t err_code;
	int8_t err_val;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);			// TODO:
	i2c_master_write(cmd, data, count, ACK_VAL);									// TODO:
 	i2c_master_stop(cmd);
	err_code = i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	switch(err_code)
	{
		case ESP_OK:
			err_val = NO_ERROR;							// sensirion common define
			break;
		case ESP_ERR_INVALID_ARG:
			err_val = NOT_IMPLEMENTED_ERROR;			// sensirion common define
			break;
		default:
			err_val = NOT_IMPLEMENTED_ERROR;			// sensirion common define
			break;
	}

	return err_val;
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution for at least the given time, but may also sleep longer.
 *
 * Despite the unit, a <10 millisecond precision is sufficient.
 *
 * @param useconds the sleep time in microseconds
 */
void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {

	// Converting micro [s] into milli [s]
	vTaskDelay((useconds/1000) / portTICK_PERIOD_MS);
	return;
}
