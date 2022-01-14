cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -o0 -frtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_BINARY_DIR}/lib")

remove_definitions(-DNDEBUG)

include_directories(${CMAKE_CURRENT_LIST_DIR}/api)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#############################################################
# spdlog library
#############################################################

set(SPDLOG_PATH ${CMAKE_CURRENT_LIST_DIR}/third-party/spdlog)

include_directories(${SPDLOG_PATH}/include)

add_subdirectory(${SPDLOG_PATH})

#############################################################
# wolfssl library
#############################################################

set(WOLFSSL_PATH ${CMAKE_CURRENT_LIST_DIR}/third-party/wolfssl)

include_directories(${WOLFSSL_PATH}/wolfssl)

add_subdirectory(${WOLFSSL_PATH})

#############################################################
# mbed TLS library
#############################################################

set(MBEDTLS_PATH ${CMAKE_CURRENT_LIST_DIR}/third-party/mbedtls)

include_directories(${MBEDTLS_PATH}/mbedtls)

add_subdirectory(${MBEDTLS_PATH})

#############################################################
# coapcpp library
#############################################################

set(
    LIB_PROJECT_NAME
        "coapcpp"
)

project (${LIB_PROJECT_NAME})

set(
    SRC_DIR
        ${CMAKE_CURRENT_LIST_DIR}/src
)

set(
    INC_DIR
        ${CMAKE_CURRENT_LIST_DIR}/api
)

set(
    LIB_SRC_LIST
        ${SRC_DIR}/packet.cc
        ${SRC_DIR}/uri.h
        ${SRC_DIR}/uri.cc
        ${SRC_DIR}/endpoint.h
        ${SRC_DIR}/endpoint.cc
        ${SRC_DIR}/blockwise.h
        ${SRC_DIR}/blockwise.cc
        ${SRC_DIR}/error.cc
        ${SRC_DIR}/utils.cc
        ${SRC_DIR}/wolfssl_error.cc
        ${SRC_DIR}/unix/unix_socket.cc
        ${SRC_DIR}/unix/unix_dns_resolver.cc
        ${SRC_DIR}/unix/unix_connection.cc
        ${SRC_DIR}/unix/unix_udp_client.cc
        ${SRC_DIR}/unix/unix_dtls_client.cc
)

add_library(
    ${LIB_PROJECT_NAME} STATIC
        ${LIB_SRC_LIST}
)

target_include_directories(
    ${LIB_PROJECT_NAME} PRIVATE
        ${INC_DIR}
        ${SRC_DIR}
        ${SRC_DIR}/unix
        ${WOLFSSL_PATH}
)

target_compile_definitions(
    ${LIB_PROJECT_NAME} PRIVATE
        USE_SPDLOG
)

#target_compile_options(
#    ${LIB_PROJECT_NAME} PRIVATE
#        -Weffc++
#        -Wundef
#)

#############################################################
# unit tests
#############################################################

set(
    TEST_PROJECT_NAME
        "test_coapcpp"
)

set(
    TEST_DIR
        ${CMAKE_CURRENT_LIST_DIR}/test
)

set(
    TEST_SRC_LIST
       ${INC_DIR}/consts.h
       ${INC_DIR}/error.h
       ${INC_DIR}/packet.h
       ${TEST_DIR}/test_packet.cc
       ${TEST_DIR}/test_uri.cc
       ${TEST_DIR}/test_dns_resolver.cc
       ${TEST_DIR}/test_socket.cc
       ${TEST_DIR}/test_blockwise.cc
       ${TEST_DIR}/test_common.cc
)

add_executable(
    ${TEST_PROJECT_NAME}
        ${TEST_SRC_LIST}
)

target_compile_definitions(
    ${TEST_PROJECT_NAME} PRIVATE
        PRINT_TESTED_VALUES
)

target_sources(
    ${TEST_PROJECT_NAME} PRIVATE
        $ENV{GTEST_DIR}/src/gtest-all.cc
)

target_include_directories(
    ${TEST_PROJECT_NAME} PRIVATE
        $ENV{GTEST_DIR}
        $ENV{GTEST_DIR}/include
        ${INC_DIR}
        ${SRC_DIR}
        ${SRC_DIR}/unix
)

target_link_libraries(
    ${TEST_PROJECT_NAME}
        coapcpp
        spdlog
        pthread
)
