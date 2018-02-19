#
# Local Makefile for the configurator programs
#

VAR_HOST = $(shell uname)
CC = gcc
CXX = gc++

ifneq ($(VAR_HOST),Darwin)
CFLAGS := -DCURSES_LOC="<ncurses.h>"
LDFLAGS := -static
LDLIBS := -lncursesw -ltinfo
endif

# lxdialog peices needed locally
check-lxdialog := lxdialog/check-lxdialog.sh
CFLAGS += $(shell $(CONFIG_SHELL) $(check-lxdialog) -ccflags)
LDLIBS += $(shell $(CONFIG_SHELL) $(check-lxdialog) -ldflags $(CC))

CFLAGS += -DKBUILD_NO_NLS
HOST_EXTRACFLAGS += -DLOCALE

conf-objs := conf.o zconf.tab.o
mconf-objs := mconf.o zconf.tab.o
gconf-objs := gconf.o kconfig_load.o zconf.tab.o
qconf-objs := qconf.o kconfig_load.o zconf.tab.o
lxdialog-objs := lxdialog/checklist.o lxdialog/inputbox.o lxdialog/menubox.o \
	lxdialog/textbox.o lxdialog/util.o lxdialog/yesno.o lxdialog/timedbox.o

all: conf mconf

conf: $(conf-objs)

mconf: $(mconf-objs) $(lxdialog-objs)

zconf.tab.o: zconf.tab.c zconf.hash.c lex.zconf.c

%.c: %.c_shipped
	cp $< $@

clean:
	rm -f conf mconf *.o .*.o.cmd lxdialog/*.o
