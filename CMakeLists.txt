cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_LIST_DIR}/build")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_LIST_DIR}/build/third-party/spdlog")

include_directories(${CMAKE_CURRENT_LIST_DIR}/api)

#############################################################
# spdlog library
#############################################################

set(SPDLOG_PATH ${CMAKE_CURRENT_LIST_DIR}/third-party/spdlog)

include_directories(${SPDLOG_PATH}/include)

add_subdirectory(${SPDLOG_PATH})

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
        ${SRC_DIR}/socket.cc
        ${SRC_DIR}/packet.cc
        ${SRC_DIR}/uri.h
        ${SRC_DIR}/uri.cc
        ${SRC_DIR}/endpoint.h
        ${SRC_DIR}/endpoint.cc
        ${SRC_DIR}/blockwise.h
        ${SRC_DIR}/blockwise.cc
        ${SRC_DIR}/error.cc
)

#target_compile_options(
#    ${LIB_PROJECT_NAME} PRIVATE
#        -Weffc++
#        -Wundef
#)

add_library(
    ${LIB_PROJECT_NAME} STATIC
        ${LIB_SRC_LIST}
)

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
)

add_executable(
    ${TEST_PROJECT_NAME}
        ${TEST_SRC_LIST}
)

target_sources(
    ${TEST_PROJECT_NAME} PRIVATE
        $ENV{GTEST_DIR}/src/gtest-all.cc
)

target_include_directories(
    ${TEST_PROJECT_NAME} PRIVATE
        $ENV{GTEST_DIR}
        $ENV{GTEST_DIR}/include
)

target_link_libraries(
    ${TEST_PROJECT_NAME}
        spdlog
        pthread
        coapcpp
)
