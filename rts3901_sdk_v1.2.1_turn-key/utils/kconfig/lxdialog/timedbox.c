/*
 *  timedbox.c -- implements the yes/no box
 *
 *  ORIGINAL AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *  MODIFIED FOR LINUX KERNEL CONFIG BY: William Roadcap (roadcap@cfw.com)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <signal.h>
#include "dialog.h"

WINDOW *dialog;

/*
 * Display termination buttons
 */
static void dialog_timedout(int signo)
{
	if (dialog != NULL)
		delwin(dialog);
	return;
}

/*
 * Display a dialog box
 */
int dialog_timedbox(const char *title, const char *prompt,
		    int height, int width, int sec)
{
	int i, x, y, key = 0;

do_resize:
	if (getmaxy(stdscr) < (height + 4))
		return -ERRDISPLAYTOOSMALL;
	if (getmaxx(stdscr) < (width + 4))
		return -ERRDISPLAYTOOSMALL;

	/* center dialog box on screen */
	x = (COLS - width) / 2;
	y = (LINES - height) / 2;

	draw_shadow(stdscr, y, x, height, width);

	dialog = newwin(height, width, y, x);
	keypad(dialog, TRUE);

	draw_box(dialog, 0, 0, height, width,
		 dlg.dialog.atr, dlg.border.atr);
	wattrset(dialog, dlg.border.atr);
	mvwaddch(dialog, height - 3, 0, ACS_LTEE);
	for (i = 0; i < width - 2; i++)
		waddch(dialog, ACS_HLINE);
	wattrset(dialog, dlg.dialog.atr);
	waddch(dialog, ACS_RTEE);

	print_title(dialog, title, width);

	wattrset(dialog, dlg.dialog.atr);
	print_autowrap(dialog, prompt, width - 2, 1, 3);
	wrefresh(dialog);

        signal(SIGALRM, dialog_timedout);
	alarm(sec);
	return 0;
}
