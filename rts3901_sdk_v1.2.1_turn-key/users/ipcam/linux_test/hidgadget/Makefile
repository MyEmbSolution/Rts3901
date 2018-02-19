#
# Realtek Semiconductor Corp.
#
# Wei WANG (wei_wang@realsil.com.cn)
# Dec. 17, 2014
#

SRCS=hidgadget.c
OUTPUT=hidgadget

all: $(OUTPUT)

$(OUTPUT):$(SRCS:.c=.o)
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@
	$(STRIP) $@

romfs:
	$(ROMFSINST) $(OUTPUT) /bin/$(OUTPUT)

clean:
	rm -f $(OUTPUT)
