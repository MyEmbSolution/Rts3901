#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
#

-include $(DIR_TMPFS)/appconfig.in

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc

exe-y :=

exe-$(CONFIG_CPU_HAS_RADIAX) += radiax1
exe-$(CONFIG_ARCH_CPU_MIPS) += unalign-mips16
exe-$(CONFIG_ARCH_CPU_RLX) += unalign-mips16

exe-y += \
        hello-c-static \
        hello-c-shared \
        hello-cxx-static \
        hello-cxx-shared \
        longjmp1 \
        longjmp2 \
        ctor \
        epoll \
        float \
        grandpa \
        grandpa2 \
        pthread \
        pthread-cpu \
        pthread-condvar \
        pthread-cond-shared \
        pthread-cond-static \
        pthread-mutex \
        pthread-join \
        pthread-stack \
        pthread-test \
        sigfillset \
        testsig \
        libtest.so \
        testso \
        testlibm \
        statfs \
        fastcp \
        string \
        unalign \
        errnotest \
	cunit-demo \
	testmtd

CFLAGS_unalign_mips16 = -mips16

LDFLAGS_testsig = -static
LDFLAGS_testso = -static
LDFLAGS_pthread = -static
LDFLAGS_errnotest = -pthread

LDLIBS_hello-cxx-static = -lstdc++
LDLIBS_hello-cxx-shared = -lstdc++
LDLIBS_pthread = -lc -lpthread
LDLIBS_pthread-cpu = -lpthread
LDLIBS_pthread-condvar = -lc -lpthread
LDLIBS_pthread-cond-static = -lpthread -lstdc++
LDLIBS_pthread-cond-shared = -lpthread -lstdc++
LDLIBS_pthread-mutex = -lpthread
LDLIBS_pthread-stack = -lpthread
LDLIBS_pthread-test = -lpthread
LDLIBS_pthread-join = -lpthread -lm
LDLIBS_grandpa2 = -lpthread
LDLIBS_testlibm = -lm
LDLIBS_testso = -ldl
LDLIBS_float = -lm
LDLIBS_fixed-point = -lm
LDLIBS_cunit-demo = -lcunit
LDLIBS_testmtd = -lmtd

LDFLAGS += $(LDFLAGS_$@)
LDLIBS += $(LDLIBS_$@)

all: $(exe-y)

%-static: %.o
	$(CC) -static $(LDFLAGS) -o $@ $< $(LDLIBS)

%-shared: %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

unalign-mips16.o: unalign.c
	$(CC) $(CFLAGS) -mips16 -o $@ -c $<

libtest.so: libtest1.os libtest2.os
	$(LD) $(LDFLAGS) -shared -nostartfiles -Wl,-soname=libtest.so -o $@ $^

romfs:
	for x in $(exe-y); do $(ROMFSINST) $$x /bin/$$x; done

clean:
	$(Q)-rm -f $(exe-y) *.o *.os

%.os: %.c
	$(CC) $(CFLAGS) -fPIC -o $(<:.c=.os) -c $<
