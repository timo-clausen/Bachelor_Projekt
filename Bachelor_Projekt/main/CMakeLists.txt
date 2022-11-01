# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "my_wifi.c" "my_mqtt.c" "device_control.c" "json_parser.c" "measurements.c" "ota_update.c" "touch_input.c" "enviromental_sensor.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
