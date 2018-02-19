#ifndef FT2_TEST_DISK_H
#define FT2_TEST_DISK_H
int ft2_disk_check(struct protocol_command * pcmd);
int ft2_disk_runonce(void * priv, struct protocol_command * pcmd, char * path);

#endif /* FT2_TEST_DISK_H */
