# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_desktop_assistant)

#第一个参数指代分区表中文件夹名
#第二个参数指代源文件夹路径
spiffs_create_partition_image(storage resources FLASH_IN_PROJECT)