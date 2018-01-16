# build directory
BUILD_BASE	= build

# firmware directory
FW_BASE		= firmware

# name for the target project
TARGET		= app

# Base directory for the compiler
XTENSA_TOOLS_ROOT ?= c:/Espressif/xtensa-lx106-elf/bin

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= c:/Espressif/ESP8266_SDK
SDK_TOOLS	?= c:/Espressif/utils/ESP8266

# Extra libs, include and ld file
EXTRA_BASE	?= c:/Espressif/extra

# esptool path and port
ESPTOOL		?= $(SDK_TOOLS)/esptool.exe
ESPPORT		?= COM3

# Baud rate for programmer
ESPBAUD		?= 256000

# Boot mode:
# Valid values are none, old, new
# none - non use bootloader
# old  - boot_v1.1
# new  - boot_v1.2+
BOOT ?= none

# Choose bin generate (0=eagle.flash.bin+eagle.irom0text.bin, 1=user1.bin, 2=user2.bin)
APP ?= 0

# Flash Frequency for ESP8266
# Clock frequency for SPI flash interactions.
# Valid values are 20, 26, 40, 80 (MHz).
SPI_SPEED ?= 40

# Flash Mode for ESP8266
# These set Quad Flash I/O or Dual Flash I/O modes.
# Valid values are  QIO, QOUT, DIO, DOUT
SPI_MODE ?= QIO

# Flash Size for ESP8266
# Size of the SPI flash, given in megabytes.
# ESP-12, ESP-12E and ESP-12F modules (and boards that use them such as NodeMCU, HUZZAH, etc.) usually
# have at least 4 megabyte / 4MB (sometimes labelled 32 megabit) flash.
# If using OTA, some additional sizes & layouts for OTA "firmware slots" are available.
# If not using OTA updates then you can ignore these extra sizes:
#
# Valid values vary by chip type: 1, 2, 3, 4, 5, 6, 7, 8, 9
#
#|SPI_SIZE_MAP|flash_size arg | Number of OTA slots | OTA Slot Size | Non-OTA Space |
#|------------|---------------|---------------------|---------------|---------------|
#|1           |256KB          | 1 (no OTA)          | 256KB         | N/A           |
#|2           |512KB          | 1 (no OTA)          | 512KB         | N/A           |
#|3           |1MB            | 2                   | 512KB         | 0KB           |
#|4           |2MB            | 2                   | 512KB         | 1024KB        |
#|5           |4MB            | 2                   | 512KB         | 3072KB        |
#|6           |2MB-c1         | 2                   | 1024KB        | 0KB           |
#|7           |4MB-c1         | 2                   | 1024KB        | 2048KB        |
#|8           |8MB [^]        | 2                   | 1024KB        | 6144KB        |
#|9           |16MB [^]       | 2                   | 1024KB        | 14336KB       |
#
# [^] Support for 8MB & 16MB flash size is not present in all ESP8266 SDKs. If your SDK doesn't support these flash sizes, use 4MB.
#
SPI_SIZE_MAP ?= 2

# Main settings includes
include	../settings.mk

# Individual project settings (Optional)

# Boot mode:
# Valid values are  none, old, new
# none - non use bootloader
# old  - boot_v1.1
# new  - boot_v1.2+
#BOOT = new

# Choose bin generate (0=eagle.flash.bin+eagle.irom0text.bin, 1=user1.bin, 2=user2.bin)
#APP = 1

# Flash Frequency for ESP8266
# Clock frequency for SPI flash interactions.
# Valid values are 20, 26, 40, 80 (MHz).
#SPI_SPEED = 40

# Flash Mode for ESP8266
# These set Quad Flash I/O or Dual Flash I/O modes.
# Valid values are  QIO, QOUT, DIO, DOUT
#SPI_MODE = QIO

