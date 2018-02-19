#ifndef _RTS_DBG_LOG_H_
#define _RTS_DBG_LOG_H_

extern FILE *realsil_log_file;
extern int realsil_log_flag;

int realsil_log_name(const char *logname);
int realsil_log_open();
int realsil_log_close();

#define DBG_LOG_LEVEL_0		(1<<0)

#define DBG_LOG_ERR(...) 			\
	do { 					\
		if (realsil_log_file) { 		\
			fprintf(realsil_log_file, "dbg_isp error: " __VA_ARGS__); \
			fflush(realsil_log_file); 	\
		} \
		fprintf(stderr, "dbg_isp error: " __VA_ARGS__); \
	} while (0)

#define DBG_LOG_WARN(...) 			\
	do { 					\
		if (realsil_log_file) { 		\
			fprintf(realsil_log_file, "dbg_isp warning: " __VA_ARGS__); \
			fflush(realsil_log_file); 	\
		} \
		if (realsil_log_flag){\
			fprintf(stderr, "dbg_isp warning: " __VA_ARGS__); \
		}\
	} while (0)

#define DBG_LOG_LOG(...) 			\
	do { 					\
		if (realsil_log_file) { 		\
			fprintf(realsil_log_file, "dbg_isp log: " __VA_ARGS__); \
			fflush(realsil_log_file); 	\
		} \
		if (realsil_log_flag){\
			fprintf(stdout, "dbg_isp log: " __VA_ARGS__); \
		}\
	} while (0)

#define DBG_LOG_PRINT(...)			\
	do { 					\
		fprintf(stdout, "dbg_isp log: " __VA_ARGS__); \
		if (realsil_log_file) { 		\
			fprintf(realsil_log_file, "dbg_isp log: " __VA_ARGS__); \
			fflush(realsil_log_file); \
		}		\
	} while (0)


#define DBG_LOG_OPT(...)			\
	do { 					\
		if (realsil_log_file) { 		\
			fprintf(realsil_log_file, __VA_ARGS__); \
			fflush(realsil_log_file); 	\
		} \
		fprintf(stdout,  __VA_ARGS__); \
	} while (0)

#define DBG_LOG_SHOW(level, ...)		\
	do { 					\
		if (realsil_log_file) { 		\
			fprintf(realsil_log_file, __VA_ARGS__); \
			fflush(realsil_log_file); 	\
		} \
		if (realsil_log_flag & (1<<level)){\
			fprintf(stdout,  __VA_ARGS__); \
		} \
	} while (0)

#endif
