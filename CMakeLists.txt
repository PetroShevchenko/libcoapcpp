cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Weffc++ -Wextra -Wpedantic -Wshadow -Wundef -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

include_directories(${CMAKE_CURRENT_LIST_DIR}/api)

set(
    LIB_PROJECT_NAME
        "coapcpp"
)

set(
    SRC_DIR
        ${CMAKE_CURRENT_LIST_DIR}/src
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

project (${LIB_PROJECT_NAME})

add_library(${LIB_PROJECT_NAME} STATIC ${LIB_SRC_LIST})
