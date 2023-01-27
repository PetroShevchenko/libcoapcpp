cmake_minimum_required (VERSION 3.5)

project("examples")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(POSIX_DIR ${CMAKE_CURRENT_LIST_DIR})
set(COMMON_DIR ${POSIX_DIR}/common)

add_subdirectory(${POSIX_DIR}/coap-server)
