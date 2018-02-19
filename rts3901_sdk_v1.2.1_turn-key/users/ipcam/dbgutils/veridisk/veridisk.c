/*
 * simple disk verify program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <linux/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SECTOR_SIZE				512
#define DUMP_CMP_MAX_ITEMS			64
#define DUMP_CMP_MAX_ITEMS_PER_LINE		8
#define BYTES_PER_MB				1048576
#define SPEED_COEFFICIENT			0.48828125

static int print_verbose_debug_line;
static int signal_int_count;

#define dbg_help(fmt, arg...)			\
	printf(fmt, ##arg)
#define dbg_data dbg_help
#define dbg_line(fmt, arg...)			\
	printf("%s: " fmt, __func__, ##arg)

#define dbg_lline(fmt, arg...)			\
	do {					\
		if (print_verbose_debug_line)	\
			printf(fmt, ##arg);	\
	} while (0)

struct veridisk {
	char *name;

	int offset;			/* sectors */
	int sectors;
	int pattern;
#define PATTERN_MASK			0xF00
#define FIXED_PATTERN			0x100
#define SEQUENCE_PATTERN		0x200
#define RANDOM_PATTERN			0x300

	float rd_time;			/* ms */
	int rd_size;			/* sectors */

	float wr_time;
	int wr_size;

	int direct;
	int quit_on_error;
	int open_errors;
	int wr_errors;
	int rd_errors;
	int cmp_errors;
	int pass_count;
	int test_count;
};

static int read_only = 0;
static int write_only = 0;

static ssize_t cond_read(int fd, void *buf, size_t count)
{
	if (!write_only)
		return read(fd, buf, count);

	return count;
}

static ssize_t cond_write(int fd, void *buf, size_t count)
{
	if (!read_only)
		return write(fd, buf, count);

	return count;
}

static int cond_cmp(const void *s1, const void *s2, size_t n)
{
	if (!read_only && !write_only)
		return memcmp(s1, s2, n);

	return 0;
}

int pattern_type(int pattern)
{
	return pattern & PATTERN_MASK;
}

void veridisk_sa(int signum)
{
	if (signum == SIGINT)
		signal_int_count++;
}

const char *pattern_name(int pattern)
{
	switch (pattern_type(pattern)) {
	case FIXED_PATTERN:
		return "fixed";
	case SEQUENCE_PATTERN:
		return "seq";
	case RANDOM_PATTERN:
		return "random";
	default:
		return "unknown";
	}
}

void set_pattern(struct veridisk *disk, int set)
{
	disk->pattern &= ~0xFF;
	disk->pattern |= (set & 0xFF);
}

void dump_difference(unsigned char *wdata, unsigned char *rdata, long length)
{
	long i;
	int line_items = 0;
	int all_items = 0;

	for (i = 0; i < length; i++, wdata++, rdata++) {
		if (*wdata != *rdata) {
			dbg_data("%04lx:%02x/%02x ", i, *wdata, *rdata);
			if (++line_items == DUMP_CMP_MAX_ITEMS_PER_LINE) {
				dbg_data("\n");
				line_items = 0;
			}
			if (++all_items == DUMP_CMP_MAX_ITEMS)
				break;
		}
	}

	if (line_items != 0)
		dbg_data("\n");
}

float time_interval_ms(struct timeval *t1, struct timeval *t2)
{
	int sec = t2->tv_sec - t1->tv_sec;
	float msec = (t2->tv_usec - t1->tv_usec) / 1000.0;

	msec =  msec + 1000 * sec;

	return msec < 0 ? -msec : msec;
}

inline float calc_speed(float ms, int sectors)
{
	return SPEED_COEFFICIENT * sectors / (ms < 0.001 ? 0.001 : ms);
}

unsigned char *memset_pattern(unsigned char *data, int pattern, int length)
{
	unsigned i = 0;

	if (pattern_type(pattern) == FIXED_PATTERN) {
		return memset(data, pattern & 0xFF, length);
	} else if (pattern_type(pattern) == SEQUENCE_PATTERN) {
		for (i = 0; i < length; i++)
			*(data + i) = (pattern + i) & 0xFF;
	} else if (pattern_type(pattern) == RANDOM_PATTERN) {
		*data = pattern & 0xFF;
		for (i = 1; i < length; i++)
			*(data + i) = rand() % 0xFF;
	} else {
		dbg_line("unknown pattern type\n");
	}

	return data;
}