# Flash Size for ESP8266
# Size of the SPI flash, given in megabytes.
# ESP-12, ESP-12E and ESP-12F modules (and boards that use them such as NodeMCU, HUZZAH, etc.) usually
# have at least 4 megabyte / 4MB (sometimes labelled 32 megabit) flash.
# If using OTA, some additional sizes & layouts for OTA "firmware slots" are available.
# If not using OTA updates then you can ignore these extra sizes:
#
# Valid values vary by chip type: 1, 2, 3, 4, 5, 6, 7, 8, 9
#
#|SPI_SIZE_MAP|flash_size arg | Number of OTA slots | OTA Slot Size | Non-OTA Space |
#|------------|---------------|---------------------|---------------|---------------|
#|1           |256KB          | 1 (no OTA)          | 256KB         | N/A           |
#|2           |512KB          | 1 (no OTA)          | 512KB         | N/A           |
#|3           |1MB            | 2                   | 512KB         | 0KB           |
#|4           |2MB            | 2                   | 512KB         | 1024KB        |
#|5           |4MB            | 2                   | 512KB         | 3072KB        |
#|6           |2MB-c1         | 2                   | 1024KB        | 0KB           |
#|7           |4MB-c1         | 2                   | 1024KB        | 2048KB        |
#|8           |8MB [^]        | 2                   | 1024KB        | 6144KB        |
#|9           |16MB [^]       | 2                   | 1024KB        | 14336KB       |
#
# [^] Support for 8MB & 16MB flash size is not present in all ESP8266 SDKs. If your SDK doesn't support these flash sizes, use 4MB.
#
SPI_SIZE_MAP = 3

# COM port settings.
# COM port number and baud rate:
ESPPORT = COM3
#ESPBAUD = 256000

# Basic project settings
MODULES	= driver user
LIBS	= c gcc hal phy pp net80211 lwip wpa main crypto

# Root includes
include	../common_nonos.mk

#############################################################
#
# Root Level Makefile
#
# Version 2.0
#
# (c) by CHERTS <sleuthhound@gmail.com>
#
#############################################################

ifeq ($(BOOT), new)
    boot = new
else
    ifeq ($(BOOT), old)
        boot = old
    else
        boot = none
    endif
endif

ifeq ($(APP), 1)
    app = 1
else
    ifeq ($(APP), 2)
        app = 2
    else
        app = 0
    endif
endif

ifeq ($(SPI_SPEED), 26)
    freqdiv = 1
    flashimageoptions = -ff 26m
else
    ifeq ($(SPI_SPEED), 20)
        freqdiv = 2
        flashimageoptions = -ff 20m
    else
        ifeq ($(SPI_SPEED), 80)
            freqdiv = 15
            flashimageoptions = -ff 80m
        else
            freqdiv = 0
            flashimageoptions = -ff 40m
        endif
    endif
endif

ifeq ($(SPI_MODE), QOUT)
    mode = 1
    flashimageoptions += -fm qout
else
    ifeq ($(SPI_MODE), DIO)
	mode = 2
	flashimageoptions += -fm dio
    else
        ifeq ($(SPI_MODE), DOUT)
            mode = 3
            flashimageoptions += -fm dout
        else
            mode = 0
            flashimageoptions += -fm qio
        endif
    endif
endif

addr = 0x01000

ifeq ($(SPI_SIZE_MAP), 1)
  size_map = 1
  flash = 256
  flashimageoptions += -fs 256KB
else
  ifeq ($(SPI_SIZE_MAP), 2)
    size_map = 2
    flash = 1024
    flashimageoptions += -fs 1MB
    ifeq ($(app), 2)
      addr = 0x81000
    endif
  else
    ifeq ($(SPI_SIZE_MAP), 3)
      size_map = 3
      flash = 2048
      flashimageoptions += -fs 2MB
      ifeq ($(app), 2)
        addr = 0x81000
      endif
    else
      ifeq ($(SPI_SIZE_MAP), 4)
	size_map = 4
	flash = 4096
	flashimageoptions += -fs 4MB
        ifeq ($(app), 2)
          addr = 0x81000
        endif
      else
        ifeq ($(SPI_SIZE_MAP), 5)
          size_map = 5
          flash = 2048
          flashimageoptions += -fs 2MB-c1
          ifeq ($(app), 2)
            addr = 0x101000
          endif
        else
          ifeq ($(SPI_SIZE_MAP), 6)
            size_map = 6
            flash = 4096
            flashimageoptions += -fs 4MB-c1
            ifeq ($(app), 2)
              addr = 0x101000
            endif
          else
            ifeq ($(SPI_SIZE_MAP), 8)
              size_map = 8
              flash = 8192
              flashimageoptions += -fs 8MB
              ifeq ($(app), 2)
                addr = 0x101000
              endif
            else
              ifeq ($(SPI_SIZE_MAP), 9)
                size_map = 9
                flash = 16384
                flashimageoptions += -fs 16MB
                ifeq ($(app), 2)
                  addr = 0x101000
                endif
              else
                size_map = 0
                flash = 512
                flashimageoptions += -fs 512KB
                ifeq ($(app), 2)
                addr = 0x41000
                endif
              endif
            endif
          endif
        endif
      endif
    endif
  endif
