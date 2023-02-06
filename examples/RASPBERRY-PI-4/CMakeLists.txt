cmake_minimum_required (VERSION 3.5)

project("examples")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(POSIX_DIR ${CMAKE_CURRENT_LIST_DIR}/../POSIX)
set(COMMON_POSIX_DIR ${POSIX_DIR}/common)
set(RPI4_DIR ${CMAKE_CURRENT_LIST_DIR})
set(COMMON_DIR ${RPI4_DIR}/common)

add_subdirectory(${RPI4_DIR}/coap-server)