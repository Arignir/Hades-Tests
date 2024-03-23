################################################################################
##
##  This file is part of the Hades GBA Emulator, and is made available under
##  the terms of the GNU General Public License version 2.
##
##  Copyright (C) 2021-2024 - The Hades Authors
##
################################################################################

# Verbosity
Q		:= @

# Source & Target directory
SOURCE_DIR	:= $(realpath $(shell pwd))/source/
BUILD_DIR	:= $(realpath $(shell pwd))/build/
ROMS_DIR	:= $(realpath $(shell pwd))/roms/

# Targets
TARGETS 	:= \
		$(ROMS_DIR)/dma-start-delay.gba \
		$(ROMS_DIR)/dma-latch.gba \
		$(ROMS_DIR)/bios-openbus.gba \
		$(ROMS_DIR)/timer-basic.gba \

PATH		:= $(DEVKITARM)/bin:$(PATH)
LIBGBA		:= $(DEVKITPRO)/libgba
CC		:= arm-none-eabi-gcc
LD		:= arm-none-eabi-gcc
OBJCOPY		:= arm-none-eabi-objcopy

# Compiler flags
CFLAGS		:= \
		-Wall \
		-Wextra \
		-std=gnu17 \
		-fms-extensions \
		-fomit-frame-pointer\
		-mcpu=arm7tdmi \
		-mtune=arm7tdmi \
		-I$(LIBGBA)/include \
		-Iinclude

# Linker flags
LDFLAGS		:= \
		-specs=gba.specs \
		-lmm -lgba \
		-L$(LIBGBA)/lib


all: $(TARGETS)

$(ROMS_DIR)/%.gba: $(BUILD_DIR)/%.o
	$(Q)echo "  LD $(shell basename $@)"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(LD) $^ $(LDFLAGS) -o $@
	$(Q)$(OBJCOPY) -O binary $@
	$(Q)gbafix -t"HADES TESTS" -cHDS $@ > /dev/null

-include $(DEP)
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(Q)echo "  CC $(shell basename $<)"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -MD -c $< -o $@

clean:
	$(Q)echo "  RM $(shell basename $(BUILD_DIR))"
	$(Q)rm -rf $(BUILD_DIR)
	$(Q)echo "  RM $(shell basename $(ROMS_DIR))"
	$(Q)rm -rf $(ROMS_DIR)

re: clean
	$(Q)$(MAKE) --no-print-directory all

.PHONY: all clean re
.PRECIOUS: $(BUILD_DIR)/%.o
