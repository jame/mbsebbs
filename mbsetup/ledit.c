/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Line Editor
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"



int		sock;			/* Connected socket		   */


int yes_no(char *T_)
{
	char t[256];
	int ch;

	strcpy(t, T_);
	strcat(t, " Y/n ");
	set_color(LIGHTGRAY, BLACK);
	locate(LINES - 3, 1);
	clrtoeol();
	mvprintw(LINES -3, 6, t);
	do {
		ch = toupper(readkey(LINES - 3, strlen(t) + 6, LIGHTGRAY, BLACK));
	} while (ch != KEY_ENTER && ch != 'Y' && ch != 'N' && ch != '\012');
	locate(LINES - 3, 6);
	clrtoeol();
	if (ch == KEY_ENTER || ch == 'Y' || ch == KEY_LINEFEED)
		return 1;
	else
		return 0;
}



void errmsg(const char *format, ...)
{
	char	*t;
	int	ch;
	va_list	va_ptr;

	t = calloc(256, sizeof(char));

	va_start(va_ptr, format);
	vsprintf(t, format, va_ptr);
	va_end(va_ptr);

	t = xstrcat(t, (char *)", Press any key ");
	set_color(LIGHTGRAY, BLACK);
	locate(LINES - 3, 1);
	clrtoeol();
	mvprintw(LINES - 3, 6, t);
	putchar(7);
	ch = readkey(LINES - 3, strlen(t) + 6, LIGHTGRAY, BLACK);
	locate(LINES - 3, 6);
	clrtoeol();
	free(t);
}



/*
 * Safe field display, does not format % characters but displays it.
 */
void show_field(int y, int x, char *str, int length, int fill)
{
    int	    i;

    locate(y, x);
    for (i = 0; i < strlen(str); i++)
	putchar(str[i]);
    if (strlen(str) < length)
	for (i = strlen(str); i < length; i++)
	    putchar(fill);
}



int insertflag = 0;

void newinsert(int i, int fg, int bg)
{
	insertflag = i;
	set_color(YELLOW, RED);
	if (insertflag != 0) {
		mvprintw(2,36," INS ");
	} else {
		mvprintw(2,36," OVR ");
	}
	set_color(fg, bg);
}




char *edit_field(int y, int x, int w, int p, char *s_)
{
	int		i, charok, first, curpos, AllSpaces;
	static char	s[256];
	unsigned int	ch;

	memset((char *)s, 0, 256);
	sprintf(s, "%s", s_);
	curpos = 0;
	first = 1;
	newinsert(1, YELLOW, BLUE);

	do {
		set_color(YELLOW, BLUE);
		show_field(y, x, s, w, '_');
		locate(y, x + curpos);
		do {
			ch = readkey(y, x + curpos, YELLOW, BLUE);
			set_color(YELLOW, BLUE);

			/*
			 *  Test if the pressed key is a valid key.
			 */
			charok = 0;
			if ((ch >= ' ') && (ch <= '~')) {
				switch(p) {
				case '!':	
					ch = toupper(ch);
					charok = 1;
					break;
				case 'X':
					charok = 1;
					break;
				case '9':
					if (ch == ' ' || ch == '-' || ch == ',' ||
					    ch == '.' || isdigit(ch)) 
						charok = 1;
					break;
				case 'U':
					ch = toupper(ch);
					if (isupper(ch))
						charok = 1;
					break;
				default:
					putchar(7);
					break;
				}
			}

		} while (charok == 0 && ch != KEY_ENTER && ch != KEY_LINEFEED &&
			   ch != KEY_DEL && ch != KEY_INS && ch != KEY_HOME &&
			   ch != KEY_LEFT && ch != KEY_RIGHT && ch != KEY_ESCAPE &&
			   ch != KEY_BACKSPACE && ch != KEY_RUBOUT && ch != KEY_END);


		if (charok == 1) {
			if (first == 1) {
				first = 0;
				memset((char *)s, 0, 256);
				curpos = 0;
			}
			if (curpos < w) {
				if (insertflag == 1) {
					/*
					 *  Insert mode 
					 */
					if (strlen(s) < w) {
						if (curpos < strlen(s)) {
							for (i = strlen(s); i >= curpos; i--)
								s[i+1] = s[i];
						} 
						s[curpos] = ch;
						if (curpos < w)
							curpos++;
					} else {
						putchar(7);
					}
				} else {
					/*
					 *  Overwrite mode
					 */
					s[curpos] = ch;
					if (curpos < w)
						curpos++;
				}
			} else {
				/*
				 *  The field is full 
				 */
				putchar(7);
			}
		} /* if charok */

		first = 0;
		switch (ch) {
		case KEY_HOME:
				curpos = 0;	
				break;
		case KEY_END:
				curpos = strlen(s);
				break;
		case KEY_LEFT:
				if (curpos > 0)
					curpos--;
				else
					putchar(7);
				break;
		case KEY_RIGHT:
				if (curpos < strlen(s))
					curpos++;
				else
					putchar(7);
				break;
		case KEY_INS:
				if (insertflag == 1)
					newinsert(0, YELLOW, BLUE);
				else
					newinsert(1, YELLOW, BLUE);
				break;
		case KEY_BACKSPACE:
				if (strlen(s) > 0) {
					if (curpos >= strlen(s)) {
						curpos--;
						s[curpos] = '\0';
					} else {
						for (i = curpos; i < strlen(s); i++)
							s[i] = s[i+1];
						s[i] = '\0';
					}
				} else
					putchar(7);
				break;
		case KEY_RUBOUT:
		case KEY_DEL:
				if (strlen(s) > 0) {
					if ((curpos) == (strlen(s) -1)) {
						s[curpos] = '\0';
					} else {
						for (i = curpos; i < strlen(s); i++)
							s[i] = s[i+1];
						s[i] = '\0';
					}
				} else
					putchar(7);
				break;
		}
	} while ((ch != KEY_ENTER) && (ch != KEY_LINEFEED) && (ch != KEY_ESCAPE));

	set_color(LIGHTGRAY, BLUE);
	mvprintw(2,36, "     ");
	set_color(LIGHTGRAY, BLACK);
	if (strlen(s)) {
		AllSpaces = TRUE;
		for (i = 0; i < strlen(s); i++) {
			if (s[i] != ' ')
				AllSpaces = FALSE;
		}
		if (AllSpaces)
			s[0] = '\0';
	}
	return s;
}



