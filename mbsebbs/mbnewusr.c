/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: New user registration
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "mbnewusr.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "newuser.h"
#include "term.h"
#include "ttyio.h"
#include "openport.h"


extern	int	do_quiet;	/* Logging quiet flag */
time_t		t_start;
char            *StartTime;


int main(int argc, char **argv)
{
    char	    *p, *tty, temp[PATH_MAX];
    FILE            *pTty;
    int             i, rc = 0;
    struct passwd   *pw;

    pTTY = calloc(15, sizeof(char));
    tty = ttyname(1);

    /*
     * Get MBSE_ROOT Path and load Config into Memory
     */
    FindMBSE();
    if (!strlen(CFG.startname)) {
	printf("FATAL: No bbs startname, edit mbsetup 1.2.10\n");
	exit(MBERR_CONFIG_ERROR);
    }

    /*
     * Set uid and gid to the "mbse" user.
     */
    if ((pw = getpwnam((char *)"mbse")) == NULL) {
	perror("Can't find user \"mbse\" in /etc/passwd");
	exit(MBERR_INIT_ERROR);
    }

    /*
     * Set effective user to mbse.bbs
     */
    if ((seteuid(pw->pw_uid) == -1) || (setegid(pw->pw_gid) == -1)) {
	perror("Can't seteuid() or setegid() to \"mbse\" user");
	exit(MBERR_INIT_ERROR);
    }

    /* 
     * Set local time and statistic indexes.
     */
    Time_Now = t_start = time(NULL); 
    l_date = localtime(&Time_Now); 
    Diw = l_date->tm_wday;
    Miy = l_date->tm_mon;
    ltime = time(NULL);  

    /*
     * Initialize this client with the server. We don't know
     * who is at the other end of the line, so that's what we tell.
     */
    do_quiet = TRUE;
    InitClient((char *)"Unknown", (char *)"mbnewusr", (char *)"Unknown", 
		    CFG.logfile, CFG.bbs_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    IsDoing("Loging in");

    Syslog(' ', " ");
    Syslog(' ', "MBNEWUSR v%s", VERSION);

    if ((rc = rawport()) != 0) {
	WriteError("Unable to set raw mode");
	Fast_Bye(MBERR_OK);;
    }

    Enter(2);
    PUTSTR((char *)"Loading MBSE BBS New User Registration ...");
    Enter(2);
    
    if ((p = getenv("CONNECT")) != NULL)
	Syslog('+', "CONNECT %s", p);
    if ((p = getenv("CALLER_ID")) != NULL)
	if (strncmp(p, "none", 4))
	    Syslog('+', "CALLER  %s", p);

    sUnixName[0] = '\0';

    /*
     * Initialize 
     */
    InitLanguage();

    if ((tty = ttyname(0)) == NULL) {
	WriteError("Not at a tty");
	Fast_Bye(MBERR_OK);
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
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGPIPE) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
	else
	    signal(i, SIG_IGN);
    }

    /*
     * Default set the terminal to ANSI mode. If your logo
     * is in color, the user will see color no mather what.
     */
    TermInit(1, 80, 24);
		
    /*
     * Now it's time to check if the bbs is open. If not, we 
     * log the user off.
     */
    if (CheckStatus() == FALSE) {
	Syslog('+', "Kicking user out, the BBS is closed");
	Fast_Bye(MBERR_OK);
    }

    sprintf(temp, "MBSE BBS v%s (Release: %s) on %s/%s", VERSION, ReleaseDate, OsName(), OsCPU());
    poutCR(YELLOW, BLACK, temp);
    pout(WHITE, BLACK, (char *)COPYRIGHT);
    Enter(2);
 
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
	    PUTSTR((char *)"No BBS on this port allowed!");
	    Enter(2);
	    Fast_Bye(MBERR_OK);
	}

	/* 
	 * Ask whether to display Connect String 
	 */
	if (CFG.iConnectString) {
	    /* Connected on port */
	    sprintf(temp, "%s\"%s\" ", (char *) Language(348), ttyinfo.comment);
	    pout(CYAN, BLACK, temp);
	    /* on */
	    sprintf(temp, "%s %s", (char *) Language(135), ctime(&ltime));
	    PUTSTR(temp);
	    Enter(1);
	}
    }

    alarm_on();
    Pause();

    newuser();
    Fast_Bye(MBERR_OK);
    return 0;
}


