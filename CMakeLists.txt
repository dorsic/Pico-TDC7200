cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(tdc7200_test)
#project(tdc7200_test C CXX)
#set(CMAKE_C_STANDARD 11)
#set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME} 
    TDC7200/tdc7200.c 
    tdc7200_test.c
)

#pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/test.pio)

target_sources(${PROJECT_NAME} PRIVATE tdc7200_test.c
    TDC7200/tdc7200.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
