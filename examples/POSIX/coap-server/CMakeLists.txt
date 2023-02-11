cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O0 -frtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ROOT_DIR}/build/POSIX/lib")

set(SPDLOG_PATH ${ROOT_DIR}/third-party/spdlog)
set(WOLFSSL_PATH ${ROOT_DIR}/third-party/wolfssl)
set(CJSON_PATH ${ROOT_DIR}/third-party/cJSON)

install(
        DIRECTORY "data" 
        DESTINATION "${ROOT_DIR}/build/POSIX/examples/bin" 
        FILES_MATCHING PATTERN "*"
    )

set(
    PROJECT_NAME
        "coap-server"
)

set(
    SRC_DIR
        ${CMAKE_CURRENT_LIST_DIR}
)

set(
    SRC_LIST
        ${SRC_DIR}/main.cc
        ${COMMON_DIR}/coap_server.cc
        ${COMMON_DIR}/sensor.cc
        ${COMMON_DIR}/sensor_stubs.cc
        ${COMMON_DIR}/endpoint.cc
        ${COMMON_DIR}/packet_helper.cc
        ${COMMON_DIR}/sensor.h
        ${COMMON_DIR}/sensor_stubs.h
        ${COMMON_DIR}/trace.h
        ${COMMON_DIR}/coap_server.h
        ${COMMON_DIR}/endpoint.h
        ${COMMON_DIR}/packet_helper.h
)

add_definitions(-DTRACE_CODE)

add_executable(
    ${PROJECT_NAME}
        ${SRC_LIST}
)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
        ${ROOT_DIR}/api
        ${ROOT_DIR}/src
        ${SRC_DIR}
        ${SPDLOG_PATH}/include
        ${EXAMPLES_PATH}
        ${WOLFSSL_PATH}
        ${CJSON_PATH}
        ${COMMON_DIR}
)

target_link_libraries(
    ${PROJECT_NAME}
        coapcpp
        spdlog
        pthread
        wolfssl
        cjson
)
