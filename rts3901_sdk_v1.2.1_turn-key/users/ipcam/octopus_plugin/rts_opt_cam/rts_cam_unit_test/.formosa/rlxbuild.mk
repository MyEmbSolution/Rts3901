CFLAGS += -Wall -g  -I$(DIR_TMPFS)/include
LDFLAGS += -L$(DIR_TMPFS)/lib -ljson-c -lpemsg -lrtsisp -lrtscamkit -loctopus -lrtscam -lcunit

.PHONY: all tmpfs romfs

all: test_rts_cam tmpfs

test_rts_cam:test.c test_isp.c test_osd.c test_mtd.c test_mask.c test_cam.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tmpfs:

romfs:

clean:
	rm -rf test_rts_cam *.o
