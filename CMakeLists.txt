cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(ts_controller C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME} 
    ts_controller.cpp
#    CounterModule/counter_module_v1.c
    CounterModule/counter_module_v2.c    
)

pico_set_program_name(${PROJECT_NAME} "TS-Controller")
pico_set_program_version(${PROJECT_NAME} "0.2")

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/CounterModule/uart.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/CounterModule/tic_gate.pio)

target_sources(${PROJECT_NAME} PRIVATE ts_controller.cpp
    TDC7200/tdc7200.c
#    CounterModule/counter_module_v1.c
    CounterModule/counter_module_v2.c
    SCPI_Parser/Vrekrer_scpi_parser.cpp
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_stdio
    pico_double
    pico_multicore
    hardware_timer
    hardware_pio
    hardware_spi
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})