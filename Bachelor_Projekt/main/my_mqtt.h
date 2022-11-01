/*
 * my_mqtt.h
 *
 *  Created on: 25.10.2021
 *      Author: TiC
 */

void mqtt_app_start(void);
void mqtt_publish_status(const char *data);
void mqtt_publish_db(const char *data);
void mqtt_publish_enviromental_sensor_data(const char *data);
