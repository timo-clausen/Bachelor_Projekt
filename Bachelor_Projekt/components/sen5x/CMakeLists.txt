# Register the component
idf_component_register(SRCS "sen5x_i2c.c" "sensirion_common.c" "sensirion_i2c_hal.c" "sensirion_i2c.c"
					INCLUDE_DIRS "include"
					REQUIRES driver )

# Set values of hypothetical variables that control the build of `foo`
#set(FOO_BUILD_STATIC OFF)
#set(FOO_BUILD_TESTS OFF)

# Create and import the library targets
#add_subdirectory(foo)

# Publicly link `foo` to `main` component
#target_link_libraries(main PUBLIC foo)


#add_library(foo bar.c fizz.cpp buzz.cpp)

#if(ESP_PLATFORM)
  # On ESP-IDF, bar.c needs to include esp_spi_flash.h from the spi_flash component
  #target_link_libraries(foo PRIVATE idf::spi_flash)
#endif()