endif

EXTRA_INCDIR    = include $(EXTRA_BASE)/include

# compiler flags using during compilation of source files
CFLAGS		= -Os -g -O2 -std=gnu90 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -mno-serialize-volatile -D__ets__ -DICACHE_FLASH
CXXFLAGS	= -Os -g -O2 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -mno-serialize-volatile -D__ets__ -DICACHE_FLASH -fno-rtti -fno-exceptions

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# linker script used for the above linkier step
LD_SCRIPT	= eagle.app.v6.ld

ifneq ($(boot), none)
ifneq ($(app),0)
    ifneq ($(findstring $(size_map),  6  8  9),)
      LD_SCRIPT = eagle.app.v6.$(boot).2048.ld
    else
      ifeq ($(size_map), 5)
        LD_SCRIPT = eagle.app.v6.$(boot).2048.ld
      else
        ifeq ($(size_map), 4)
          LD_SCRIPT = eagle.app.v6.$(boot).1024.app$(app).ld
        else
          ifeq ($(size_map), 3)
            LD_SCRIPT = eagle.app.v6.$(boot).1024.app$(app).ld
          else
            ifeq ($(size_map), 2)
              LD_SCRIPT = eagle.app.v6.$(boot).1024.app$(app).ld
            else
              ifeq ($(size_map), 0)
                LD_SCRIPT = eagle.app.v6.$(boot).512.app$(app).ld
              endif
            endif
	      endif
	    endif
	  endif
	endif
	BIN_NAME = user$(app).$(flash).$(boot).$(size_map)
	CFLAGS += -DAT_UPGRADE_SUPPORT
endif
else
    app = 0
endif

# various paths from the SDK used in this project
SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include include/json

# select which tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
CXX		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-g++
AR		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
OBJDUMP		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objdump

SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c*))
C_OBJ		:= $(patsubst %.c,%.o,$(SRC))
CXX_OBJ		:= $(patsubst %.cpp,%.o,$(C_OBJ))
OBJ		:= $(patsubst %.o,$(BUILD_BASE)/%.o,$(CXX_OBJ))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)

LD_SCRIPT	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR		:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

vpath %.c $(SRC_DIR)
vpath %.cpp $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS)  -c $$< -o $$@
$1/%.o: %.cpp
	$(vecho) "C+ $$<"
	$(Q) $(CXX) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CXXFLAGS)  -c $$< -o $$@
endef

.PHONY: all checkdirs clean flash flashboot flashinit rebuild

all: checkdirs $(TARGET_OUT)

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@
	$(vecho) "Run objcopy, please wait..."
	$(Q) $(OBJCOPY) --only-section .text -O binary $@ eagle.app.v6.text.bin
	$(Q) $(OBJCOPY) --only-section .data -O binary $@ eagle.app.v6.data.bin
	$(Q) $(OBJCOPY) --only-section .rodata -O binary $@ eagle.app.v6.rodata.bin
	$(Q) $(OBJCOPY) --only-section .irom0.text -O binary $@ eagle.app.v6.irom0text.bin
	$(vecho) "objcopy done"
	$(vecho) "Run gen_appbin.exe"
ifeq ($(app), 0)
	$(Q) $(SDK_TOOLS)/gen_appbin.exe $@ 0 $(mode) $(freqdiv) $(size_map) $(app)
	$(Q) mv eagle.app.flash.bin $(FW_BASE)/eagle.flash.bin
	$(Q) mv eagle.app.v6.irom0text.bin $(FW_BASE)/eagle.irom0text.bin
	$(Q) rm eagle.app.v6.*
	$(vecho) "No boot needed."
	$(vecho) "Generate eagle.flash.bin and eagle.irom0text.bin successully in folder $(FW_BASE)"
	$(vecho) "eagle.flash.bin-------->0x00000"
	$(vecho) "eagle.irom0text.bin---->0x10000"
