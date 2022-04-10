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
.PHONY:	all clean clean-libs

include config.make

LIB						+= hal
LIB						+= bsp
LIB						+= freertos
LIB						+= lwip
LIB 					+= coapcpp
LIB 					+= cjson

LD_SCRIPT_PATH 			:= $(PROJECT_PATH)/../script
LD_SCRIPT 				:= STM32F429ZITX_FLASH.ld
MAKE_SCRIPT_PATH		:= $(LD_SCRIPT_PATH)

INCLUDE_PATH 			+= $(PROJECT_PATH)/Core/Inc
INCLUDE_PATH 			+= $(PROJECT_PATH)/Drivers
INCLUDE_PATH 			+= $(PROJECT_PATH)/LwIP/App
INCLUDE_PATH 			+= $(PROJECT_PATH)/LwIP/Target
INCLUDE_PATH 			+= $(PROJECT_PATH)/../common

VPATH 					+= $(PROJECT_PATH)/Core/Src
VPATH 					+= $(PROJECT_PATH)/Core/Startup
VPATH 					+= $(PROJECT_PATH)/Drivers
VPATH 					+= $(PROJECT_PATH)/LwIP/App
VPATH 					+= $(PROJECT_PATH)/LwIP/Target
VPATH 					+= $(PROJECT_PATH)/../common

CMSIS_PATH				:= $(CUBE_PATH)/Drivers/CMSIS
HAL_PATH 				:= $(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver
BSP_PATH				:= $(CUBE_PATH)/Drivers/BSP
RTOS_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/FreeRTOS/Source
LWIP_PATH				:= $(CUBE_PATH)/Middlewares/Third_Party/LwIP
COAPCPP_PATH   			:= $(PROJECT_PATH)/../../..

INCLUDE_PATH			+= $(CMSIS_PATH)/Include
INCLUDE_PATH			+= $(CMSIS_PATH)/Device/ST/$(MCU_SERIES)/Include
INCLUDE_PATH			+= $(HAL_PATH)/Inc
INCLUDE_PATH			+= $(BSP_PATH)/$(BOARD_NAME)
INCLUDE_PATH			+= $(RTOS_PATH)/include
INCLUDE_PATH			+= $(RTOS_PATH)/portable/MemMang
INCLUDE_PATH			+= $(RTOS_PATH)/portable/GCC/ARM_CM4F
INCLUDE_PATH			+= $(RTOS_PATH)/CMSIS_RTOS_V2
INCLUDE_PATH			+= $(RTOS_PATH)/CMSIS_RTOS
INCLUDE_PATH			+= $(LWIP_PATH)/src/include
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/lwip
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/netif
INCLUDE_PATH			+= $(LWIP_PATH)/src/include/netif/ppp
INCLUDE_PATH			+= $(LWIP_PATH)/system
INCLUDE_PATH			+= $(COAPCPP_PATH)
INCLUDE_PATH			+= $(COAPCPP_PATH)/api
INCLUDE_PATH			+= $(COAPCPP_PATH)/src
INCLUDE_PATH			+= $(COAPCPP_PATH)/src/lwip

SRC 					:= startup_stm32f429zitx.s
SRC 					+= main.cc
SRC 					+= syscalls.c
SRC 					+= sysmem.c
SRC 					+= system_stm32f4xx.c
SRC 					+= stm32f4xx_hal_timebase_tim.c
SRC 					+= stm32f4xx_hal_msp.c
SRC 					+= stm32f4xx_it.c
SRC 					+= command.cc
SRC 					+= freertos.c
SRC 					+= log.c
SRC 					+= rtc.c
SRC 					+= udp_server.cc
SRC 					+= lwip.c
SRC 					+= ethernetif.c

LIB						+= stdc++

OPTIMIZE_LEVEL			:= 2
DEBUG_LEVEL				:= gdb

DEFINE					+= USE_HAL_DRIVER
DEFINE					+= $(MCU_DEVICE)
DEFINE					+= $(MCU_CORE)
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
LIB_PATH 				+= $(PATH_TO_LIBS)

LIBFLAGS				+= $(addprefix -L,$(LIB_PATH)) $(addprefix -l,$(LIB))

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

LDFLAGS					:= -T$(LD_SCRIPT_PATH)/$(LD_SCRIPT)
LDFLAGS					+= -Wl,-Map,$(IMAGE).map,--cref -Wl,--gc-sections
LDFLAGS 				+= -Wl,--start-group -lc -lm -Wl,--end-group

OBJECTS					:= $(addprefix $(OBJ_PATH)/,$(patsubst %.c, %.o,$(filter %.c,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.cpp, %.o,$(filter %.cpp,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.cc, %.o,$(filter %.cc,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.s, %.o,$(filter %.s,$(SRC))))
OBJECTS					+= $(addprefix $(OBJ_PATH)/,$(patsubst %.S, %.o,$(filter %.S,$(SRC))))

UPLOADER 				:= openocd
UFLAGS 					:= -f interface/stlink.cfg
UFLAGS 					+= -f target/stm32f4x.cfg
UFLAGS 					+= -c init -c "reset init"
UFLAGS 					+= -c "halt" -c "flash probe 0"
UFLAGS 					+= -c "flash write_image erase $(IMAGE).elf 0x00000000 elf"
UFLAGS 					+= -c shutdown

flash:
	$(UPLOADER) $(UFLAGS)

all: elf bin hex s19 size

bsp:
	make -f $(MAKE_SCRIPT_PATH)/bsp.make all -j$(shell nproc || echo 2)

hal:
	make -f $(MAKE_SCRIPT_PATH)/hal.make all -j$(shell nproc || echo 2)

freertos:
	make -f $(MAKE_SCRIPT_PATH)/freertos.make all -j$(shell nproc || echo 2)

lwip:
	make -f $(MAKE_SCRIPT_PATH)/lwip.make all -j$(shell nproc || echo 2)

cjson:
	make -f $(MAKE_SCRIPT_PATH)/cjson.make all -j$(shell nproc || echo 2)

elf:$(IMAGE).elf

lst:$(IMAGE).lst

bin:$(IMAGE).bin

hex:$(IMAGE).hex

s19:$(IMAGE).s19

size:$(IMAGE).elf
	@echo $@
	$(SIZE) $(IMAGE).elf

$(IMAGE).bin:$(IMAGE).elf
	@echo $@
	$(OBJCOPY) -R .RAM_D3 -O binary $< $@

$(IMAGE).hex:$(IMAGE).elf
	@echo $@
	$(OBJCOPY) -O ihex $< $@

$(IMAGE).s19:$(IMAGE).elf
	@echo $@
	$(OBJCOPY) -O srec $< $@

$(IMAGE).lst:$(IMAGE).elf
	@echo $@
	$(OBJDUMP) -h -S -z $<  > $@

$(IMAGE).elf:$(OBJECTS)
	@echo $@
	$(LD) $(CCFLAGS) $(LDFLAGS) $^ -o $@ $(LIBFLAGS)

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
	rm -f $(IMAGE).bin $(IMAGE).elf $(IMAGE).hex $(IMAGE).s19 $(IMAGE).map $(IMAGE).lst

clean-libs:
	make clean -f $(MAKE_SCRIPT_PATH)/hal.make
	make clean -f $(MAKE_SCRIPT_PATH)/bsp.make
	make clean -f $(MAKE_SCRIPT_PATH)/freertos.make
	make clean -f $(MAKE_SCRIPT_PATH)/lwip.make
	make clean -f $(MAKE_SCRIPT_PATH)/cjson.make
