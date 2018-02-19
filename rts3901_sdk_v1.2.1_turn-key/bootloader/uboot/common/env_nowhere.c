/*
 * (C) Copyright 2000-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <spi_flash.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_ENV_SPI_BUS
# define CONFIG_ENV_SPI_BUS	0
#endif
#ifndef CONFIG_ENV_SPI_CS
# define CONFIG_ENV_SPI_CS	0
#endif
#ifndef CONFIG_ENV_SPI_MAX_HZ
# define CONFIG_ENV_SPI_MAX_HZ	1000000
#endif
#ifndef CONFIG_ENV_SPI_MODE
# define CONFIG_ENV_SPI_MODE	SPI_MODE_3
#endif

extern struct spi_flash *flash;

env_t *env_ptr;

void env_relocate_spec(void)
{
	struct spi_flash *new;

	new = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
			CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);

	if (flash)
		spi_flash_free(flash);
	flash = new;
}

/*
 * Initialize Environment use
 *
 * We are still running from ROM, so data use is limited
 */
int env_init(void)
{
	gd->env_addr	= (ulong)&default_environment[0];
	gd->env_valid	= 0;

	return 0;
}

int  exit_SPI_QPI_mode(void)
{
	int ret = 0;

	if (flash)
		ret = spi_flash_exit_QPI(flash);
	return ret;
}

unsigned int read_spi_flash(unsigned int offset)
{

	if (flash)
		return spi_flash_read_word(flash, offset);
	else
		return 0xDEADBEAF;
}
