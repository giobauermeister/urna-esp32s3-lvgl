set(SRC_FILES
    "main.c"
    "keypad.c"
    "lcd_init.c"
    "lvgl_init.c"
    "i2s_init.c"
    "i2c_init.c"
    "ui.c"
    "sound.c"
    "spiffs.c"
    "sd_init.c"
)

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "."
    REQUIRES lvgl esp_lcd spiffs fatfs esp_driver_sdspi
)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_SOURCE_DIR}/include)

target_compile_options(${COMPONENT_LIB} PRIVATE "-std=c++17")