char *select_show(int max)
{
	static	char s[12];
	static	char *menu=(char *)"-";
	char	help[81];

	memset((char *)s, 0, 12);

	if (max == 0)
		sprintf(help, "Select ^\"-\"^ for previous level");
	else
		if (max > 10)
			sprintf(help, "Select ^\"-\"^ for previous level, ^\"P\" or \"N\"^ to page");
		else
			sprintf(help, "Select ^\"-\"^ for previous level");
	showhelp(help);

	/*
	 * Loop until the answer is right
	 */
	for (;;) {
		mvprintw(LINES - 3, 6, "Enter your choice >");
		menu = (char *)"-";
		menu = edit_field(LINES - 3, 26, 6, '!', menu);
		locate(LINES - 3, 6);
		clrtoeol();

		if (strncmp(menu, "-", 1) == 0)
			break;
		if (max > 10) {
			if (strncmp(menu, "N", 1) == 0)
				break;
			if (strncmp(menu, "P", 1) == 0)
				break;
		}

		working(2, 0, 0);
		working(0, 0, 0);
	}
	
	return menu;
}



char *select_record(int max, int items)
{
	static	char s[12];
	static	char *menu=(char *)"-";
	char	help[81];
	int	pick;

	memset((char *)s, 0, 12);

	if (max == 0)
		sprintf(help, "Select ^\"-\"^ for previous level, ^\"A\"^ to append first record");
	else
		if (max > items)
			sprintf(help, "Record (1..%d), ^\"-\"^ prev. level, ^\"A\"^ Append record, ^\"P\" or \"N\"^ to page", max);
		else
			sprintf(help, "Select record (1..%d), ^\"-\"^ for previous level, ^\"A\"^ to append a new record", max);
	showhelp(help);

	/*
	 * Loop until the answer is right
	 */
	for (;;) {
		mvprintw(LINES - 3, 6, "Enter your choice >");
		menu = (char *)"-";
		menu = edit_field(LINES - 3, 26, 6, '!', menu);
		locate(LINES - 3, 6);
		clrtoeol();

		if (strncmp(menu, "A", 1) == 0)
			break;
		if (strncmp(menu, "-", 1) == 0)
			break;
		if (strncmp(menu, "M", 1) == 0)
			break;
		if (max > items) {
			if (strncmp(menu, "N", 1) == 0)
				break;
			if (strncmp(menu, "P", 1) == 0)
				break;
		}
		pick = atoi(menu);
		if ((pick >= 1) && (pick <= max))
			break;

		working(2, 0, 0);
		working(0, 0, 0);
	}
	
	return menu;
}



char *select_area(int max, int items)
{
	static  char s[12];
	static  char *menu=(char *)"-";
	char    help[81];
	int     pick;

	memset((char *)s, 0, 12);

	if (max == 0)
		sprintf(help, "^\"-\"^ back, ^A^ppend");
	else
		if (max > items)
			sprintf(help, "Record (1..%d), ^\"-\"^ back, ^A^ppend, ^G^lobal, ^M^ove, ^N^ext, ^P^revious", max);
		else
			sprintf(help, "Record (1..%d), ^\"-\"^ back, ^A^ppend, ^G^lobal, ^M^ove", max);
	showhelp(help);

	/*
	 * Loop until the answer is right
	 */
	for (;;) {
		mvprintw(LINES - 3, 6, "Enter your choice >");
		menu = (char *)"-";
		menu = edit_field(LINES - 3, 26, 6, '!', menu);
		locate(LINES - 3, 6);
		clrtoeol();

		if (strncmp(menu, "A", 1) == 0)
			break;
		if (strncmp(menu, "-", 1) == 0)
			break;
		if ((strncmp(menu, "M", 1) == 0) && max)
			break;
		if ((strncmp(menu, "G", 1) == 0) && max)
			break;
		if (max > items) {
			if (strncmp(menu, "N", 1) == 0)
				break;
			if (strncmp(menu, "P", 1) == 0)
				break;
		}
		pick = atoi(menu);
		if ((pick >= 1) && (pick <= max))
			break;
	}

	return menu;
}



