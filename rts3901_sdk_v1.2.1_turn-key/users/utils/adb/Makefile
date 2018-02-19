#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
#

BUILD := $(shell pwd)/.formosa/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
STRIP = $(CROSS_COMPILE)strip
SSTRIP = $(CROSS_COMPILE)sstrip

TARGETS := $(BUILD)/adbd

.PHONY: all romfs clean prepare

all: prepare $(TARGETS)

prepare:
	-mkdir -p $(BUILD)/adb $(BUILD)/libcutils

SRC_FILES :=

ADB_SRC_FILES := \
	adb.c \
	backup_service.c \
	fdevent.c \
	transport.c \
	transport_local.c \
	transport_usb.c \
	sockets.c \
	services.c \
	file_sync_service.c \
	jdwp_service.c \
	framebuffer_service.c \
	remount_service.c \
	usb_linux_client.c \
	log_service.c \
	utils.c
SRC_FILES += $(addprefix adb/, $(ADB_SRC_FILES))

CUTILS_SRC_FILES += list.c \
	socket_local_server.c \
	socket_local_client.c \
	socket_loopback_server.c \
	socket_loopback_client.c \
	socket_inaddr_any_server.c

SRC_FILES += $(addprefix libcutils/, $(CUTILS_SRC_FILES))
OBJ_FILES := $(addprefix $(BUILD)/, $(subst .c,.o,$(SRC_FILES)))

CFLAGS := -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter
CFLAGS += -D_XOPEN_SOURCE -D_GNU_SOURCE
CFLAGS += -DALLOW_ADBD_ROOT=1

# our own definition
CFLAGS += -DRLXLINUX -DNO_AUTH

INCLUDES := -Iadb -Ilibcutils -Iinclude

# from android/hardware/libhardware
INCLUDES += -Ilibhardware/include

LDFLAGS := -static
LIBS := -lpthread -lc

$(BUILD)/%.o: %.c
	$(CC) -c -o $@ $(INCLUDES) $(CFLAGS) $<

$(BUILD)/adbd: $(OBJ_FILES)
	$(LD) -o $@ $(LDFLAGS) $^ $(LIBS)

romfs:
	$(ROMFSINST) $(BUILD)/adbd /bin/adbd
	$(ROMFSINST) $(shell pwd)/S25adb /etc/rcS.d/

clean:
	-rm -rf $(BUILD)
