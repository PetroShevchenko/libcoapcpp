cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -o0 -frtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ROOT_DIR}/build/lib")

set(SPDLOG_PATH ${ROOT_DIR}/third-party/spdlog)
set(MBEDTLS_PATH ${ROOT_DIR}/third-party/mbedtls)

set(
    PROJECT_NAME
        "tls-client2"
)

set(
    SRC_DIR
        ${CMAKE_CURRENT_LIST_DIR}
)

set(
    SRC_LIST
        ${SRC_DIR}/tls-client2.cc
        ${SRC_DIR}/mbedtls_error.cc
        ${COMMON_PATH}/http_requests.cc
        ${COMMON_PATH}/my_certificates.cc
)

add_executable(
    ${PROJECT_NAME}
        ${SRC_LIST}
)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
        ${ROOT_DIR}/api
        ${ROOT_DIR}/src
        ${ROOT_DIR}/src/unix
        ${SRC_DIR}
        ${SPDLOG_PATH}/include
        ${MBEDTLS_PATH}/include
        ${EXAMPLES_PATH}
)

target_link_libraries(
    ${PROJECT_NAME}
        coapcpp
        spdlog
        pthread
        mbedtls
        mbedx509
        mbedcrypto
)
