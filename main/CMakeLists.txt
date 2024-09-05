idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_SOURCE_DIR}/../include/gui)