char *select_pick(int max, int items)
{
	static	char s[12];
	static	char *menu=(char *)"-";
	char	help[81];
	int	pick;

	memset((char *)s, 0, 12);

	if (max == 0)
		sprintf(help, "Select ^\"-\"^ for previous level");
	else
		if (max > items)
			sprintf(help, "Record (1..%d), ^\"-\"^ prev. level, ^\"P\" or \"N\"^ to page", max);
		else
			sprintf(help, "Select record (1..%d), ^\"-\"^ for previous level", max);
	showhelp(help);

	/*
	 * Loop until the answer is right
	 */
	for (;;) {
		mvprintw(LINES - 3, 6, "Enter your choice >");
		menu = (char *)"-";
		menu = edit_field(LINES - 3, 26, 6, '!', menu);
		locate(LINES - 3, 6);
		clrtoeol();

		if (strncmp(menu, "-", 1) == 0)
			break;
		if (max > items) {
			if (strncmp(menu, "N", 1) == 0)
				break;
			if (strncmp(menu, "P", 1) == 0)
				break;
		}
		pick = atoi(menu);
		if ((pick >= 1) && (pick <= max))
			break;

		working(2, 0, 0);
		working(0, 0, 0);
	}
	
	return menu;
}




/* 
 * Select menu, max is the highest item to pick. Returns zero if
 * "-" (previous level) is selected, -2 and -1 for the N and P keys.
 */
int select_menu_sub(int, int, char *);

int select_menu(int max)
{
	return select_menu_sub(max, 50, (char *)"Select menu item");
}

int select_tag(int max)
{
	return select_menu_sub(max, 40, (char *)"Toggle item");
}


int select_menu_sub(int max, int items, char *hlp)
{
	static char	*menu=(char *)"-";
	char		help[81];
	int		pick;

	if (max == 0)
	    sprintf(help, "Select ^\"-\"^ for previous level");
	else
	    if (max > items)
		sprintf(help, "%s (1..%d), ^\"-\"^ prev. level, ^\"P\" or \"N\"^ to page", hlp, max);
	    else
		sprintf(help, "%s (1..%d), ^\"-\"^ for previous level", hlp, max);
	showhelp(help);

	/* 
	 * Loop forever until it's right.
	 */
	for (;;) {
		mvprintw(LINES - 3, 6, "Enter your choice >");
		menu = (char *)"-";
		menu = edit_field(LINES - 3, 26, 3, '!', menu);
		locate(LINES -3, 6);
		clrtoeol();

		if (strncmp(menu, "-", 1) == 0) 
			return 0;

		if (max > items) {
		    if (strncmp(menu, "N", 1) == 0)
			return -1;
		    if (strncmp(menu, "P", 1) == 0)
			return -2;
		}

		pick = atoi(menu);
		if ((pick >= 1) && (pick <= max)) 
			return pick;

		working(2, 0, 0);
		working(0, 0, 0);
	}
}



void show_str(int y, int x, int l, char *line)
{
	show_field(y, x, line, l, ' ');
}



char *edit_str(int y, int x, int l, char *line, char *help)
{
	static char s[256];

	showhelp(help);
	memset((char *)s, 0, 256);
	strcpy(s, edit_field(y, x, l, 'X', line));
	set_color(WHITE, BLACK);
	show_str(y, x, l, s);
	return s;
}



char *edit_pth(int y, int x, int l, char *line, char *help)
{
	static	char s[256];
	char	*temp;

	showhelp(help);
	memset((char *)s, 0, 256);
	strcpy(s, edit_field(y, x, l, 'X', line));
	temp = xstrcpy(s);
	if (strlen(temp)) {
		temp = xstrcat(temp, (char *)"/foobar");
		if (access(s, R_OK)) {
			if (yes_no((char *)"Path doesn't exist, create"))
				if (! mkdirs(temp, 0775))
					errmsg((char *)"Can't create path");
		}
	}
	free(temp);
	set_color(WHITE, BLACK);
	show_str(y, x, l, s);
	return s;
}



char *edit_jam(int y, int x, int l, char *line, char *help)
{
	static	char s[256];
	char	*temp;

	showhelp(help);
	memset((char *)s, 0, 256);
	strcpy(s, edit_field(y, x, l, 'X', line));

	/*
	 * Check if the messagebase exists, if not, create it.
	 */
	temp = xstrcpy(s);
	temp = xstrcat(temp, (char *)".jhr");
	if (access(temp, W_OK)) {
		if (mkdirs(s, 0770)) {
			if (yes_no((char *)"Messagebase doesn't exist, create")) {
				if (Msg_Open(s))
					Msg_Close();
			}
		} else {
			errmsg((char *)"Can't create directory");
		}
	}
	free(temp);

	set_color(WHITE, BLACK);
	show_str(y, x, l, s);
	return s;
}



char *edit_ups(int y, int x, int l, char *line, char *help)
{
	static char s[256];

	showhelp(help);
	memset((char *)s, 0, 256);
	strcpy(s, edit_field(y, x, l, '!', line));
	set_color(WHITE, BLACK);
	show_str(y, x, l, s);
	return s;
}



char *getboolean(int val)
{
	if (val)
		return (char *)"Yes";
	else
		return (char *)"No ";
}



void show_bool(int y, int x, int val)
{
	mvprintw(y, x, getboolean(val));
}



