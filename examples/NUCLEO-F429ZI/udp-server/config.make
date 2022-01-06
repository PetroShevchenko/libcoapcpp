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

VERSION					:= Debug
#VERSION				:= Release

PROJECT_NAME			:= udp-server
BOARD_NAME 				:= STM32F4xx_Nucleo_144

MCU_CORE 				:= CORE_CM4
MCU_SERIES				:= STM32F4xx
MCU_DEVICE 				:= STM32F429xx
DEFINE 					:= USE_LOG

PROJECT_PATH			:= .
CUBE_PATH				:= $(PROJECT_PATH)/../../../third-party/STM32CubeF4

TOOL_PATH				:= $(HOME)/gcc-arm-none-eabi-10.3-2021.07

BUILD_PATH				:= $(PROJECT_PATH)/../../../build/$(BOARD_NAME)
PATH_TO_LIBS			:= $(BUILD_PATH)/common-libraries/lib
BIN_PATH 				:= $(BUILD_PATH)/$(PROJECT_NAME)
OBJ_PATH				:= $(BIN_PATH)/obj
IMAGE					:= $(BIN_PATH)/$(PROJECT_NAME)
