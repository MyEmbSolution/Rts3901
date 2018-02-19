CFLAGS += -Wall -g  -I$(DIR_TMPFS)/include
LDFLAGS += -L$(DIR_TMPFS)/lib -loctopus

.PHONY: all tmpfs romfs

SHARED_LIB_CLI = librtsuvc.so
SHARED_LIB_SVR = opt_uvc.so
all: $(SHARED_LIB_CLI) $(SHARED_LIB_SVR) tmpfs

$(SHARED_LIB_CLI):cli_uvc.c cli_v4l2.c char_library.c isp.c rts_cam.c rts_cam_utils.c
	$(CC) -shared $(CFLAGS) -fPIC -o $@ $^

$(SHARED_LIB_SVR):svr_uvc.c svr_uvc_utils.c
	$(CC) -shared $(CFLAGS) -fPIC -o $@ $^ -L$(DIR_TMPFS)/lib -ljson-c -lm


tmpfs:
	cp -f $(SHARED_LIB_CLI) $(DIR_TMPFS)/lib
	cp -f $(SHARED_LIB_SVR) $(DIR_TMPFS)/lib
	cp -f rts_uvc.h $(DIR_TMPFS)/include

romfs:
	$(ROMFSINST) $(SHARED_LIB_CLI) /lib
	mkdir -p $(DIR_ROMFS)/usr/lib/octopus
	$(ROMFSINST) $(SHARED_LIB_SVR) /usr/lib/octopus


clean:
	rm -rf *.so *.o
