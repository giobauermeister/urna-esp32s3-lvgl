set(SRC_FILES
    "main.c"
    "keypad/keypad.c"
    "lcd/lcd_init.c"
    "ui/lvgl_init.c"
    "i2s/i2s_init.c"
    "i2c/i2c_init.c"
    "ui/ui.c"
    "sound/sound.c"
    "storage/spiffs.c"
    "storage/sd_init.c"
    "candidate/candidate.c"
    "wifi/wifi.c"
    # API source files
    "web_api/web_api.c"
    "web_api/handlers/candidate/api_candidate.c"
    "web_api/handlers/party/api_party.c"
    "web_api/handlers/role/api_role.c"
    "web_api/handlers/voting_result/api_voting_result.c"
)

set(UI_ASSETS
    "ui/assets/img_unknown.c"
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
    esp_http_server
)

idf_component_register(
    SRCS ${SRC_FILES} ${UI_ASSETS}
    INCLUDE_DIRS "."
    REQUIRES ${REQ_COMPONENTS}
)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_SOURCE_DIR}/include)

# target_compile_options(${COMPONENT_LIB} PRIVATE "-std=c++17")

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    WIFI_SSID=${ENV_WIFI_SSID}
    WIFI_PASSWORD=${ENV_WIFI_PASSWORD}
)