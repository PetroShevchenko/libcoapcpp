set(LIBCOAPCPP_ROOT ../../../../../)

set(
    LIBCOAPCPP_INC
    ${LIBCOAPCPP_ROOT}/api
    ${LIBCOAPCPP_ROOT}/src
)

set(
    LIBCOAPCPP_SRC
    ${LIBCOAPCPP_ROOT}/src/utils.cc
    ${LIBCOAPCPP_ROOT}/src/uri.cc
    ${LIBCOAPCPP_ROOT}/src/packet.cc
    ${LIBCOAPCPP_ROOT}/src/endpoint.cc
    ${LIBCOAPCPP_ROOT}/src/blockwise.cc
)

idf_component_register(
    SRCS ${LIBCOAPCPP_SRC}
    INCLUDE_DIRS ${LIBCOAPCPP_INC}
)