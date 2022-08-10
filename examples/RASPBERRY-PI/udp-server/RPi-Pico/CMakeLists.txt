cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME udp-server)
set(PROJECT_NAME_STR "udp-server")
set(PROJECT_VERSION_STR "0.1")

set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../script)

set(
        SRC_LIST
                ${SRC_DIR}/ADXL345.cc
	        ${SRC_DIR}/pico_protocol.cc
                ${SRC_DIR}/main.cc
                ${SRC_DIR}/error.cc
)

set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})

include(${SCRIPT_DIR}/pico_sdk_import.cmake)
include(${SCRIPT_DIR}/FreeRTOS_Kernel_import.cmake)

project(${PROJECT_NAME} C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME} "${SRC_LIST}")

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME_STR})
pico_set_program_version(${PROJECT_NAME} ${PROJECT_VERSION_STR})

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

target_include_directories(
	${PROJECT_NAME} PRIVATE
                ${INC_DIR}
                ${FREERTOS_KERNEL_PATH}/include
                ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
)

target_link_libraries(${PROJECT_NAME} pico_stdlib)

target_link_libraries(
	${PROJECT_NAME}
	        hardware_spi
                hardware_i2c
                hardware_dma
                hardware_pio
                FreeRTOS-Kernel
                FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(${PROJECT_NAME})