int edit_bool(int y, int x, int val, char *help)
{
	int	ch;
	char	*temp;

	temp = xstrcpy(help);
	temp = xstrcat(temp, (char *)" (Spacebar = toggle)");
	showhelp(temp);
	free(temp);

	do {
		set_color(YELLOW, BLUE);
		show_bool(y, x, val);
		ch = readkey(y, x, YELLOW, BLUE);
		if (ch == ' ') 
			val ^= 1;
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_bool(y, x, val);
	fflush(stdout);
	return val;
}



char *getloglevel(long val)
{
	char	*p;

	p = xstrcpy((char *)"?");

	if (val & DLOG_ATTENT)	p = xstrcat(p, (char *)"!");
	if (val & DLOG_NORMAL)	p = xstrcat(p, (char *)"+");
	if (val & DLOG_VERBOSE)	p = xstrcat(p, (char *)"-");
	if (val & DLOG_TCP)	p = xstrcat(p, (char *)"A");
	if (val & DLOG_BBS)	p = xstrcat(p, (char *)"B");
	if (val & DLOG_CHAT)	p = xstrcat(p, (char *)"C");
	if (val & DLOG_DEVIO)	p = xstrcat(p, (char *)"D");
	if (val & DLOG_EXEC)	p = xstrcat(p, (char *)"E");
	if (val & DLOG_FILEFWD)	p = xstrcat(p, (char *)"F");
	if (val & DLOG_HYDRA)	p = xstrcat(p, (char *)"H");
	if (val & DLOG_IEMSI)	p = xstrcat(p, (char *)"I");
	if (val & DLOG_LOCK)	p = xstrcat(p, (char *)"L");
	if (val & DLOG_MAIL)	p = xstrcat(p, (char *)"M");
	if (val & DLOG_NEWS)	p = xstrcat(p, (char *)"N");
	if (val & DLOG_OUTSCAN)	p = xstrcat(p, (char *)"O");
	if (val & DLOG_PACK)	p = xstrcat(p, (char *)"P");
	if (val & DLOG_ROUTE)	p = xstrcat(p, (char *)"R");
	if (val & DLOG_SESSION)	p = xstrcat(p, (char *)"S");
	if (val & DLOG_TTY)	p = xstrcat(p, (char *)"T");
	if (val & DLOG_XMODEM)	p = xstrcat(p, (char *)"X");
	if (val & DLOG_ZMODEM)	p = xstrcat(p, (char *)"Z");

	return p;
}



void show_logl(int y, int x, long val)
{
	char	*p;

	p = getloglevel(val);
	show_field(y, x, p, 21, ' ');
	free(p);
}



long edit_logl(long val, char *txt)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw(5, 5, (char *)"%s EDIT LOGLEVEL", txt);
	set_color(CYAN, BLACK);
	mvprintw( 5,45, "Logflags");

	mvprintw( 7, 5, "1.  ! Attention");
	mvprintw( 8, 5, "2.  + Normal");
	mvprintw( 9, 5, "3.  - Verbose");
	mvprintw(10, 5, "4.  A Debug TCP");
	mvprintw(11, 5, "5.  B Debug BBS/binkp");
	mvprintw(12, 5, "6.  C Chat modems");
	mvprintw(13, 5, "7.  D Device IO");
	mvprintw(14, 5, "8.  E Execute");
	mvprintw(15, 5, "9.  F File forward");
	mvprintw(16, 5, "10. H Hydra debug");
	mvprintw(17, 5, "11. I EMSI debug");
	mvprintw( 7,45, "12. L Locking");
	mvprintw( 8,45, "13. M Mail debug");
	mvprintw( 9,45, "14. N News debug");
	mvprintw(10,45, "15. O Outboundscan");
	mvprintw(11,45, "16. P Packing");
	mvprintw(12,45, "17. R Routing");
	mvprintw(13,45, "18. S Session");
	mvprintw(14,45, "19. T TTY debug");
	mvprintw(15,45, "20. X Xmodem debug");
	mvprintw(16,45, "21. Z Zmodem debug");

	for (;;) {
		set_color(WHITE, BLACK);
		show_logl(5, 54, val);
		show_lbit( 7,24, val, DLOG_ATTENT);
		show_lbit( 8,24, val, DLOG_NORMAL);
		show_lbit( 9,24, val, DLOG_VERBOSE);
		show_lbit(10,24, val, DLOG_TCP);
		show_lbit(11,24, val, DLOG_BBS);
		show_lbit(12,24, val, DLOG_CHAT);
		show_lbit(13,24, val, DLOG_DEVIO);
		show_lbit(14,24, val, DLOG_EXEC);
		show_lbit(15,24, val, DLOG_FILEFWD);
		show_lbit(16,24, val, DLOG_HYDRA);
		show_lbit(17,24, val, DLOG_IEMSI);
		show_lbit( 7,64, val, DLOG_LOCK);
		show_lbit( 8,64, val, DLOG_MAIL);
		show_lbit( 9,64, val, DLOG_NEWS);
		show_lbit(10,64, val, DLOG_OUTSCAN);
		show_lbit(11,64, val, DLOG_PACK);
		show_lbit(12,64, val, DLOG_ROUTE);
		show_lbit(13,64, val, DLOG_SESSION);
		show_lbit(14,64, val, DLOG_TTY);
		show_lbit(15,64, val, DLOG_XMODEM);
		show_lbit(16,64, val, DLOG_ZMODEM);

		switch(select_menu(21)) {
		case 0:	return val;
		case 1: val ^= DLOG_ATTENT;	break;
		case 2: val ^= DLOG_NORMAL;	break;
		case 3: val ^= DLOG_VERBOSE;	break;
		case 4: val ^= DLOG_TCP;	break;
		case 5: val ^= DLOG_BBS;	break;
		case 6: val ^= DLOG_CHAT;	break;
		case 7: val ^= DLOG_DEVIO;	break;
		case 8: val ^= DLOG_EXEC;	break;
		case 9: val ^= DLOG_FILEFWD;	break;
		case 10:val ^= DLOG_HYDRA;	break;
		case 11:val ^= DLOG_IEMSI;	break;
		case 12:val ^= DLOG_LOCK;	break;
		case 13:val ^= DLOG_MAIL;	break;
		case 14:val ^= DLOG_NEWS;	break;
		case 15:val ^= DLOG_OUTSCAN;	break;
		case 16:val ^= DLOG_PACK;	break;
		case 17:val ^= DLOG_ROUTE;	break;
		case 18:val ^= DLOG_SESSION;	break;
		case 19:val ^= DLOG_TTY;	break;
		case 20:val ^= DLOG_XMODEM;	break;
		case 21:val ^= DLOG_ZMODEM;	break;
		}
	}

	return val;
}



