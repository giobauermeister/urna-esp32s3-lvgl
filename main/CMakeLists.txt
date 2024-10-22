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
    "candidate.c"
    "wifi.c"
    "img_unknown.c"
    "img_no_photo.c"   
)

set(REQ_COMPONENTS
    lvgl
    esp_lcd
    spiffs
    esp_driver_sdspi
    fatfs
    json
    nvs_flash
    esp_wifi
)

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "."
    REQUIRES ${REQ_COMPONENTS}
)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_SOURCE_DIR}/include)

target_compile_options(${COMPONENT_LIB} PRIVATE "-std=c++17")