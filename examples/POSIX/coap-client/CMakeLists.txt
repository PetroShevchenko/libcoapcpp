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

set(
    PROJECT_NAME
        "coap-client"
)

set(
    SRC_DIR
        ${CMAKE_CURRENT_LIST_DIR}
)

set(
    SRC_LIST
        ${SRC_DIR}/main.cc
        ${COMMON_DIR}/coap_client.cc
        ${COMMON_DIR}/coap_client.h
        ${COMMON_DIR}/trace.h
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
        ${COMMON_DIR}
        ${SPDLOG_PATH}/include
        ${EXAMPLES_PATH}
        ${WOLFSSL_PATH}
        ${CJSON_PATH}
)

target_link_libraries(
    ${PROJECT_NAME}
        coapcpp
        spdlog
        pthread
        wolfssl
        cjson
)