void show_int(int y, int x, int val)
{
	mvprintw(y, x, (char *)"       ");
	mvprintw(y, x, (char *)"%d", val);
}



int edit_int(int y, int x, int val, char *help)
{
	static char s[6];
	static char line[6];

	showhelp(help);
	memset((char *)s, 0, 6);
	sprintf(line, "%d", val);
	strcpy(s, edit_field(y, x, 7, '9', line));
	set_color(WHITE, BLACK);
	show_int(y, x, atoi(s));
	fflush(stdout);
	return atoi(s);
}



void show_ushort(int y, int x, unsigned short val)
{
	mvprintw(y, x, (char *)"%d", val);
}



unsigned short edit_ushort(int y, int x, unsigned short val, char *help)
{
	unsigned short r;
	static char s[6];
	static char line[6];

	showhelp(help);
	memset((char *)s, 0, 6);
	do {
		sprintf(line, "%d", val);
		strcpy(s, edit_field(y, x, 5, '9', line));
		r = atoi(s);
		if (r >= 65535L) {
			working(2, y, x);
			working(0, y, x);
		}
	} while (r >= 65535L);
	set_color(WHITE, BLACK);
	show_int(y, x, val);
	fflush(stdout);
	return r;
}



void show_sbit(int y, int x, unsigned short val, unsigned short mask)
{
	show_bool(y, x, (val & mask) != 0);
}



