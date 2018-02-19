
include Makefile.conf

STRIP	:= strip
TARGET_BIN := rtwftool

SRCS := $(wildcard *.c)

C_FLAGS +=-I$(DIR_TMPFS)/include
LD_FLAGS += -L$(DIR_TMPFS)/lib
LD_FLAGS += -lpthread

ifeq ($(STATIC_BUILT),y)
LD_FLAGS += -static
else
ifeq ($(COMMON_PCI),y)
LD_FLAGS += -lpci
C_FLAGS += -DCOMMON_PCI
endif

ifeq ($(SUPPORT_UI),y)
LD_FLAGS += -lcurses
C_FLAGS += -DSUPPORT_UI
endif
endif #STATIC_BUILT

ifeq ($(RANDOM_DATA),y)
C_FLAGS += -DRANDOM_DATA
endif

CFLAGS := $(C_FLAGS)
LDFLAGS := $(LD_FLAGS)

OBJS := $(patsubst %.c, %.o, $(SRCS))
DEPS := $(patsubst %.c, %.d, $(SRCS))
HDRS := $(wildcard ./*.h)

all: $(DEPS) $(OBJS) $(TARGET_BIN)

${TARGET_BIN}: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c $(HDRS)
	@$(CC) -MM $(CFLAGS) $< > $@
	@$(CC) -MM $(CFLAGS) $< | sed s/\\.o/.d/ >> $@

strip:
	$(STRIP) ${TARGET_BIN}

distclean: clean

romfs:
	$(ROMFSINST) rtwftool /bin/rtwftool
	$(ROMFSINST) rtwftool /bin/rtwftool_dbg
clean:
	-rm -rf $(OBJS) $(DEPS) $(TARGET_BIN)

.PHONY : clean distclean strip
