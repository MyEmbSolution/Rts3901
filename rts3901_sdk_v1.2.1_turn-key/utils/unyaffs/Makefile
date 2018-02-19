#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Dec. 28, 2012

all: unyaffs

CC := gcc
CFLAGS := -O2

unyaffs: unyaffs.o
	$(CC) -o unyaffs $(CFLAGS) unyaffs.o

clean:
	-rm -f *.o unyaffs
