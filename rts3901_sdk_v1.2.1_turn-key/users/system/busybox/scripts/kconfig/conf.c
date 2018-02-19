/*
 * Copyright (C) 2002 Roman Zippel <zippel@linux-m68k.org>
 * Released under the terms of the GNU GPL v2.0.
 */

#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define LKC_DIRECT_LINK
#include "lkc.h"

static void conf(struct menu *menu);
static void check_conf(struct menu *menu);

enum {
	ask_all,
	ask_new,
	ask_silent,
	set_default,
	set_yes,
	set_mod,
	set_no,
	set_random
} input_mode = ask_all;
char *defconfig_file;

static int indent = 1;
static int valid_stdin = 1;
static int conf_cnt;
static char line[128];
static struct menu *rootEntry;

static char nohelp_text[] = N_("Sorry, no help available for this option yet.\n");

static void strip(char *str)
{
	char *p = str;
	int l;

	while ((isspace(*p)))
		p++;
	l = strlen(p);
	if (p != str)
		memmove(str, p, l + 1);
	if (!l)
		return;
	p = str + l - 1;
	while ((isspace(*p)))
		*p-- = 0;
}

static void conf_askvalue(struct symbol *sym, const char *def)
{
	enum symbol_type type = sym_get_type(sym);

	line[0] = '\n';
	line[1] = 0;

	if (!sym_is_changable(sym)) {
		line[0] = '\n';
		line[1] = 0;
		return;
	}

	if (sym_has_value(sym)) {
		return;
	}

	switch (type) {
	case S_INT:
	case S_HEX:
	case S_STRING:
		return;
	default:
		;
	}
	if (sym_tristate_within_range(sym, no)) {
		line[0] = 'n';
		line[1] = '\n';
		line[2] = 0;
	}
}

int conf_string(struct menu *menu)
{
	struct symbol *sym = menu->sym;
	const char *def;

	while (1) {
		def = sym_get_string_value(sym);
		conf_askvalue(sym, def);
		switch (line[0]) {
		case '\n':
			break;
		case '?':
			/* print help */
			if (line[1] == '\n') {
				printf("\n%s\n", menu->sym->help ? menu->sym->help : nohelp_text);
				def = NULL;
				break;
			}
		default:
			line[strlen(line)-1] = 0;
			def = line;
		}
		if (def && sym_set_string_value(sym, def))
			return 0;
	}
}

static int conf_sym(struct menu *menu)
{
	struct symbol *sym = menu->sym;
	tristate oldval, newval;
	const char *help;

	while (1) {
		oldval = sym_get_tristate_value(sym);
		conf_askvalue(sym, sym_get_string_value(sym));
		strip(line);

		switch (line[0]) {
		case 'n':
		case 'N':
			newval = no;
			if (!line[1] || !strcmp(&line[1], "o"))
				break;
			continue;
		case 'm':
		case 'M':
			newval = mod;
			if (!line[1])
				break;
			continue;
		case 'y':
		case 'Y':
			newval = yes;
			if (!line[1] || !strcmp(&line[1], "es"))
				break;
			continue;
		case 0:
			newval = oldval;
			break;
		case '?':
			goto help;
		default:
			continue;
		}
		if (sym_set_tristate_value(sym, newval))
			return 0;
help:
		help = nohelp_text;
		if (sym->help)
			help = sym->help;
		printf("\n%s\n", help);
	}
}

static int conf_choice(struct menu *menu)
{
	struct symbol *sym, *def_sym;
	struct menu *child;
	bool is_new;

	sym = menu->sym;
	is_new = !sym_has_value(sym);
	if (sym_is_changable(sym)) {
		conf_sym(menu);
		sym_calc_value(sym);
		switch (sym_get_tristate_value(sym)) {
		case no:
			return 1;
		case mod:
			return 0;
		case yes:
			break;
		}
	} else {
		switch (sym_get_tristate_value(sym)) {
		case no:
			return 1;
		case mod:
			printf("%*s%s\n", indent - 1, "", menu_get_prompt(menu));
			return 0;
		case yes:
			break;
		}
	}

	while (1) {
		int cnt, def;

		def_sym = sym_get_choice_value(sym);
		cnt = def = 0;
		line[0] = 0;
		for (child = menu->list; child; child = child->next) {
			if (!menu_is_visible(child))
				continue;
			if (!child->sym) {
				continue;
			}
			cnt++;
			if (child->sym == def_sym)
				def = cnt;
		}
		if (cnt == 1) {
			goto conf_childs;
		}

		cnt = def;

	conf_childs:
		for (child = menu->list; child; child = child->next) {
			if (!child->sym || !menu_is_visible(child))
				continue;
			if (!--cnt)
				break;
		}
		if (!child)
			continue;
		if (strlen(line) > 0 && line[strlen(line) - 1] == '?') {
			printf("\n%s\n", child->sym->help ?
				child->sym->help : nohelp_text);
			continue;
		}
		sym_set_choice_value(sym, child->sym);
		if (child->list) {
			indent += 2;
			conf(child->list);
			indent -= 2;
		}
		return 1;
	}
}

