#ifndef FT2_TEST_SD_H
#define FT2_TEST_SD_H /* FT2_TEST_SD_H */

int ft2_sd_check(struct protocol_command *pcmd);
int ft2_sd_runonce(void *priv, struct protocol_command *pcmd, char *path);

#endif /* FT2_TEST_SD_H */
