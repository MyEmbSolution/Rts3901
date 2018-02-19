#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <linux/i2c-dev.h>

#define DEFAULT_I2C_BUS      "/dev/i2c-0"
#define DEFAULT_EEPROM_ADDR  0x50
#define BYTES_PER_PAGE       32
#define TOTAL_BYTES         (64*1024)
#define PAGES_NUM    (TOTAL_BYTES/BYTES_PER_PAGE)
#define MAX_BYTES	BYTES_PER_PAGE

struct i2c_msg {
	__u16 addr;
	unsigned short flags;
#define I2C_M_TEN	0x10
#define I2C_M_RD	0x01
#define I2C_M_NOSTART	0x4000
#define I2C_M_REV_DIR_ADDR	0x2000
#define I2C_M_IGNORE_NAK	0x1000
#define I2C_M_NO_RD_ACK		0x0800
	short len;
	char *buf;
};

int eeprom_write(int fd,
		 unsigned int addr,
		 unsigned int offset, unsigned char *buf, unsigned char len)
{
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg i2cmsg;
	int ret, i;
	char _buf[MAX_BYTES + 2];

	if (len > MAX_BYTES) {
		fprintf(stdout,
			"I can only write MAX_BYTES bytes at a time!\n");
		return -1;
	}

	if (len + offset > TOTAL_BYTES) {
		fprintf(stdout, "Sorry, len(%d)+offset(%d) > (page boundary)\n",
			len, offset);
		return -1;
	}

	_buf[0] = (offset & 0xff00) >> 8;
	_buf[1] = offset & 0xff;

	for (i = 0; i < len; i++)
		_buf[2 + i] = buf[i];

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr = addr;
	i2cmsg.flags = 0;
	i2cmsg.len = 2 + len;
	i2cmsg.buf = _buf;

	ret = ioctl(fd, I2C_RDWR, &msg_rdwr);
	if (ret < 0) {
		perror("ioctl()");
		fprintf(stdout, "ioctl returned %d\n", ret);
		return -1;
	}

	if (len > 0)
		fprintf(stdout,
			"Wrote %d bytes to eeprom at 0x%02x, offset %08x\n",
			len, addr, offset);
	return 0;
}

int eeprom_read(int fd,
		unsigned int addr,
		unsigned int offset, unsigned char *buf, unsigned char len)
{
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg i2cmsg;
	int ret;

	if (eeprom_write(fd, addr, offset, NULL, 0) < 0)
		return -1;

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr = addr;
	i2cmsg.flags = I2C_M_RD;
	i2cmsg.len = len;
	i2cmsg.buf = buf;

	ret = ioctl(fd, I2C_RDWR, &msg_rdwr);
	if (ret < 0) {
		perror("ioctl()");
		fprintf(stdout, "ioctl returned %d\n", ret);
		return -1;
	}

	fprintf(stdout, "Read %d bytes from eeprom at 0x%02x, offset %08x\n",
		len, addr, offset);

	return 0;
}

