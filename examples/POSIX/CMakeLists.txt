cmake_minimum_required (VERSION 3.5)

project("examples")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(EXAMPLES_PATH ${CMAKE_CURRENT_LIST_DIR})
set(COMMON_PATH ${EXAMPLES_PATH}/common)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tcp-client)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tls-client1)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tls-client2)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/udp-echo-server)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/udp-client)