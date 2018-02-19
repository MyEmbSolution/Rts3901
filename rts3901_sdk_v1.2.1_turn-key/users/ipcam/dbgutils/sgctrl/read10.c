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
	const char *sg, *output;

	if (argc != 5) {
		fprintf(stderr, "Usage: read10 sg lba blks output\n");
		exit(1);
	}

	sg = argv[1];
	lba = (unsigned int)strtoul(argv[2], NULL, 0);
	blks = (int)strtol(argv[3], NULL, 0);
	output = argv[4];
	len = blks * 512;

	buf = malloc(len);
	if (!buf) {
		fprintf(stderr, "out of memory\n");
		goto out;
	}

	fd = open(sg, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s fail\n", sg);
		goto out;
	}

	err = rts_read10(fd, lba, blks, buf, len);
	if (err < 0) {
		fprintf(stderr, "read10 fail\n");
		goto out;
	}

	close(fd);
	fd = 0;

	fd = open(output, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "open %s fail\n", output);
		goto out;
	}

	if (write(fd, buf, len) < len) {
		fprintf(stderr, "write %s fail\n", output);
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