int main(int argc, char **argv)
{
	int i, j, err = 0;
	unsigned char buf[TOTAL_BYTES];
	unsigned char buf2[TOTAL_BYTES];

	/* filedescriptor and name of device */
	int d;
	char *dn = DEFAULT_I2C_BUS;

	/* filedescriptor and name of data file */
	char *fn = NULL;

	unsigned int addr = DEFAULT_EEPROM_ADDR;

	d = open(dn, O_RDWR);
	if (d < 0) {
		fprintf(stdout, "Could not open i2c at %s\n", dn);
		perror(dn);
		goto failed;
	}

	fprintf(stdout,
		"*******************start write 0x00 test*********************\n");

	memset(buf, 0, TOTAL_BYTES);
	memset(buf2, 0, TOTAL_BYTES);

	for (j = 0; j < PAGES_NUM; j++) {
		if (eeprom_write
		    (d, addr, j * BYTES_PER_PAGE, buf + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "write addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}
		usleep(10000);
	}
	for (j = 0; j < PAGES_NUM; j++)
		if (eeprom_read
		    (d, addr, j * BYTES_PER_PAGE, buf2 + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "read addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i++) {
		if (buf[i] != buf2[i]) {
			fprintf(stdout, "addr:%x %x %x\n", i, buf[i], buf2[i]);
			err = 1;
		}

	}

	if (err == 1) {
		fprintf(stdout, "write 0x0 test failed\n");
		goto failed;
	} else
		fprintf(stdout, "write 0x0 test passed\n");

	fprintf(stdout,
		"*******************start write 0xFF test*********************\n");

	memset(buf, 0xff, TOTAL_BYTES);
	memset(buf2, 0, TOTAL_BYTES);

	for (j = 0; j < PAGES_NUM; j++) {
		if (eeprom_write
		    (d, addr, j * BYTES_PER_PAGE, buf + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "write addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}
		usleep(10000);
	}
	for (j = 0; j < PAGES_NUM; j++)
		if (eeprom_read
		    (d, addr, j * BYTES_PER_PAGE, buf2 + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "read addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i++) {
		if (buf[i] != buf2[i]) {
			fprintf(stdout, "addr:%x %x %x\n", i, buf[i], buf2[i]);
			err = 1;
		}

	}

	if (err == 1) {
		fprintf(stdout, "write 0xFF test failed\n");
		goto failed;
	} else
		fprintf(stdout, "write 0xFF test passed\n");

	fprintf(stdout,
		"*******************start write 0xaa test*********************\n");

	memset(buf, 0xaa, TOTAL_BYTES);
	memset(buf2, 0, TOTAL_BYTES);

	for (j = 0; j < PAGES_NUM; j++) {
		if (eeprom_write
		    (d, addr, j * BYTES_PER_PAGE, buf + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "write addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}
		usleep(10000);
	}
	for (j = 0; j < PAGES_NUM; j++)
		if (eeprom_read
		    (d, addr, j * BYTES_PER_PAGE, buf2 + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "read addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i++) {
		if (buf[i] != buf2[i]) {
			fprintf(stdout, "addr:%x %x %x\n", i, buf[i], buf2[i]);
			err = 1;
		}

	}

	if (err == 1) {
		fprintf(stdout, "write 0xaa test failed\n");
		goto failed;
	} else
		fprintf(stdout, "write 0xaa test passed\n");

	fprintf(stdout,
		"*******************start write 0x55 test*********************\n");

	memset(buf, 0x55, TOTAL_BYTES);
	memset(buf2, 0, TOTAL_BYTES);

	for (j = 0; j < PAGES_NUM; j++) {
		if (eeprom_write
		    (d, addr, j * BYTES_PER_PAGE, buf + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "write addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}
		usleep(10000);
	}
	for (j = 0; j < PAGES_NUM; j++)
		if (eeprom_read
		    (d, addr, j * BYTES_PER_PAGE, buf2 + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "read addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i++) {
		if (buf[i] != buf2[i]) {
			fprintf(stdout, "addr:%x %x %x\n", i, buf[i], buf2[i]);
			err = 1;
		}

	}

	if (err == 1) {
		fprintf(stdout, "write 0x55 test failed\n");
		goto failed;
	} else
		fprintf(stdout, "write 0x55 test passed\n");

	fprintf(stdout,
		"*******************start write 0x5a test*********************\n");

	memset(buf, 0x5a, TOTAL_BYTES);
	memset(buf2, 0, TOTAL_BYTES);

	for (j = 0; j < PAGES_NUM; j++) {
		if (eeprom_write
		    (d, addr, j * BYTES_PER_PAGE, buf + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "write addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}
		usleep(10000);
	}
	for (j = 0; j < PAGES_NUM; j++)
		if (eeprom_read
		    (d, addr, j * BYTES_PER_PAGE, buf2 + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "read addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i++) {
		if (buf[i] != buf2[i]) {
			fprintf(stdout, "addr:%x %x %x\n", i, buf[i], buf2[i]);
			err = 1;
		}

	}

	if (err == 1) {
		fprintf(stdout, "write 0x5a test failed\n");
		goto failed;
	} else
		fprintf(stdout, "write 0x5a test passed\n");

	fprintf(stdout,
		"*******************start write 0xa5 test*********************\n");

	memset(buf, 0xa5, TOTAL_BYTES);
	memset(buf2, 0, TOTAL_BYTES);

	for (j = 0; j < PAGES_NUM; j++) {
		if (eeprom_write
		    (d, addr, j * BYTES_PER_PAGE, buf + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "write addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}
		usleep(10000);
	}
	for (j = 0; j < PAGES_NUM; j++)
		if (eeprom_read
		    (d, addr, j * BYTES_PER_PAGE, buf2 + (j * BYTES_PER_PAGE),
		     BYTES_PER_PAGE) < 0) {
			fprintf(stdout, "read addr:0x%x failed\n",
				j * BYTES_PER_PAGE);
			goto failed;
		}

	for (i = 0; i < TOTAL_BYTES; i++) {
		if (buf[i] != buf2[i]) {
			fprintf(stdout, "addr:%x %x %x\n", i, buf[i], buf2[i]);
			err = 1;
		}

	}

	if (err == 1) {
		fprintf(stdout, "write 0xa5 test failed\n");
		goto failed;
	} else
		fprintf(stdout, "write 0xa5 test passed\n");

	fprintf(stdout, "all tests passed!!\n");

failed:
	close(d);
	exit(0);

}
