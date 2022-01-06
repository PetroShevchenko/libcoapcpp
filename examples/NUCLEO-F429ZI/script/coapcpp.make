############################################################################
#
# Copyright (C) 2020 - 2022 Petro Shevchenko <shevchenko.p.i@gmail.com>
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

LIB_NAME				:= coapcpp

CMSIS_PATH				:= $(CUBE_PATH)/Drivers/CMSIS
HAL_PATH 				:= $(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver
RTOS_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/FreeRTOS/Source
LWIP_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/LwIP
COAPCPP_PATH   			:= $(PROJECT_PATH)/../../..

INCLUDE_PATH			+= $(CMSIS_PATH)/Include
INCLUDE_PATH			+= $(CMSIS_PATH)/Device/ST/$(MCU_SERIES)/Include
INCLUDE_PATH			+= $(HAL_PATH)/Inc
INCLUDE_PATH			+= $(COAPCPP_PATH)/api
INCLUDE_PATH			+= $(COAPCPP_PATH)/src
INCLUDE_PATH			+= $(COAPCPP_PATH)/src/lwip

INCLUDE_PATH			+= $(RTOS_PATH)
INCLUDE_PATH			+= $(RTOS_PATH)/include
INCLUDE_PATH			+= $(RTOS_PATH)/portable/MemMang
INCLUDE_PATH			+= $(RTOS_PATH)/portable/GCC/ARM_CM4F
INCLUDE_PATH			+= $(RTOS_PATH)/CMSIS_RTOS_V2
INCLUDE_PATH			+= $(RTOS_PATH)/CMSIS_RTOS

VPATH					+= $(COAPCPP_PATH)/src
VPATH					+= $(COAPCPP_PATH)/src/lwip

INCLUDE_PATH			+= $(VPATH)
INCLUDE_PATH			+= $(LWIP_PATH)/src/include
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/lwip
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/netif
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/netif/ppp
INCLUDE_PATH			+= $(LWIP_PATH)/system
INCLUDE_PATH			+= $(PROJECT_PATH)
INCLUDE_PATH			+= $(COAPCPP_PATH) 

SRC_COAPCPP				:= blockwise.cc
SRC_COAPCPP				+= endpoint.cc
SRC_COAPCPP				+= error.cc
SRC_COAPCPP				+= packet.cc
SRC_COAPCPP				+= uri.cc
SRC_COAPCPP				+= utils.cc
SRC_COAPCPP				+= lwip_dns_resolver.cc
SRC_COAPCPP				+= lwip_socket.cc

SRC 					:= $(SRC_COAPCPP)

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
