#
# Realtek Semiconductor Corp.
#
# Lei WANG (lei_wang@realsil.com.cn)
# June. 4, 2015
#

SRCS=testusb_device.c
OUTPUT=testusb_device

CFLAGS += -Wall -Wextra -g

all: $(OUTPUT)

$(OUTPUT):$(SRCS:.c=.o)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $^ -o $@
	$(STRIP) $@

romfs:
	$(ROMFSINST) $(OUTPUT) /bin/$(OUTPUT)

clean:
	rm -f $(OUTPUT)
