/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: New user registration
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "mbnewusr.h"
#include "funcs.h"
#include "funcs4.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "newuser.h"



extern	int	do_quiet;	/* Logging quiet flag */
time_t		t_start;
char            *StartTime;


int main(int argc, char **argv)
{
	char		*p, *tty;
        FILE            *pTty;
        int             i;
        char            temp[PATH_MAX];
        struct passwd   *pw;

#ifdef MEMWATCH
	mwInit();
#endif
	printf("Loading MBSE BBS New User Registration ...\n");
 	pTTY = calloc(15, sizeof(char));
	tty = ttyname(1);

	/*
	 * Get MBSE_ROOT Path and load Config into Memory
	 */
	FindMBSE();
	if (!strlen(CFG.startname)) {
		printf("FATAL: No bbs startname, edit mbsetup 1.2.10\n");
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(1);
	}

	/*
	 * Set uid and gid to the "mbse" user.
	 */
	if ((pw = getpwnam((char *)"mbse")) == NULL) {
		perror("Can't find user \"mbse\" in /etc/passwd");
#ifdef MEMWATCH
                mwTerm();
#endif
		exit(1);
	}
	if ((setuid(pw->pw_uid) == -1) || (setgid(pw->pw_gid) == -1)) {
		perror("Can't setuid() or setgid() to \"mbse\" user");
#ifdef MEMWATCH
                mwTerm();
#endif
		exit(1);
	}

	/* 
	 * Set local time and statistic indexes.
	 */
	time(&Time_Now); 
	time(&t_start);
	l_date = localtime(&Time_Now); 
	Diw = l_date->tm_wday;
	Miy = l_date->tm_mon;
	time(&ltime);  

	/*
	 * Initialize this client with the server. We don't know
	 * who is at the other end of the line, so that's what we tell.
	 */
	do_quiet = TRUE;
	InitClient((char *)"Unknown", (char *)"mbnewusr", (char *)"Unknown", CFG.logfile, CFG.bbs_loglevel, CFG.error_log);
	IsDoing("Loging in");

	Syslog(' ', " ");
	Syslog(' ', "MBNEWUSR v%s", VERSION);

	if ((p = getenv("CONNECT")) != NULL)
		Syslog('+', "CONNECT %s", p);
	if ((p = getenv("CALLER_ID")) != NULL)
		if (!strncmp(p, "none", 4))
			Syslog('+', "CALLER  %s", p);

	sUnixName[0] = '\0';

	/*
	 * Initialize 
	 */
	InitLanguage();

	if ((tty = ttyname(0)) == NULL) {
		WriteError("Not at a tty");
		Fast_Bye(0);
	}

	if (strncmp("/dev/", tty, 5) == 0)
		sprintf(pTTY, "%s", tty+5);
	else if (*tty == '/') {
		tty = strrchr(ttyname(0), '/');
		++tty;
		sprintf(pTTY, "%s", tty);
	}

	umask(007);

	/* 
	 * Trap signals
	 */
	for(i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGILL) ||
		    (i == SIGSEGV) || (i == SIGTERM) || (i == SIGKILL))
		signal(i, (void (*))die);
	else
		signal(i, SIG_IGN);
	}

	/*
	 * Default set the terminal to ANSI mode. If your logo
	 * is in color, the user will see color no mather what.
	 */
	TermInit(1);
		
	/*
	 * Now it's time to check if the bbs is open. If not, we 
	 * log the user off.
	 */
	if (CheckStatus() == FALSE) {
		Syslog('+', "Kicking user out, the BBS is closed");
		Fast_Bye(0);
	}

	clear();
	DisplayLogo();

	colour(YELLOW, BLACK);
	printf("MBSE BBS v%s (Release: %s)\n", VERSION, ReleaseDate);
	colour(WHITE, BLACK);
	printf("%s\n\n", COPYRIGHT);
 
	/*
	 * Check if this port is available.
	 */
	sprintf(temp, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));

	if ((pTty = fopen(temp, "r")) == NULL) {
		WriteError("Can't read %s", temp);	
	} else {
		fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, pTty);

		while (fread(&ttyinfo, ttyinfohdr.recsize, 1, pTty) == 1) {
			if (strcmp(ttyinfo.tty, pTTY) == 0) 
				break;
		}
		fclose(pTty);

		if ((strcmp(ttyinfo.tty, pTTY) != 0) || (!ttyinfo.available)) {
			Syslog('+', "No BBS allowed on port \"%s\"", pTTY);
			printf("No BBS on this port allowed!\n\n");
			Fast_Bye(0);
		}

		/* 
		 * Ask whether to display Connect String 
		 */
		if (CFG.iConnectString) {
			/* Connected on port */
			colour(CYAN, BLACK);
			printf("%s\"%s\" ", (char *) Language(348), ttyinfo.comment);
			/* on */
			printf("%s %s\n", (char *) Language(135), ctime(&ltime));
		}
	}

	alarm_on();
	Pause();

	newuser();
	Fast_Bye(0);
	return 0;
}


