cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gpio_box)

# This command tells ESP-IDF to generate a SPIFFS partition image from `spiffs_data`
spiffs_create_partition_image(storage spiffs_data FLASH_IN_PROJECT)