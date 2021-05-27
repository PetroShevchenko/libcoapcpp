set(LIBCOAPCPP_ROOT2 ../../../../)

set(
    LIBCOAPCPP_INC2
    ${LIBCOAPCPP_ROOT2}/api
    ${LIBCOAPCPP_ROOT2}/src
)

set(
	COAP_CLIENT_SRC
	"coap-client.cc"
)

idf_component_register(
	SRCS ${COAP_CLIENT_SRC}
    INCLUDE_DIRS "." ${LIBCOAPCPP_INC2}
    REQUIRES console spiffs libcoapcpp
)