static void conf(struct menu *menu)
{
	struct symbol *sym;
	struct property *prop;
	struct menu *child;

	if (!menu_is_visible(menu))
		return;

	sym = menu->sym;
	prop = menu->prompt;
	if (prop) {
		switch (prop->type) {
		case P_MENU:
			if (input_mode == ask_silent && rootEntry != menu) {
				check_conf(menu);
				return;
			}
		default:
			;
		}
	}

	if (!sym)
		goto conf_childs;

	if (sym_is_choice(sym)) {
		conf_choice(menu);
		if (sym->curr.tri != mod)
			return;
		goto conf_childs;
	}

	switch (sym->type) {
	case S_INT:
	case S_HEX:
	case S_STRING:
		conf_string(menu);
		break;
	default:
		conf_sym(menu);
		break;
	}

conf_childs:
	if (sym)
		indent += 2;
	for (child = menu->list; child; child = child->next)
		conf(child);
	if (sym)
		indent -= 2;
}

static void check_conf(struct menu *menu)
{
	struct symbol *sym;
	struct menu *child;

	if (!menu_is_visible(menu))
		return;

	sym = menu->sym;
	if (sym && !sym_has_value(sym)) {
		if (sym_is_changable(sym) ||
		    (sym_is_choice(sym) && sym_get_tristate_value(sym) == yes)) {
			conf_cnt++;
			rootEntry = menu_get_parent_menu(menu);
			conf(rootEntry);
		}
	}

	for (child = menu->list; child; child = child->next)
		check_conf(child);
}

int main(int ac, char **av)
{
	int i = 1;
	const char *name;
	struct stat tmpstat;

	if (ac > i && av[i][0] == '-') {
		switch (av[i++][1]) {
		case 'o':
			input_mode = ask_new;
			break;
		case 's':
			input_mode = ask_silent;
			valid_stdin = isatty(0); //bbox: && isatty(1) && isatty(2);
			break;
		case 'd':
			input_mode = set_default;
			break;
		case 'D':
			input_mode = set_default;
			defconfig_file = av[i++];
			if (!defconfig_file) {
				printf(_("%s: No default config file specified\n"),
					av[0]);
				exit(1);
			}
			break;
		case 'n':
			input_mode = set_no;
			break;
		case 'm':
			input_mode = set_mod;
			break;
		case 'y':
			input_mode = set_yes;
			break;
		case 'r':
			input_mode = set_random;
			srandom(time(NULL));
			break;
		case 'h':
		case '?':
			fprintf(stderr, "See README for usage info\n");
			exit(0);
		}
	}
	name = av[i];
	if (!name) {
		printf(_("%s: Kconfig file missing\n"), av[0]);
	}
	conf_parse(name);
	//zconfdump(stdout);
	switch (input_mode) {
	case set_default:
		if (!defconfig_file)
			defconfig_file = conf_get_default_confname();
		if (conf_read(defconfig_file)) {
			printf("***\n"
				"*** Can't find default configuration \"%s\"!\n"
				"***\n", defconfig_file);
			exit(1);
		}
		break;
	case ask_silent:
		if (stat(".config", &tmpstat)) {
			printf(_("***\n"
				"*** You have not yet configured busybox!\n"
				"***\n"
				"*** Please run some configurator (e.g. \"make oldconfig\" or\n"
				"*** \"make menuconfig\" or \"make defconfig\").\n"
				"***\n"));
			exit(1);
		}
	case ask_all:
	case ask_new:
		conf_read(NULL);
		break;
	case set_no:
	case set_mod:
	case set_yes:
	case set_random:
		name = getenv("KCONFIG_ALLCONFIG");
		if (name && !stat(name, &tmpstat)) {
			conf_read_simple(name);
			break;
		}
		switch (input_mode) {
		case set_no:	 name = "allno.config"; break;
		case set_mod:	 name = "allmod.config"; break;
		case set_yes:	 name = "allyes.config"; break;
		case set_random: name = "allrandom.config"; break;
		default: break;
		}
		if (!stat(name, &tmpstat))
			conf_read_simple(name);
		else if (!stat("all.config", &tmpstat))
			conf_read_simple("all.config");
		break;
	default:
		break;
	}

	if (input_mode != ask_silent) {
		rootEntry = &rootmenu;
		conf(&rootmenu);
		if (input_mode == ask_all) {
			input_mode = ask_silent;
			valid_stdin = 1;
		}
	}
	do {
		conf_cnt = 0;
		check_conf(&rootmenu);
	} while (conf_cnt);
	if (conf_write(NULL)) {
		fprintf(stderr, _("\n*** Error during writing of the configuration.\n\n"));
		return 1;
	}
	return 0;
}
