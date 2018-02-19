#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "rtcr.h"

int main(int argc, char **argv)
{
	int err = 0, fd = 0, blks, len;
	unsigned int lba;
	unsigned char *buf;
	const char *sg, *input;

	if (argc != 5) {
		fprintf(stderr, "Usage: write10 sg lba blks input\n");
		exit(1);
	}

	sg = argv[1];
	lba = (unsigned int)strtoul(argv[2], NULL, 0);
	blks = (int)strtol(argv[3], NULL, 0);
	input = argv[4];
	len = blks * 512;

	buf = malloc(len);
	if (!buf) {
		fprintf(stderr, "out of memory\n");
		goto out;
	}

	fd = open(input, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open %s fail\n", input);
		goto out;
	}

	if (read(fd, buf, len) < len) {
		fprintf(stderr, "read %s fail\n", input);
		goto out;
	}

	close(fd);

	fd = open(sg, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s fail\n", sg);
		goto out;
	}

	err = rts_write10(fd, lba, blks, buf, len);
	if (err < 0) {
		fprintf(stderr, "write10 fail\n");
		goto out;
	}

	close(fd);
	fd = 0;

out:
	if (buf)
		free(buf);
	if (fd)
		close(fd);

	return err;
}
