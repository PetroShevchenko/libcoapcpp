cmake_minimum_required (VERSION 3.5)

project("fuzzer" CXX)

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "libFuzzer requires clang, configure with -DCMAKE_CXX_COMPILER=clang++")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
set(SRC_DIR ${ROOT_DIR}/src)
set(INC_DIR ${ROOT_DIR}/api)
set(CJSON_PATH ${ROOT_DIR}/third-party/cJSON)

# The library sources are compiled straight into every target: they have to be instrumented,
# and the root project would bring in wolfSSL, mbedTLS, spdlog and the unit tests.
# NDEBUG is not defined on purpose, internal invariants stay checked.
set(
    FUZZ_FLAGS
        -fsanitize=fuzzer,address,undefined
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
        -g
        -O1
)

#############################################################
# fuzz_packet
#############################################################

add_executable(
    fuzz_packet
        ${CMAKE_CURRENT_LIST_DIR}/fuzz_packet.cc
        ${SRC_DIR}/packet.cc
        ${SRC_DIR}/error.cc
)

target_include_directories(
    fuzz_packet PRIVATE
        ${INC_DIR}
        ${CJSON_PATH}
)

target_compile_options(
    fuzz_packet PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
        ${FUZZ_FLAGS}
)

target_link_libraries(
    fuzz_packet PRIVATE
        ${FUZZ_FLAGS}
)
