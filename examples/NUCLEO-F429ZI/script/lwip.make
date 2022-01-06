############################################################################
#
# Copyright (C) 2020 - 2021 Petro Shevchenko <shevchenko.p.i@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################
.PHONY:	all clean

include config.make

LIB_NAME				:= lwip

CMSIS_PATH				:= $(CUBE_PATH)/Drivers/CMSIS
HAL_PATH 				:= $(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver
RTOS_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/FreeRTOS/Source
LWIP_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/LwIP

INCLUDE_PATH			+= $(CMSIS_PATH)/Include
INCLUDE_PATH			+= $(CMSIS_PATH)/Device/ST/$(MCU_SERIES)/Include
INCLUDE_PATH			+= $(HAL_PATH)/Inc

INCLUDE_PATH			+= $(RTOS_PATH)
INCLUDE_PATH			+= $(RTOS_PATH)/include
INCLUDE_PATH			+= $(RTOS_PATH)/portable/MemMang
INCLUDE_PATH			+= $(RTOS_PATH)/portable/GCC/ARM_CM4F
INCLUDE_PATH			+= $(RTOS_PATH)/CMSIS_RTOS_V2
INCLUDE_PATH			+= $(RTOS_PATH)/CMSIS_RTOS

VPATH					+= $(LWIP_PATH)/src/api
VPATH					+= $(LWIP_PATH)/src/netif
VPATH					+= $(LWIP_PATH)/src/netif/ppp
VPATH					+= $(LWIP_PATH)/system/OS
VPATH					+= $(LWIP_PATH)/src/core
VPATH					+= $(LWIP_PATH)/src/core/ipv4
VPATH					+= $(LWIP_PATH)/src/core/ipv6
VPATH					+= $(LWIP_PATH)/src/core/netif
VPATH					+= $(LWIP_PATH)/src/core/netif/ppp
VPATH					+= $(LWIP_PATH)/src/apps/mqtt

INCLUDE_PATH			+= $(VPATH)
INCLUDE_PATH			+= $(LWIP_PATH)/src/include
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/lwip
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/netif
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/netif/ppp
INCLUDE_PATH			+= $(LWIP_PATH)/system
INCLUDE_PATH			+= $(PROJECT_PATH)

SRC_LWIP				:= sys_arch.c
SRC_LWIP				+= auth.c
SRC_LWIP				+= ccp.c
SRC_LWIP				+= chap-md5.c
SRC_LWIP				+= chap-new.c
SRC_LWIP				+= chap_ms.c
SRC_LWIP				+= demand.c
SRC_LWIP				+= eap.c
SRC_LWIP				+= eui64.c
SRC_LWIP				+= fsm.c
SRC_LWIP				+= ipcp.c
SRC_LWIP				+= ipv6cp.c
SRC_LWIP				+= lcp.c
SRC_LWIP				+= magic.c
SRC_LWIP				+= mppe.c
SRC_LWIP				+= multilink.c
SRC_LWIP				+= ppp.c
SRC_LWIP				+= pppapi.c
SRC_LWIP				+= pppcrypt.c
SRC_LWIP				+= pppoe.c
SRC_LWIP				+= pppol2tp.c
SRC_LWIP				+= pppos.c
SRC_LWIP				+= upap.c
SRC_LWIP				+= utils.c
SRC_LWIP				+= vj.c
SRC_LWIP				+= bridgeif.c
SRC_LWIP				+= bridgeif_fdb.c
SRC_LWIP				+= ethernet.c
SRC_LWIP				+= lowpan6.c
SRC_LWIP				+= lowpan6_ble.c
SRC_LWIP				+= lowpan6_common.c
SRC_LWIP				+= slipif.c
SRC_LWIP				+= zepif.c
SRC_LWIP				+= dhcp6.c
SRC_LWIP				+= ethip6.c
SRC_LWIP				+= icmp6.c
SRC_LWIP				+= inet6.c
SRC_LWIP				+= ip6.c
SRC_LWIP				+= ip6_addr.c
SRC_LWIP				+= ip6_frag.c
SRC_LWIP				+= mld6.c
SRC_LWIP				+= nd6.c
SRC_LWIP				+= autoip.c
SRC_LWIP				+= dhcp.c
SRC_LWIP				+= etharp.c
SRC_LWIP				+= icmp.c
SRC_LWIP				+= igmp.c
SRC_LWIP				+= ip4.c
SRC_LWIP				+= ip4_addr.c
SRC_LWIP				+= ip4_frag.c
SRC_LWIP				+= altcp.c
SRC_LWIP				+= altcp_alloc.c
SRC_LWIP				+= altcp_tcp.c
SRC_LWIP				+= def.c
SRC_LWIP				+= dns.c
SRC_LWIP				+= inet_chksum.c
SRC_LWIP				+= init.c
SRC_LWIP				+= ip.c
SRC_LWIP				+= mem.c
SRC_LWIP				+= memp.c
SRC_LWIP				+= netif.c
SRC_LWIP				+= pbuf.c
SRC_LWIP				+= raw.c
SRC_LWIP				+= stats.c
SRC_LWIP				+= sys.c
SRC_LWIP				+= tcp.c
SRC_LWIP				+= tcp_in.c
SRC_LWIP				+= tcp_out.c
SRC_LWIP				+= timeouts.c
SRC_LWIP				+= udp.c
SRC_LWIP				+= mqtt.c
SRC_LWIP				+= api_lib.c
SRC_LWIP				+= api_msg.c
SRC_LWIP				+= err.c
SRC_LWIP				+= if_api.c
SRC_LWIP				+= netbuf.c
SRC_LWIP				+= netdb.c
SRC_LWIP				+= netifapi.c
SRC_LWIP				+= sockets.c
SRC_LWIP				+= tcpip.c

SRC 					:= $(SRC_LWIP)

OPTIMIZE_LEVEL			:= 2
DEBUG_LEVEL				:= gdb

DEFINE					:= USE_HAL_DRIVER
DEFINE					+= $(MCU_DEVICE)
DEFINE					+= $(MCU_CORE)
DEFINE 					+= $(BOARD_NAME)

CCFLAGS 				:= -mcpu=cortex-m4
CCFLAGS 				+= -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CCFLAGS 				+= -specs=nosys.specs
#CCFLAGS 				+= -specs=nano.specs

CROSS_PREFIX			:= arm-none-eabi-

ROOT					:= $(shell pwd)

CC						:= $(CROSS_PREFIX)gcc
AS						:= $(CROSS_PREFIX)g++
LD						:= $(CROSS_PREFIX)g++
CPPC					:= $(CROSS_PREFIX)g++
AR						:= $(CROSS_PREFIX)ar
RANLIB					:= $(CROSS_PREFIX)ranlib
SIZE					:= $(CROSS_PREFIX)size
OBJCOPY					:= $(CROSS_PREFIX)objcopy
OBJDUMP					:= $(CROSS_PREFIX)objdump

INCLUDE_PATH			+= $(TOOL_PATH)/include
LIB_PATH				+= $(TOOL_PATH)/lib

LIBFLAGS				+= $(addprefix -L,$(LIB_PATH)) $(addprefix -l,$(LIB))
ARFLAGS					:= rcs

CCFLAGS					+= -Wall
CCFLAGS					+= $(addprefix -D,$(DEFINE)) $(addprefix -I,$(INCLUDE_PATH))
CCFLAGS					+= -ffunction-sections -fdata-sections

CONLYFLAGS				+= -std=gnu11

ASFLAGS					+= -Wall
ASFLAGS					+= $(addprefix -D,$(DEFINE)) $(addprefix -I,$(INCLUDE_PATH))

ifeq ($(VERSION),Debug)
CCFLAGS					+= -g$(DEBUG_LEVEL) -O0 -DDEBUG
ASFLAGS					+= -g$(DEBUG_LEVEL) -O0 -DDEBUG
endif

ifeq ($(VERSION),Release)
CCFLAGS					+= -O$(OPTIMIZE_LEVEL)
ASFLAGS					+= -O0
endif

CPPCFLAGS				+= $(CCFLAGS)
CPPCFLAGS				+= -Weffc++ -Wextra -Wpedantic -Wshadow -Wundef -Wno-missing-field-initializers
CPPCFLAGS				+= -std=c++11

OBJECTS					:= $(addprefix $(OBJ_PATH)/,$(patsubst %.c, %.o,$(filter %.c,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.cpp, %.o,$(filter %.cpp,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.cc, %.o,$(filter %.cc,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.s, %.o,$(filter %.s,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.S, %.o,$(filter %.S,$(SRC))))

LIBRARY 				:= $(BIN_LIB_PATH)/lib$(LIB_NAME).a

all: lib size

lib: $(LIBRARY)
	$(RANLIB) $(LIBRARY)

lst:$(IMAGE).lst

$(IMAGE).lst: $(LIBRARY)
	@echo $@
	$(OBJDUMP) -D $< > $@

size: $(LIBRARY)
	@echo $@
	$(SIZE) $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	@echo $@
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_PATH)/%.o:%.c
	@echo $<
	$(CC) $(CCFLAGS) $(CONLYFLAGS) -MD -c $< -o $@

$(OBJ_PATH)/%.o:%.cpp
	$(CPPC) $(CPPCFLAGS) -MD -c $< -o $@

$(OBJ_PATH)/%.o:%.cc
	@echo $<
	$(CPPC) $(CPPCFLAGS) -MD -c $< -o $@

$(OBJ_PATH)/%.o:%.s
	$(AS) $(ASFLAGS) -c $< -o $@

$(OBJ_PATH)/%.o:%.S
	@echo $<
	$(AS) $(ASFLAGS) -c $< -o $@

include $(wildcard $(OBJ_PATH)/*.d)

clean:
	rm -f $(OBJECTS)
	rm -f $(patsubst %.o, %.d,$(OBJECTS))
	rm -f $(LIBRARY) $(IMAGE).lst
