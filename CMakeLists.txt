cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

list(APPEND
    EXTRA_COMPONENT_DIRS
        components/lv_port_esp32/components/lvgl
        components/lv_port_esp32/components/lvgl_esp32_drivers
        components/lv_port_esp32/components/lvgl_esp32_drivers/lvgl_touch
        components/lv_port_esp32/components/lvgl_esp32_drivers/lvgl_tft)

project(ttgo-xy-cp-v1.1-freertos)