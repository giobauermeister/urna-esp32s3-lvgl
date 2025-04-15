# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Parse .env file
set(ENV_FILE "${CMAKE_SOURCE_DIR}/.env")
if(EXISTS ${ENV_FILE})
    file(READ ${ENV_FILE} ENV_CONTENTS)
    string(REPLACE "\n" ";" ENV_LINES "${ENV_CONTENTS}")
    foreach(line ${ENV_LINES})
        if(line MATCHES "^([^#=]+)=(.*)$")
            string(STRIP "${CMAKE_MATCH_1}" KEY)
            string(STRIP "${CMAKE_MATCH_2}" VALUE_RAW)

            # Remove optional surrounding quotes (")
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" VALUE "${VALUE_RAW}")

            # Extra trim (handles \r from Windows files too)
            string(STRIP "${VALUE}" VALUE)

            set(ENV_${KEY} "${VALUE}")
            message(STATUS "Loaded from .env: ${KEY}=${VALUE}")
        endif()
    endforeach()
else()
    message(WARNING ".env file not found!")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Urna)

# Add SPIFFS partition
spiffs_create_partition_image(spiffs spiffs_image FLASH_IN_PROJECT)
