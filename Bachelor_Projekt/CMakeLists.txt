
set(PROJECT_VER "1.0.4")
# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# (Not part of the boilerplate)
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Bachelor_Projekt)

target_add_binary_data(Bachelor_Projekt.elf "server_certs/hivemq-server-cert.pem" TEXT)
target_add_binary_data(Bachelor_Projekt.elf "server_certs/mqtt-client-cert-2.pem" TEXT)
target_add_binary_data(Bachelor_Projekt.elf "server_certs/mqtt-client-key-2.pem" TEXT)
target_add_binary_data(Bachelor_Projekt.elf "server_certs/ca_cert.pem" TEXT)