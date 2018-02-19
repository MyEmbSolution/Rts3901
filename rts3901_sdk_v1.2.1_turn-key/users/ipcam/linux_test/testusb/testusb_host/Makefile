#
# Realtek Semiconductor Corp.
#
# Lei WANG (lei_wang@realsil.com.cn)
# Aug. 21, 2014
#

SRCS=testusb_host.c
OUTPUT=testusb_host

PTHREAD_LIBS = -lpthread
WARNINGS = -Wall -Wextra
CFLAGS += $(WARNING) -g $(PTHREAD_LIBS)

all: $(OUTPUT)

$(OUTPUT):$(SRCS:.c=.o)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $^ -o $@
	$(STRIP) $@

romfs:
	$(ROMFSINST) $(OUTPUT) /bin/$(OUTPUT)
	$(ROMFSINST) hcd-tests.sh /bin/hcd-tests.sh

clean:
	rm -f $(OUTPUT)
