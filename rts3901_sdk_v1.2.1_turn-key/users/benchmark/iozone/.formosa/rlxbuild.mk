#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Jan. 10, 2011

ifeq ($(VAR_ARCH),arm)
TARGET := linux-arm
else
TARGET := linux
endif

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-
CFLAGS += $(CFLAGS_$@)
LDLIBS += $(LDLIBS_$@)

BINARY := iozone fileop pit_server

.PHONY: apps romfs clean
all: apps

apps:
	$(MAKE) -C src/current $(TARGET) \
		CC=$(CROSS_COMPILE)gcc \
		GCC=$(CROSS_COMPILE)gcc

romfs:
	@set -e; for x in $(BINARY); do \
		$(ROMFSINST) src/current/$$x /bin/$$x; \
	done

clean:
	$(MAKE) -C src/current clean
