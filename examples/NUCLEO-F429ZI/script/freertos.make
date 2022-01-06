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

LIB_NAME				:= freertos

CMSIS_PATH				:= $(CUBE_PATH)/Drivers/CMSIS
HAL_PATH 				:= $(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver

INCLUDE_PATH			+= $(CMSIS_PATH)/Include
INCLUDE_PATH			+= $(CMSIS_PATH)/Device/ST/$(MCU_SERIES)/Include
INCLUDE_PATH			+= $(HAL_PATH)/Inc

RTOS_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/FreeRTOS/Source

VPATH					+= $(RTOS_PATH)
VPATH					+= $(RTOS_PATH)/portable/MemMang
VPATH					+= $(RTOS_PATH)/portable/GCC/ARM_CM4F
VPATH					+= $(RTOS_PATH)/CMSIS_RTOS_V2
VPATH					+= $(RTOS_PATH)/CMSIS_RTOS

INCLUDE_PATH			+= $(VPATH)
INCLUDE_PATH			+= $(RTOS_PATH)/include
INCLUDE_PATH			+= $(PROJECT_PATH)

SRC_RTOS				:= heap_4.c
SRC_RTOS				+= port.c
#SRC_RTOS				+= cmsis_os.c
SRC_RTOS				+= cmsis_os2.c
SRC_RTOS				+= croutine.c
SRC_RTOS				+= event_groups.c
SRC_RTOS				+= list.c
SRC_RTOS				+= queue.c
SRC_RTOS				+= stream_buffer.c
SRC_RTOS				+= tasks.c
SRC_RTOS				+= timers.c

SRC 					:= $(SRC_RTOS)

OPTIMIZE_LEVEL			:= 2
DEBUG_LEVEL				:= gdb

DEFINE					:= USE_HAL_DRIVER
DEFINE					+= $(MCU_DEVICE)
DEFINE					+= $(MCU_CORE)
DEFINE 					+= $(BOARD_NAME)

CCFLAGS 				:= -mcpu=cortex-m4
CCFLAGS 				+= -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
#CCFLAGS 				+= -specs=nano.specs
CCFLAGS 				+= -specs=nosys.specs

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
