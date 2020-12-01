idf_component_register(
    SRCS
        demo-screens/demo-screen-color-rotate.c
        demo-screens/demo-screen-common.c
        demo-screens/demo-screen-hello-world.c
        demo-screens/demo-screen-voltage.c
        demo-screens/demo-screen-wifi.c
        tasks/task-button.c
        tasks/task-voltage.c
        tasks/task-wifi.c
        ttgo-xy-cp-v1.1-freertos.c
    INCLUDE_DIRS
        include/
    REQUIRES
        lvgl_esp32_drivers
        lvgl_touch
        lvgl_tft
        lvgl
        esp_adc_cal
        nvs_flash
        spi_flash
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)