idf_component_register(
    SRCS "lcd_init.c" "main.c" "lvgl_init.c" "ui.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl esp_lcd
)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_SOURCE_DIR}/include)

target_compile_options(${COMPONENT_LIB} PRIVATE "-std=c++17")