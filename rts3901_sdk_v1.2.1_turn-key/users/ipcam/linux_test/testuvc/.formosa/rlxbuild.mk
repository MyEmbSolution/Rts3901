#
# Realtek Semiconductor Corp.
#
# peter_sun (peter_sun@realsil.com.cn)
# Oct. 31, 2014
#

SRCS=testuvc.c h264enc.c
OUTPUT=testuvc

all: $(OUTPUT)

$(OUTPUT):$(SRCS:.c=.o)
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@ -lpthread -lh1encoder -lrtsisp
	$(STRIP) $@

romfs:
	$(ROMFSINST) $(OUTPUT) /bin/$(OUTPUT)

clean:
	rm -f $(OUTPUT)