unsigned short toggle_sbit(int y, int x, unsigned short val, unsigned short mask, char *help)
{
	int ch;

	showhelp(help);
	do {
		set_color(YELLOW, BLUE);
		show_sbit(y, x, val, mask);
		ch = readkey(y, x, YELLOW, BLUE);
		if (ch == ' ') 
			val ^= mask;
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_sbit(y, x, val, mask);
	fflush(stdout);
	return val;
}



void show_lbit(int y, int x, long val, long mask)
{
	show_bool(y, x, (val & mask) != 0);
}



long toggle_lbit(int y, int x, long val, long mask, char *help)
{
	int ch;

	showhelp(help);
	do {
		set_color(YELLOW, BLUE);
		show_lbit(y, x, val, mask);
		ch = readkey(y, x, YELLOW, BLUE);
		if (ch == ' ') 
			val ^= mask;
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_lbit(y, x, val, mask);
	fflush(stdout);
	return val;
}



char *getflag(unsigned long flag, unsigned long not)
{
	static	char temp[33];
	int	i;

	memset(temp, 0, 33);
	memset(temp, '-', 32);

	for (i = 0; i < 32; i++) {
		if ((flag >> i) & 1)
			temp[i] = 'X';
		if ((not >> i) & 1)
			temp[i] = 'O';
		/*
		 * The next one may never show up!
		 */
		if (((flag >> i) & 1) && ((not >> i) & 1))
			temp[i] = '!';
	}

	return temp;
}



void show_sec(int y, int x, securityrec sec)
{
	show_int(y, x, sec.level);
	mvprintw(y, x + 6, getflag(sec.flags, sec.notflags));
}



securityrec edit_sec(int y, int x, securityrec sec, char *shdr)
{
	int	c, i, xx, yy, s;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw(4,3,shdr);
	set_color(CYAN, BLACK);
	xx = 3;
	yy = 6;

	for (i = 0; i < 32; i++) {
		if (i == 11) {
			xx = 28;
			yy = 6;
		}
		if (i == 22) {
			xx = 53;
			yy = 6;
		}
		set_color(CYAN,BLACK);
		mvprintw(yy, xx, (char *)"%2d. %-16s", i+1, CFG.fname[i]);
		yy++;
	}
	mvprintw(16,53, "33. Security level");

	for (;;) {
		set_color(WHITE, BLACK);
		xx = 24;
		yy = 6;

		for (i = 0; i < 32; i++) {
			if (i == 11) {
				xx = 49;
				yy = 6;
			}
			if (i == 22) {
				xx = 74;
				yy = 6;
			}
			c = '-';
			if ((sec.flags >> i) & 1)
				c = 'X';
			if ((sec.notflags >> i) & 1)
				c = 'O';
			/*
			 * The next one may never show up
			 */
			if (((sec.flags >> i) & 1) && ((sec.notflags >> i) & 1))
				c = '!';
			mvprintw(yy,xx,(char *)"%c", c);
			yy++;
		}
		show_int(16,74, sec.level);
		s = select_menu(33);

		switch(s) {
		case 0: 
			return sec;
		case 33:
			sec.level = edit_int(16,74, sec.level, (char *)"^Security level^ 0..65535"); 
			break;
		default:
			if ((sec.notflags >> (s - 1)) & 1) {
				sec.notflags = (sec.notflags ^ (1 << (s - 1)));
				break;
			}
			if ((sec.flags >> (s - 1)) & 1) {
				sec.flags = (sec.flags ^ (1 << (s - 1)));
				sec.notflags = (sec.notflags | (1 << (s - 1)));
				break;
			}
			sec.flags = (sec.flags | (1 << (s - 1)));
			break;
		}
	}
}



securityrec edit_usec(int y, int x, securityrec sec, char *shdr)
{
	int	c, i, xx, yy, s;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw(4,3,shdr);
	set_color(CYAN, BLACK);
	xx = 3;
	yy = 6;

	for (i = 0; i < 32; i++) {
		if (i == 11) {
			xx = 28;
			yy = 6;
		}
		if (i == 22) {
			xx = 53;
			yy = 6;
		}
		set_color(CYAN,BLACK);
		mvprintw(yy, xx, (char *)"%2d. %-16s", i+1, CFG.fname[i]);
		yy++;
	}
	mvprintw(16,53, "33. Security level");

	for (;;) {
		set_color(WHITE, BLACK);
		xx = 24;
		yy = 6;

		for (i = 0; i < 32; i++) {
			if (i == 11) {
				xx = 49;
				yy = 6;
			}
			if (i == 22) {
				xx = 74;
				yy = 6;
			}
			c = '-';
			if ((sec.flags >> i) & 1)
				c = 'X';
			if ((sec.notflags >> i) & 1)
				c = 'O';
			/*
			 * The next one may never show up
			 */
			if (((sec.flags >> i) & 1) && ((sec.notflags >> i) & 1))
				c = '!';
			mvprintw(yy,xx,(char *)"%c", c);
			yy++;
		}
		show_int(16,74, sec.level);
		s = select_menu(33);

		switch(s) {
		case 0: 
			return sec;
		case 33:
			sec.level = edit_int(16,74, sec.level, (char *)"^Security level^ 0..65535"); 
			break;
		default:
			if ((sec.flags >> (s - 1)) & 1) {
				sec.flags = (sec.flags ^ (1 << (s - 1)));
				break;
			}
			sec.flags = (sec.flags | (1 << (s - 1)));
			break;
		}
	}
}



char *get_secstr(securityrec S)
{
	static char temp[45];

	memset(&temp, 0, sizeof(temp));
	sprintf(temp, "%-5d %s", S.level, getflag(S.flags, S.notflags));
	return temp;
}



char *getmsgtype(int val)
{
	switch (val) {
		case LOCALMAIL: return (char *)"Local   ";
		case NETMAIL:   return (char *)"Netmail ";
		case ECHOMAIL:  return (char *)"Echomail";
		case NEWS:	return (char *)"News    ";
		case LIST:	return (char *)"Listserv";
		default:        return NULL;
	}
}



void show_msgtype(int y, int x, int val)
{
	mvprintw(y, x, getmsgtype(val));
}



int edit_msgtype(int y, int x, int val)
{
	int ch;

	showhelp((char *)"Toggle ^Message Type^ with spacebar, press <Enter> whene done.");
	do {
		set_color(YELLOW, BLUE);
		show_msgtype(y, x, val);

		ch = readkey(y, x, YELLOW, BLUE);

		if (ch == ' ') {
			if (val < LIST)
				val++;
			else
				val = LOCALMAIL;
		} 
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_msgtype(y, x, val);
	fflush(stdout);
	return val;
}



char *getemailmode(int val)
{
	switch (val) {
		case E_NOISP:	return (char *)"No internet   ";
		case E_TMPISP:	return (char *)"No maildomain ";
		case E_PRMISP:	return (char *)"Own maildomain";
		default:	return NULL;
	}
}



void show_emailmode(int y, int x, int val)
{
        mvprintw(y, x, getemailmode(val));
}



int edit_emailmode(int y, int x, int val)
{
	int ch;

	showhelp((char *)"Toggle ^Internet Email Mode^ with spacebar, press <Enter> whene done.");
	do {
		set_color(YELLOW, BLUE);
		show_emailmode(y, x, val);

		ch = readkey(y, x, YELLOW, BLUE);

		if (ch == ' ') {
			if (val < E_PRMISP)
				val++;
			else
				val = E_NOISP;
		}
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_emailmode(y, x, val);
	return val;
}



char *getmsgkinds(int val)
{
	switch (val) {
		case BOTH:    return (char *)"Private/Public  ";
		case PRIVATE: return (char *)"Private only    ";
		case PUBLIC:  return (char *)"Public only     ";
		case RONLY:   return (char *)"Read Only       ";
		case FTNMOD:  return (char *)"FTN moderated   ";
		case USEMOD:  return (char *)"Usenet moderated";
		default:      return NULL;
	}
}



void show_msgkinds(int y, int x, int val)
{
	mvprintw(y, x, getmsgkinds(val));
}



int edit_msgkinds(int y, int x, int val)
{
	int ch;

	showhelp((char *)"Toggle ^Message Kinds^ with spacebar, press <Enter> whene done.");
	do {
		set_color(YELLOW, BLUE);
		show_msgkinds(y, x, val);

		ch = readkey(y, x, YELLOW, BLUE);

		if (ch == ' ') {
			if (val < USEMOD)
				val++;
			else
				val = BOTH;
		} 
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_msgkinds(y, x, val);
	return val;
}



char *getlinetype(int val)
{
	switch (val) {
		case POTS:    return (char *)"POTS   ";
		case ISDN:    return (char *)"ISDN   ";
		case NETWORK: return (char *)"Network";
		case LOCAL:   return (char *)"Local  ";
		default:      return NULL;
	}
}



void show_linetype(int y, int x, int val)
{
	mvprintw(y, x, getlinetype(val));
}



int edit_linetype(int y, int x, int val)
{
	int ch;

	showhelp((char *)"Toggle ^Line Type^ with spacebar, press <Enter> whene done.");
	do {
		set_color(YELLOW, BLUE);
		show_linetype(y, x, val);

		ch = readkey(y, x, YELLOW, BLUE);

		if (ch == ' ') {
			if (val < LOCAL)
				val++;
			else
				val = POTS;
		} 
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_linetype(y, x, val);
	return val;
}



char *getservice(int val)
{
	switch (val) {
		case AREAMGR:	return (char *)"AreaMgr";
		case FILEMGR:	return (char *)"FileMgr";
		case EMAIL:	return (char *)"Email  ";
		default:	return NULL;
	}
}



void show_service(int y, int x, int val)
{
	mvprintw(y, x, getservice(val));
}



int edit_service(int y, int x, int val)
{
	int ch;

	showhelp((char *)"Toggle ^Service Type^ with spacebar, press <Enter> whene done.");
	do {
		set_color(YELLOW, BLUE);
		show_service(y, x, val);

		ch = readkey(y, x, YELLOW, BLUE);

		if (ch == ' ') {
			if (val < EMAIL)
				val++;
			else
				val = AREAMGR;
		}
	} while (ch != KEY_ENTER && ch != '\012');
	set_color(WHITE, BLACK);
	show_service(y, x, val);
	return val;
}



char *getnewsmode(int val)
{
        switch (val) {
                case FEEDINN:   return (char *)"NNTP feed ";
                case FEEDRNEWS: return (char *)"rnews feed";
                case FEEDUUCP:  return (char *)"UUCP feed ";
                default:        return NULL;
        }
}



void show_newsmode(int y, int x, int val)
{
        mvprintw(y, x, getnewsmode(val));
}



int edit_newsmode(int y, int x, int val)
{
        int ch;

        showhelp((char *)"Toggle ^Newsfeed mode^ with spacebar, press <Enter> whene done.");
        do {
                set_color(YELLOW, BLUE);
                show_newsmode(y, x, val);

                ch = readkey(y, x, YELLOW, BLUE);

                if (ch == ' ') {
                        if (val < FEEDUUCP)
                                val++;
                        else
                                val = FEEDINN;
                }
        } while (ch != KEY_ENTER && ch != '\012');
        set_color(WHITE, BLACK);
        show_newsmode(y, x, val);
        return val;
}



void show_magictype(int y, int x, int val)
{
	mvprintw(y, x, getmagictype(val));
}



int edit_magictype(int y, int x, int val)
{
	int	ch;

	showhelp((char *)"Toggle ^Magic type^ with spacebar, press <Enter> whene done");

	do {
		set_color(YELLOW, BLUE);
		show_magictype(y, x, val);

		ch = readkey(y, x, YELLOW, BLUE);

		if (ch == ' ') {
			if (val < MG_DELETE)
				val++;
			else
				val = 0;
		}

	} while ((ch != KEY_ENTER) && (ch != '\012'));

	set_color(WHITE, BLACK);
	show_magictype(y, x, val);
	return val;
}



char *getmagictype(int val)
{
	switch(val) {
		case MG_EXEC:		return (char *)"Execute     ";
		case MG_COPY:		return (char *)"Copy file   ";
		case MG_UNPACK:		return (char *)"Unpack file ";
		case MG_KEEPNUM:	return (char *)"Keep number ";
		case MG_MOVE:		return (char *)"Move file   "; 
		case MG_UPDALIAS:	return (char *)"Update alias";
		case MG_ADOPT:		return (char *)"Adopt file  ";
		case MG_DELETE:		return (char *)"Delete file ";
		default:		return NULL;
	}
}



void show_aka(int y, int x, fidoaddr aka)
{
	char	temp[24];

	if (aka.point == 0)
		sprintf(temp, "%d:%d/%d@%s", aka.zone, aka.net, aka.node, aka.domain);
	else
		sprintf(temp, "%d:%d/%d.%d@%s", aka.zone, aka.net, aka.node, aka.point, aka.domain);
	mvprintw(y, x, temp);
}



void edit_color(int *fg, int *bg, char *title, char *help) 
{
	int	ch, f, b;
	char	temp[81];

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw(5, 6, title);
	sprintf(temp, "Change the ^%s^ color with arrow keys, press <Enter> whene done", help);
	showhelp(temp);

	for (f = 0; f < 16; f++)
		for (b = 0; b < 8; b++) {
			set_color(f, b);
			mvprintw(b + 9, f + 33, ".");
		}

	f = (*fg) & 15;
	b = (*bg) & 7;

	for (;;) {
		set_color(f, b);
		mvprintw(7, 6, "This is an example...");
		fflush(stdout);
		mvprintw(b + 9, f + 33, "*");
		ch = readkey(7,28, f, b);
		mvprintw(b + 9, f + 33, ".");
		switch(ch) {
			case KEY_LINEFEED: 
			case KEY_ENTER: (* fg) = f;
					(* bg) = b;
					return;
			case KEY_LEFT:	if (f > 0)
						f--;
					break;
			case KEY_RIGHT:	if (f < 15)
						f++;
					break;
			case KEY_UP:	if (b > 0)
						b--;
					break;
			case KEY_DOWN:	if (b < 7)
						b++;
					break;
		}
	}
}



char *get_color(int c)
{
	switch (c) {
		case BLACK:		return (char *)"black";
		case BLUE:		return (char *)"blue";
		case GREEN:		return (char *)"green";
		case CYAN:		return (char *)"cyan";
		case RED:		return (char *)"red";
		case MAGENTA:		return (char *)"magenta";
		case BROWN:		return (char *)"brown";
		case LIGHTGRAY:		return (char *)"lightgray";
		case DARKGRAY:		return (char *)"darkgray";
		case LIGHTBLUE:		return (char *)"lightblue";
		case LIGHTGREEN:	return (char *)"lightgreen";
		case LIGHTCYAN:		return (char *)"lightcyan";
		case LIGHTRED:		return (char *)"lightred";
		case LIGHTMAGENTA:	return (char *)"lightmagenta";
		case YELLOW:		return (char *)"yellow";
		case WHITE:		return (char *)"white";
		default:		return NULL;
	}
}



char *getmenutype(int val)
{
	switch (val) {
		case 1:		return (char *)"Goto another menu";
		case 2:		return (char *)"Gosub another menu";
		case 3:		return (char *)"Return from gosub";
		case 4:		return (char *)"Return to top menu";
		case 5:		return (char *)"Display A?? file w/Ctrl codes";
		case 6:		return (char *)"Display menu prompt";
		case 7:		return (char *)"Run external program in shell";
		case 8:		return (char *)"Show product information";
		case 9:		return (char *)"Display todays callers list";
		case 10:	return (char *)"Display user list";
		case 11:	return (char *)"Display time statistics";
		case 12:	return (char *)"Page sysop for a chat";
		case 13:	return (char *)"Terminate call";
		case 14:	return (char *)"Make a log entry";
		case 15:	return (char *)"Print text to screen";
		case 16:	return (char *)"Who's currently on-line";
		case 17:	return (char *)"Comment to sysop";
		case 18:	return (char *)"Send an on-line message";
		case 19:	return (char *)"Display textfile with more";
		case 20:	return (char *)"Display .A?? file with Enter";
		case 21:	return (char *)"Display Text Only";
		case 22:	return (char *)"Message to nextuser door";
		case 23:	return (char *)"Time banking system";

		case 25:	return (char *)"Safe cracker door";

		case 101:	return (char *)"Select new file area";
		case 102:	return (char *)"List files in current area";
		case 103:	return (char *)"View a textfile";
		case 104:	return (char *)"Download tagged files";
		case 105:	return (char *)"Raw directory listing";
		case 106:	return (char *)"Search file on keyword";
		case 107:	return (char *)"Search file on filename";
		case 108:	return (char *)"Scan for new files";
		case 109:	return (char *)"Upload a file";
		case 110:	return (char *)"Edit the taglist";
		case 111:	return (char *)"View file in home directory";
		case 112:	return (char *)"Download a specific file";
		case 113:	return (char *)"Copy file to home directory";
		case 114:	return (char *)"List files in home directory";
		case 115:	return (char *)"Delete file in home directory";
		case 116:	return (char *)"Unpack file in home directory";
		case 117:	return (char *)"Pack files in home directory";
		case 118:	return (char *)"Download from home directory";
		case 119:	return (char *)"Upload to home directory";

		case 201:	return (char *)"Select new message area";
		case 202:	return (char *)"Post a new message";
		case 203:	return (char *)"Read messages";
		case 204:	return (char *)"Check for new mail";
		case 205:	return (char *)"Quick-scan messages";
		case 206:	return (char *)"Delete a specific message";
		case 207:	return (char *)"Show mail status";
		case 208:	return (char *)"OLR Tag Area";
		case 209:	return (char *)"OLR Remove Area";
		case 210:	return (char *)"OLR View Areas";
		case 211:	return (char *)"OLR Restrict Date";
		case 212:	return (char *)"OLR Upload";
		case 213:	return (char *)"OLR Download BlueWave";
		case 214:	return (char *)"OLR Download QWK";
		case 215:	return (char *)"OLR Download ASCII";
		case 216:	return (char *)"Read email";
		case 217:	return (char *)"Post email";
		case 218:	return (char *)"Trash email";
		case 219:	return (char *)"Choose mailbox";
		case 220:	return (char *)"Quick-scan email's";

		case 301:	return (char *)"Change transfer protocol";
		case 302:	return (char *)"Change password";
		case 303:	return (char *)"Change location";
		case 304:	return (char *)"Change graphics mode";
		case 305:	return (char *)"Change voice phone";
		case 306:	return (char *)"Change data phone";
		case 307:	return (char *)"Change show news bulletins";
		case 308:	return (char *)"Change screen length";
		case 309:	return (char *)"Change date of birth";
		case 310:	return (char *)"Change language";
		case 311:	return (char *)"Change hot-keys";
		case 312:	return (char *)"Change handle (alias)";
		case 313:	return (char *)"Change check for new mail";
		case 314:	return (char *)"Change do-not-disturb";
		case 315:	return (char *)"Change check for new files";
		case 316:	return (char *)"Change fullscreen editor";
		case 317:	return (char *)"Change FS edit shortcut keys";
		case 318:	return (char *)"Change address";

		case 401:	return (char *)"Add oneliner";
		case 402:	return (char *)"List oneliners";
		case 403:	return (char *)"Show a oneliner";
		case 404:	return (char *)"Mark oneliner for deletion";
		case 405:	return (char *)"Print a random oneliner";

		case 501:	return (char *)"Add a BBS";
		case 502:	return (char *)"List BBS's";
		case 503:	return (char *)"Show a BBS";
		case 504:	return (char *)"Mark a BBS for deletion";
		case 505:	return (char *)"Print a BBS";
		case 506:	return (char *)"Search for a BBS";

		default:	return (char *)"Unknown menu";
	}
}