int veridisk_write(struct veridisk *disk)
{
	struct timeval t1 = {0}, t2 = {0};
	float wr_delta = 0, rd_delta = 0;
	int offset = disk->offset * SECTOR_SIZE;
	int length = disk->sectors * SECTOR_SIZE;
	int err = 0;
	int fd, flags;
	unsigned char *wdata;
	unsigned char *rdata;
	int size;

	dbg_lline("offset %d, len %d, %s pattern 0x%02x\n",
		disk->offset, disk->sectors, pattern_name(disk->pattern),
		disk->pattern & 0xFF);

	disk->test_count++;

	flags = disk->direct ? O_RDWR | O_DIRECT : O_RDWR;
	fd = open(disk->name, flags);
	if (fd < 0) {
		disk->open_errors++;
		dbg_line("open file failed\n");
		return -EIO;
	}

	wdata = memalign(SECTOR_SIZE, length);
	if (!wdata) {
		err = -ENOMEM;
		dbg_line("mem align for write failed\n");
		goto close_file;
	}

	wdata = memset_pattern(wdata, disk->pattern, length);

	lseek(fd, offset, SEEK_SET);
	gettimeofday(&t1, NULL);
	size = cond_write(fd, wdata, length);
	if (size < length) {
		err = -EAGAIN;
		dbg_line("write %d/%d\n", size, length);
		goto release_wdata;
	}
	gettimeofday(&t2, NULL);
	wr_delta = time_interval_ms(&t1, &t2);
	disk->wr_time += wr_delta;
	disk->wr_size += disk->sectors;

	rdata = memalign(SECTOR_SIZE, length);
	if (!rdata) {
		err = -ENOMEM;
		dbg_line("mem align for read failed\n");
		goto release_wdata;
	}

	lseek(fd, offset, SEEK_SET);
	gettimeofday(&t1, NULL);
	size = cond_read(fd, rdata, length);
	if (size < length) {
		disk->rd_errors++;
		err = -EAGAIN;
		dbg_line("read not complete %d/%d\n", size, length);
		goto release_rdata;
	}
	gettimeofday(&t2, NULL);
	rd_delta = time_interval_ms(&t1, &t2);
	disk->rd_time += rd_delta;
	disk->rd_size += disk->sectors;

	err = cond_cmp(wdata, rdata, length);
	if (err) {
		disk->cmp_errors++;
		err = -EILSEQ;
		dbg_line("compare %d\n", err);
		dump_difference(wdata, rdata, length);
		goto release_rdata;
	}
	disk->pass_count++;

	dbg_lline("w %7.3fMB/s, r %7.3fMB/s\n",
		calc_speed(wr_delta, disk->sectors),
		calc_speed(rd_delta, disk->sectors));

release_rdata:
	free(rdata);
release_wdata:
	free(wdata);
close_file:
	close(fd);

	return err;
}

int veridisk_write_loop(struct veridisk *disk,
		int min, int max, int begin, int end)
{
	int i;
	int err = 0;

	if (min > max || min < 0 || begin > end || begin < 0)
		return -EINVAL;

	dbg_help("offset [%d, %d), len [%d, %d), %s pattern 0x%02x\n",
		begin, end, min, max, pattern_name(disk->pattern),
		disk->pattern & 0xFF);

	for (disk->offset = begin; disk->offset < end; disk->offset++) {
		for (i = min; i < max; i++) {
			if (pattern_type(disk->pattern) == SEQUENCE_PATTERN)
				set_pattern(disk, i);
			else if (pattern_type(disk->pattern) == RANDOM_PATTERN)
				set_pattern(disk, rand());
			disk->sectors = i;
			err = veridisk_write(disk);
			if (err && disk->quit_on_error)
				break;
			if (signal_int_count)
				goto show_result;
		}
		dbg_lline("pass %d/%d, w %7.3fMB/s, r %7.3fMB/s\n",
			disk->pass_count, disk->test_count,
			calc_speed(disk->wr_time, disk->wr_size),
			calc_speed(disk->rd_time, disk->rd_size));
		if (err && disk->quit_on_error)
			break;
	}

show_result:
	dbg_line("-------------------- TEST RESULT --------------------\n");
	dbg_line("pass %d/%d, w %7.3fMB/s, r %7.3fMB/s\n",
		disk->pass_count, disk->test_count,
		calc_speed(disk->wr_time, disk->wr_size),
		calc_speed(disk->rd_time, disk->rd_size));
	dbg_line("failed count: open %d, write %d, read %d, compare %d\n",
		disk->open_errors, disk->wr_errors,
		disk->rd_errors, disk->cmp_errors);

	return err;
}

int main(int argc, char **argv)
{
	struct veridisk disk = {0};
	struct sigaction sa = {0};
	int min = 1, max = 0, begin = 0, end = 0, pattern;
	int c;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = veridisk_sa;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		dbg_line("failed register INT signal handler\n");
		return -EINVAL;
	}

	disk.pattern = SEQUENCE_PATTERN;

	while ((c = getopt(argc, argv, "drwvqf:m:M:b:e:p:")) != -1) {
		switch (c) {
		case 'f':
			disk.name = optarg;
			break;
		case 'm':
			min = atol(optarg);
			break;
		case 'M':
			max = atol(optarg);
			break;
		case 'b':
			begin = atol(optarg);
			break;
		case 'e':
			end = atol(optarg);
			break;
		case 'p':
			pattern = atol(optarg);
			if (pattern >= 0 && pattern <= 0xFF) {
				disk.pattern = FIXED_PATTERN;
				set_pattern(&disk, pattern);
			} else {
				disk.pattern = RANDOM_PATTERN;
				set_pattern(&disk, pattern);
				srand(pattern);
			}
			break;
		case 'd':
			disk.direct = 1;
			break;
		case 'r':
			read_only = 1;
			break;
		case 'w':
			write_only = 1;
			break;
		case 'q':
			disk.quit_on_error = 1;
			break;
		case 'v':
			print_verbose_debug_line = 1;
			break;
		case 'h':
		case '?':
		default:
			goto show_help;
		}
	}

	if (!disk.name)
		goto show_help;

	max = max ? max : min + 1;
	end = end ? end : begin + 1;

	return veridisk_write_loop(&disk, min, max, begin, end);

show_help:
	dbg_help("Usage: veridisk -f filename [options]\n"
		"OPTIONS:\n"
		"  -m N		min sectors, default 1\n"
		"  -M N		max sectors, default min + 1\n"
		"  -b N		offset sector begin, default 0\n"
		"  -e N		offset sector end, default begin + 1\n"
		"  -p N		write data pattern, default sequence pattern\n"
		"		fixed pattern if N in range [0, 0xFF]\n"
		"		random pattern if N out of range [0, 0xFF]\n"
		"  -d		using direct mode, default is buffer mode\n"
		"  -r		read only, will not test write/check\n"
		"  -w		write only, will not test read/check\n"
		"  -v		print verbose message\n"
		"  -q		quit when error occur, default not quit\n");
	return 0;
}
