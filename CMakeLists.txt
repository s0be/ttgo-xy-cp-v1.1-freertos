cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

execute_process(COMMAND {
    pushd components/esp-mdf > /dev/null
    git describe --always --tags --dirty
    popd > /dev/null
} OUTPUT_VARIABLE MDF_VER)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMDF_VER=\\\"${MDF_VER}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMDF_VER=\\\"${MDF_VER}\\\"")

list(APPEND
    EXTRA_COMPONENT_DIRS
        components/lv_port_esp32/components/lvgl
        components/lv_port_esp32/components/lvgl_esp32_drivers
        components/lv_port_esp32/components/lvgl_esp32_drivers/lvgl_touch
        components/lv_port_esp32/components/lvgl_esp32_drivers/lvgl_tft
	    components/esp-mdf/components
        components/esp-mdf/components/third_party
        )

project(ttgo-xy-cp-v1.1-freertos)
