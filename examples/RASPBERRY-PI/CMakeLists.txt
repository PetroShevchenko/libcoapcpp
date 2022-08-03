cmake_minimum_required (VERSION 3.5)

project("examples")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(EXAMPLES_PATH ${CMAKE_CURRENT_LIST_DIR})
set(COMMON_PATH ${EXAMPLES_PATH}/common)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/udp-server/RPi-400)
