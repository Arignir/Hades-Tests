################################################################################
##
##  This file is part of the Hades GBA Emulator, and is made available under
##  the terms of the GNU General Public License version 2.
##
##  Copyright (C) 2021-2022 - The Hades Authors
##
################################################################################

# Verbosity
Q	:= @

# Source & Target directory
SOURCE	:= $(realpath $(shell pwd))/source
BUILD	:= $(realpath $(shell pwd))/build
ROMS	:= $(realpath $(shell pwd))/roms/

TARGETS := \
	$(BUILD)/dma/dma-start-delay.gba \
	$(BUILD)/openbus/openbus-bios.gba \
	$(BUILD)/timer/timer-basic.gba \
	$(BUILD)/misc/misc-test.gba \

include $(DEVKITARM)/gba_rules

PATH	:= $(DEVKITARM)/bin:$(PATH)
CC	:= arm-none-eabi-gcc
LD	:= arm-none-eabi-gcc
OBJCOPY	:= arm-none-eabi-objcopy

CFLAGS	:= \
	-Wall \
	-Wextra \
	-std=gnu17 \
	-fms-extensions \
	-fomit-frame-pointer\
	-mcpu=arm7tdmi \
	-mtune=arm7tdmi \
	-I$(LIBGBA)/include \
	-Iinclude

LDFLAGS	:= \
	-specs=gba.specs \
	-lmm -lgba \
	-L$(LIBGBA)/lib


all: $(TARGETS)
	$(Q)mkdir -p $(ROMS)
	$(Q)cp $(TARGETS) $(ROMS)

$(BUILD)/dma/dma-start-delay.gba: $(BUILD)/dma/start-delay.o
$(BUILD)/openbus/openbus-bios.gba: $(BUILD)/openbus/bios.o
$(BUILD)/timer/timer-basic.gba: $(BUILD)/timer/basic.o
$(BUILD)/misc/misc-test.gba: $(BUILD)/misc/test.o

$(TARGETS):
	$(Q)echo "  LD $(shell basename $@)"
	$(Q)$(LD) $^ $(LDFLAGS) -o $@
	$(Q)$(OBJCOPY) -O binary $@
	$(Q)gbafix $@ > /dev/null

-include $(DEP)
$(BUILD)/%.o: $(SOURCE)/%.c
	$(Q)echo "  CC ${subst $(SOURCE)/,,$<}"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -MD -c $< -o $@

%.o: %.S
	$(Q)echo "  AS $(shell basename $<)"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(Q)echo "  RM $(shell basename $(BUILD))"
	$(Q)rm -rf $(BUILD)
	$(Q)echo "  RM $(shell basename $(ROMS))"
	$(Q)rm -rf $(ROMS)

re: clean
	$(Q)$(MAKE) --no-print-directory all
