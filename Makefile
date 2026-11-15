#---------------------------------------------------------------------------------
# Pirate Chest 3DS - Makefile
#---------------------------------------------------------------------------------
export DEVKITPRO ?= /opt/devkitpro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM

PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)

TARGET      := piratechest
BUILD       := build
SOURCES     := source
INCLUDES    := include

APP_TITLE       := Pirate Chest
APP_DESCRIPTION := Ouvre des coffres !
APP_AUTHOR      := Moi

ICON := icon.png

#---------------------------------------------------------------------------------
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
AS      := $(PREFIX)as
AR      := $(PREFIX)ar
OBJCOPY := $(PREFIX)objcopy
STRIP   := $(PREFIX)strip
NM      := $(PREFIX)nm
RANLIB  := $(PREFIX)ranlib

ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS  := -g -Wall -O2 -mword-relocations \
           -fomit-frame-pointer -ffunction-sections \
           $(ARCH) -DARM11 -D_3DS

LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map

LIBS    := -lcitro2d -lcitro3d -lctru -lm

LIBDIRS := $(DEVKITPRO)/libctru $(DEVKITPRO)/portlibs/3ds

INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
           $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
           -I$(CURDIR)/$(BUILD)

LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#---------------------------------------------------------------------------------
CFILES   := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OFILES   := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(CFILES)))

SMDH_ARGS := --title="$(APP_TITLE)" \
             --description="$(APP_DESCRIPTION)" \
             --author="$(APP_AUTHOR)" \
             --icon=$(ICON)

.PHONY: all clean

all: $(BUILD) $(TARGET).3dsx

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: source/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(TARGET).elf: $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(TARGET).smdh: $(ICON)
	smdhtool $(SMDH_ARGS) $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh

clean:
	rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map