else
    ifneq ($(boot), new)
	$(Q) $(SDK_TOOLS)/gen_appbin.exe $@ 1 $(mode) $(freqdiv) $(size_map) $(app)
	$(vecho) "Support boot_v1.2 and +"
    else
	$(Q) $(SDK_TOOLS)/gen_appbin.exe $@ 2 $(mode) $(freqdiv) $(size_map) $(app)
    	ifneq ($(findstring $(size_map),  6  8  9),)
		$(vecho) "Support boot_v1.7 and +"
        else
            ifeq ($(size_map), 5)
		$(vecho) "Support boot_v1.6 and +"
            else
		$(vecho) "Support boot_v1.2 and +"
            endif
        endif
    endif
	$(Q) mv eagle.app.flash.bin $(FW_BASE)/upgrade/$(BIN_NAME).bin
	$(Q) rm eagle.app.v6.*
	$(vecho) "Generate $(BIN_NAME).bin successully in folder $(FW_BASE)/upgrade"
	$(vecho) "boot.bin------->0x00000"
	$(vecho) "$(BIN_NAME).bin--->$(addr)"
endif
	$(vecho) "Done"

$(APP_AR): $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(FW_BASE):
	$(Q) mkdir -p $@
	$(Q) mkdir -p $@/upgrade

flashboot:
ifeq ($(app), 0)
	$(vecho) "No boot needed."
else
    ifneq ($(boot), new)
	$(vecho) "Flash boot_v1.2 and +"
	$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(SDK_BASE)/bin/boot_v1.2.bin
    else
    	ifneq ($(findstring $(size_map),  6  8  9),)
		$(vecho) "Flash boot_v1.7 and +"
		$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(SDK_BASE)/bin/boot_v1.7.bin
        else
            ifeq ($(size_map), 5)
		$(vecho) "Flash boot_v1.5 and +"
		$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(SDK_BASE)/bin/boot_v1.6.bin
            else
		$(vecho) "Flash boot_v1.2 and +"
		$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(SDK_BASE)/bin/boot_v1.2.bin
            endif
        endif
    endif
endif

flash: all
ifeq ($(app), 0)
	$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(FW_BASE)/eagle.flash.bin 0x10000 $(FW_BASE)/eagle.irom0text.bin
else
ifeq ($(boot), none)
	$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) 0x00000 $(FW_BASE)/eagle.flash.bin 0x10000 $(FW_BASE)/eagle.irom0text.bin
else
	$(ESPTOOL) -p $(ESPPORT) -b $(ESPBAUD) write_flash $(flashimageoptions) $(addr) $(FW_BASE)/upgrade/$(BIN_NAME).bin
endif
endif

# ===============================================================
# From http://bbs.espressif.com/viewtopic.php?f=10&t=305
# master-device-key.bin is only need if using espressive services
# master_device_key.bin 0x3e000 is not used , write blank
# See 2A-ESP8266__IOT_SDK_User_Manual__EN_v1.1.0.pdf
# http://bbs.espressif.com/download/file.php?id=532
#
# System parameter area is the last 16KB of flash
# 512KB flash - system parameter area starts from 0x7C000
# 	download blank.bin to 0x7E000 as initialization.
# 1024KB flash - system parameter area starts from 0xFC000
# 	download blank.bin to 0xFE000 as initialization.
# 2048KB flash - system parameter area starts from 0x1FC000
# 	download blank.bin to 0x1FE000 as initialization.
# 4096KB flash - system parameter area starts from 0x3FC000
# 	download blank.bin to 0x3FE000 as initialization.
# ===============================================================

# FLASH SIZE
flashinit:
	$(vecho) "Flash init data default and blank data."
	$(ESPTOOL) -p $(ESPPORT) write_flash $(flashimageoptions) 0xfc000 $(SDK_BASE)/bin/esp_init_data_default.bin 0xfe000 $(SDK_BASE)/bin/blank.bin

rebuild: clean all

clean:
	$(Q) rm -f $(APP_AR)
	$(Q) rm -f $(TARGET_OUT)
	$(Q) rm -rf $(BUILD_DIR)
	$(Q) rm -rf $(BUILD_BASE)
	$(Q) rm -rf $(FW_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
