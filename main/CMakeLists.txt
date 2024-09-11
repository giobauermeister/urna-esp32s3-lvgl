idf_component_register(
    SRCS "lcd_init.c" "main.c" "lvgl_init.c" "ui.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl esp_lcd
)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_SOURCE_DIR}/include)