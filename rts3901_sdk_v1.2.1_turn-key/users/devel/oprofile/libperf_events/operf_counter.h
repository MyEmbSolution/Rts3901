/**
 * @file pe_profiling/operf_counter.h
 * C++ class definition that abstracts the user-to-kernel interface
 * for using Linux Performance Events Subsystem.
 *
 * @remark Copyright 2011 OProfile authors
 * @remark Read the file COPYING
 *
 * Created on: Dec 7, 2011
 * @author Maynard Johnson
 * (C) Copyright IBM Corp. 2011
 *
 * Modified by Maynard Johnson <maynardj@us.ibm.com>
 * (C) Copyright IBM Corporation 2012
 *
 */

#ifndef OPERF_COUNTER_H_
#define OPERF_COUNTER_H_

#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <poll.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <limits.h>
#include <istream>
#include <fstream>
#include "operf_event.h"
#include "op_cpu_type.h"
#include "operf_utils.h"

extern char * start_time_human_readable;

class operf_record;

#define OP_BASIC_SAMPLE_FORMAT (PERF_SAMPLE_ID | PERF_SAMPLE_IP \
    | PERF_SAMPLE_TID)

static inline int
op_perf_event_open(struct perf_event_attr * attr,
		      pid_t pid, int cpu, int group_fd,
		      unsigned long flags)
{
	return syscall(__NR_perf_event_open, attr, pid, cpu,
			       group_fd, flags);
}

#define OP_PERF_HANDLED_ERROR -101


class operf_counter {
public:
	operf_counter(operf_event_t & evt, bool enable_on_exec, bool callgraph,
	              bool separate_by_cpu);
	~operf_counter();
	int perf_event_open(pid_t ppid, int cpu, unsigned counter, operf_record * pr);
	const struct perf_event_attr * the_attr(void) const { return &attr; }
	int get_fd(void) const { return fd; }
	int get_id(void) const { return id; }
	const std::string get_event_name(void) const { return event_name; }

private:
	struct perf_event_attr attr;
	int fd;
	int id;
	std::string event_name;
};


class operf_record {
public:
	/* For system-wide profiling, set sys_wide=true, the_pid=-1, and pid_running=false.
	 * For single app profiling, set sys_wide=false, the_pid=<processID-to-profile>,
	 * and pid_running=true if profiling an already active process; otherwise false.
	 */
	operf_record(int output_fd, bool sys_wide, pid_t the_pid, bool pid_running,
	             std::vector<operf_event_t> & evts, OP_perf_utils::vmlinux_info_t vi,
	             bool callgraph, bool separate_by_cpu, bool output_fd_is_file);
	~operf_record();
	void recordPerfData(void);
	int out_fd(void) const { return output_fd; }
	void add_to_total(int n) { total_bytes_recorded += n; }
	int get_total_bytes_recorded(void) const { return total_bytes_recorded; }
	void register_perf_event_id(unsigned counter, u64 id, perf_event_attr evt_attr);
	bool get_valid(void) { return valid; }

private:
	void create(std::string outfile, std::vector<operf_event_t> & evts);
	void setup(void);
	int prepareToRecord(int cpu, int fd);
	void write_op_header_info(void);
	int _write_header_to_file(void);
	int _write_header_to_pipe(void);
	int output_fd;
	bool write_to_file;
	struct pollfd * poll_data;
	std::vector<struct mmap_data> samples_array;
	int num_cpus;
	pid_t pid;
	bool pid_started;
	bool system_wide;
	bool callgraph;
	bool separate_cpu;
	std::vector< std::vector<operf_counter> > perfCounters;
	int total_bytes_recorded;
	int poll_count;
	struct OP_header opHeader;
	std::vector<operf_event_t> evts;
	bool valid;
	std::string vmlinux_file;
	u64 kernel_start, kernel_end;
};

class operf_read {
public:
	operf_read(void) : sample_data_fd(-1), inputFname(""), cpu_type(CPU_NO_GOOD) { valid = syswide = false;}
	void init(int sample_data_pipe_fd, std::string input_filename, std::string samples_dir, op_cpu cputype,
	          std::vector<operf_event_t> & evts, bool systemwide);
	~operf_read();
	int readPerfHeader(void);
	int convertPerfData(void);
	bool is_valid(void) {return valid; }
	int get_eventnum_by_perf_event_id(u64 id) const;
	inline const operf_event_t * get_event_by_counter(u32 counter) { return &evts[counter]; }

private:
	int sample_data_fd;
	std::string inputFname;
	std::string sampledir;
	std::ifstream istrm;
	struct OP_header opHeader;
	std::vector<operf_event_t> evts;
	bool valid;
	bool syswide;
	op_cpu cpu_type;
	int _get_one_perf_event(event_t *);
	int _read_header_info_with_ifstream(void);
	int _read_perf_header_from_file(void);
	int _read_perf_header_from_pipe(void);
};


#endif /* OPERF_COUNTER_H_ */
