cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O0 -frtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ROOT_DIR}/build/RASPBERRY-PI-4/lib")

set(SPDLOG_PATH ${ROOT_DIR}/third-party/spdlog)
set(WOLFSSL_PATH ${ROOT_DIR}/third-party/wolfssl)
set(CJSON_PATH ${ROOT_DIR}/third-party/cJSON)

install(
        DIRECTORY "data" 
        DESTINATION "${ROOT_DIR}/build/RASPBERRY-PI-4/examples/bin" 
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
        ${COMMON_DIR}/DHT11.cc
        ${COMMON_DIR}/RGB_LED.cc
        ${COMMON_POSIX_DIR}/coap_server.cc
        ${COMMON_POSIX_DIR}/sensor.cc
        ${COMMON_POSIX_DIR}/endpoint.cc
        ${COMMON_DIR}/DHT11.h
        ${COMMON_DIR}/RGB_LED.h
        ${COMMON_POSIX_DIR}/sensor.h
        ${COMMON_POSIX_DIR}/trace.h
        ${COMMON_POSIX_DIR}/coap_server.h
        ${COMMON_POSIX_DIR}/endpoint.h
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
        ${COMMON_POSIX_DIR}
)

target_link_libraries(
    ${PROJECT_NAME}
        coapcpp
        spdlog
        pthread
        wolfssl
        cjson
        wiringPi
        wiringPiDev
)
