/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Task Manager
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "../lib/structs.h"
#include "signame.h"
#include "taskstat.h"
#include "taskutil.h"
#include "taskregs.h"
#include "taskcomm.h"
#include "callstat.h"
#include "outstat.h"
#include "nodelist.h"
#include "ports.h"
#include "calllist.h"
#include "ping.h"
#include "mbtask.h"



/*
 *  Global variables
 */
static onetask		task[MAXTASKS];		/* Array with tasks	*/
extern tocall		calllist[MAXTASKS];	/* Array with calllist	*/
reg_info		reginfo[MAXCLIENT];	/* Array with clients	*/
static pid_t		pgrp;			/* Pids group		*/
static char		lockfile[PATH_MAX];	/* Lockfile 		*/
int			sock = -1;		/* Datagram socket	*/
struct sockaddr_un	servaddr;		/* Server address	*/
struct sockaddr_un	from;			/* From address		*/
int			fromlen;
static char		spath[PATH_MAX];	/* Socket path		*/
int			logtrans = 0;		/* Log transactions	*/
struct taskrec		TCFG;			/* Task config record	*/
struct sysconfig	CFG;			/* System config	*/
struct _nodeshdr	nodeshdr;		/* Nodes header record	*/
struct _nodes		nodes;			/* Nodes data record	*/
struct _fidonethdr	fidonethdr;		/* Fidonet header rec.	*/
struct _fidonet		fidonet;		/* Fidonet data record	*/
time_t			tcfg_time;		/* Config record time	*/
time_t			cfg_time;		/* Config record time	*/
time_t			tty_time;		/* TTY config time	*/
char			tcfgfn[PATH_MAX];	/* Config file name	*/
char			cfgfn[PATH_MAX];	/* Config file name	*/
char			ttyfn[PATH_MAX];	/* TTY file name	*/
extern int     		ping_isocket;		/* Ping socket		*/
int			internet = FALSE;	/* Internet is down	*/
double			Load;			/* System Load		*/
int			Processing;		/* Is system running	*/
int			ZMH = FALSE;		/* Zone Mail Hour	*/
int			UPSalarm = FALSE;	/* UPS alarm status	*/
extern int		s_bbsopen;		/* BBS open semafore	*/
extern int		s_scanout;		/* Scan outbound sema	*/
extern int		s_mailout;		/* Mail out semafore	*/
extern int		s_mailin;		/* Mail in semafore	*/
extern int		s_index;		/* Compile nl semafore	*/
extern int		s_newnews;		/* New news semafore	*/
extern int		s_reqindex;		/* Create req index sem */
extern int		s_msglink;		/* Messages link sem	*/
extern int		pingresult[2];		/* Ping results		*/
int			masterinit = FALSE;	/* Master init needed	*/
int			ptimer = PAUSETIME;	/* Pause timer		*/
int			tflags = FALSE;		/* if nodes with Txx	*/
extern int		nxt_hour;		/* Next event hour	*/
extern int		nxt_min;		/* Next event minute	*/
extern _alist_l		*alist;			/* Nodes to call list	*/
int			rescan = FALSE;		/* Master rescan flag	*/
extern int		pots_calls;
extern int		isdn_calls;
extern int		inet_calls;
extern int		pots_lines;		/* POTS lines available	*/
extern int		isdn_lines;		/* ISDN lines available */
extern int		pots_free;		/* POTS lines free	*/
extern int		isdn_free;		/* ISDN lines free	*/
extern pp_list		*pl;			/* List of tty ports	*/
extern int		ipmailers;		/* TCP/IP mail sessions	*/



/*
 *  Load main configuration, if it doesn't exist, create it.
 *  This is the case the very first time when you start MBSE BBS.
 */
void load_maincfg(void)
{
	FILE		*fp;
	struct utsname	un;
	int		i;

        if ((fp = fopen(cfgfn, "r")) == NULL) {
		masterinit = TRUE;
                memset(&CFG, 0, sizeof(CFG));

                /*
                 * Fill Registration defaults
                 */
                sprintf(CFG.bbs_name, "MBSE BBS");
                uname((struct utsname *)&un); 
#ifdef __USE_GNU
                sprintf(CFG.sysdomain, "%s.%s", un.nodename, un.domainname); 
#else
#ifdef __linux__
                sprintf(CFG.sysdomain, "%s.%s", un.nodename, un.__domainname);
#endif
#endif
                sprintf(CFG.comment, "MBSE BBS development");
                sprintf(CFG.origin, "MBSE BBS. Made in the Netherlands");
                sprintf(CFG.location, "Earth");

                /*
                 * Fill Filenames defaults
                 */
                sprintf(CFG.logfile, "system.log");
                sprintf(CFG.error_log, "error.log");
                sprintf(CFG.default_menu, "main.mnu");
                sprintf(CFG.current_language, "english.lang");
                sprintf(CFG.chat_log, "chat.log");
                sprintf(CFG.welcome_logo, "logo.asc");

                /*
                 * Fill Global defaults
                 */
                sprintf(CFG.bbs_menus, "%s/english/menus", getenv("MBSE_ROOT"));
                sprintf(CFG.bbs_txtfiles, "%s/english/txtfiles", getenv("MBSE_ROOT"));
                sprintf(CFG.bbs_usersdir, "%s/home", getenv("MBSE_ROOT"));
                sprintf(CFG.nodelists, "%s/var/nodelist", getenv("MBSE_ROOT"));
                sprintf(CFG.inbound, "%s/var/unknown", getenv("MBSE_ROOT"));
                sprintf(CFG.pinbound, "%s/var/inbound", getenv("MBSE_ROOT"));
                sprintf(CFG.outbound, "%s/var/bso/outbound", getenv("MBSE_ROOT"));
		sprintf(CFG.msgs_path, "%s/var/msgs", getenv("MBSE_ROOT"));
                sprintf(CFG.uxpath, "%s", getenv("MBSE_ROOT"));
                sprintf(CFG.badtic, "%s/var/badtic", getenv("MBSE_ROOT"));
                sprintf(CFG.ticout, "%s/var/ticqueue", getenv("MBSE_ROOT"));
                sprintf(CFG.req_magic, "%s/magic", getenv("MBSE_ROOT"));
		sprintf(CFG.alists_path, "%s/var/arealists", getenv("MBSE_ROOT"));
		CFG.leavecase = TRUE;

                /*
                 * Newfiles reports
                 */
                sprintf(CFG.ftp_base, "%s/ftp/pub", getenv("MBSE_ROOT"));
                CFG.newdays = 30;
                CFG.security.level = 20;
                CFG.new_split = 27;
                CFG.new_force = 30;

                /*
                 * BBS Globals
                 */
                CFG.CityLen = 6;
                CFG.exclude_sysop = TRUE;
                CFG.iConnectString = FALSE;
                CFG.iAskFileProtocols = FALSE;
                CFG.sysop_access = 32000;
                CFG.password_length = 4;
                CFG.iPasswd_Char = '.';
                CFG.idleout = 3;
                CFG.iQuota = 10;
                CFG.iCRLoginCount = 10;
                CFG.bbs_loglevel = DLOG_ALLWAYS | DLOG_ERROR | DLOG_ATTENT | DLOG_NORMAL | DLOG_VERBOSE;
                CFG.util_loglevel = DLOG_ALLWAYS | DLOG_ERROR | DLOG_ATTENT | DLOG_NORMAL | DLOG_VERBOSE;
                CFG.OLR_NewFileLimit = 30;
                CFG.OLR_MaxReq = 25;
                CFG.slow_util = TRUE;
                CFG.iCrashLevel = 100;
                CFG.iAttachLevel = 100;
                CFG.new_groups = 25;

                CFG.slow_util = TRUE;
                CFG.iCrashLevel = 100;
                CFG.iAttachLevel = 100;
                CFG.new_groups = 25;
                sprintf(CFG.startname, "bbs");
                CFG.freespace = 10;

                /*
                 * New Users
                 */
                CFG.newuser_access.level = 20;
                CFG.iCapUserName = TRUE;
                CFG.iAnsi = TRUE;
                CFG.iDataPhone = TRUE;
                CFG.iVoicePhone = TRUE;
                CFG.iDOB = TRUE;
                CFG.iTelephoneScan = TRUE;
                CFG.iLocation = TRUE;
                CFG.iHotkeys = TRUE;
                CFG.iCapLocation = FALSE;
                CFG.AskAddress = TRUE;
                CFG.GiveEmail = TRUE;

                /*
                 * Colors
                 */
                CFG.TextColourF         = 3;
                CFG.TextColourB         = 0;
                CFG.UnderlineColourF    = 14;
                CFG.UnderlineColourB    = 0;
                CFG.InputColourF        = 11;
                CFG.InputColourB        = 0;
                CFG.CRColourF           = 15;
                CFG.CRColourB           = 0;
                CFG.MoreF               = 13;
                CFG.MoreB               = 0;
                CFG.HiliteF             = 15;
                CFG.HiliteB             = 0;
                CFG.FilenameF           = 14;
                CFG.FilenameB           = 0;
                CFG.FilesizeF           = 13;
                CFG.FilesizeB           = 0;
                CFG.FiledateF           = 10;
                CFG.FiledateB           = 0;
                CFG.FiledescF           = 3;
                CFG.FiledescB           = 0;
                CFG.MsgInputColourF     = 3;
                CFG.MsgInputColourB     = 0;

                /*
                 * NextUser Door
                 */
                sprintf(CFG.sNuScreen, "welcome");
                sprintf(CFG.sNuQuote, "Please press [ENTER] to continue: ");

                /*
                 * Safe Door
                 */
                CFG.iSafeFirstDigit     = 1;
                CFG.iSafeSecondDigit    = 2;
                CFG.iSafeThirdDigit     = 3;
                CFG.iSafeMaxTrys        = 4;
                CFG.iSafeMaxNumber      = 20;
                CFG.iSafeNumGen         = FALSE;
                strcpy(CFG.sSafePrize, "Free access for a year!");
                sprintf(CFG.sSafeWelcome, "safewel");
                sprintf(CFG.sSafeOpened, "safeopen");

                /*
                 * Paging
                 */
                CFG.iPageLength         = 30;
                CFG.iMaxPageTimes       = 5;
                CFG.iAskReason          = TRUE;
                CFG.iSysopArea          = 1;
                CFG.iExternalChat       = FALSE;
                strcpy(CFG.sExternalChat, "/usr/local/bin/chat");
                CFG.iAutoLog            = TRUE;
                strcpy(CFG.sChatDevice, "/dev/tty01");
                CFG.iChatPromptChk      = TRUE;
                CFG.iStopChatTime       = TRUE;
                for (i = 0; i < 7; i++) {
                        sprintf(CFG.cStartTime[i], "18:00");
                        sprintf(CFG.cStopTime[i], "23:59");
                }

                /*
                 * Time Bank
                 */
                CFG.iMaxTimeBalance     = 200;
                CFG.iMaxTimeWithdraw    = 100;
                CFG.iMaxTimeDeposit     = 60;
                CFG.iMaxByteBalance     = 500;
                CFG.iMaxByteWithdraw    = 300;
                CFG.iMaxByteDeposit     = 150;
                strcpy(CFG.sTimeRatio, "3:1");
                strcpy(CFG.sByteRatio, "3:1");

                /*
                 * Fill ticconf defaults
                 */
                CFG.ct_ResFuture = TRUE;
                CFG.ct_ReplExt = TRUE;
                CFG.ct_PlusAll = TRUE;
                CFG.ct_Notify = TRUE;
                CFG.ct_Message = TRUE;
                CFG.ct_TIC = TRUE;
                CFG.tic_days = 30;
                sprintf(CFG.hatchpasswd, "DizIzMyBIGseeKret");
                CFG.drspace = 2048;
                CFG.tic_systems = 10;
                CFG.tic_groups  = 25;
                CFG.tic_dupes   = 16000;

                /*
                 * Fill Mail defaults
                 */
                CFG.maxpktsize = 150;
                CFG.maxarcsize = 300;
                sprintf(CFG.badboard, "%s/var/mail/badmail", getenv("MBSE_ROOT"));
                sprintf(CFG.dupboard, "%s/var/mail/dupemail", getenv("MBSE_ROOT"));
                sprintf(CFG.popnode, "localhost");
                sprintf(CFG.smtpnode, "localhost");
                sprintf(CFG.nntpnode, "localhost");
                CFG.toss_days = 30;
                CFG.toss_dupes = 16000;
                CFG.toss_old = 60;
                CFG.defmsgs = 500;
                CFG.defdays = 90;
                CFG.toss_systems = 10;
                CFG.toss_groups = 25;
                CFG.UUCPgate.zone = 2;
                CFG.UUCPgate.net  = 292;
                CFG.UUCPgate.node = 875;
                sprintf(CFG.UUCPgate.domain, "fidonet");
                CFG.nntpdupes = 16000;

                for (i = 0; i < 32; i++) 
                        sprintf(CFG.fname[i], "Flag %d", i+1);


                /*
                 * Fido mailer defaults
                 */
                CFG.timeoutreset = 3L;
                CFG.timeoutconnect = 60L;
                sprintf(CFG.phonetrans[0].match, "31-255");
                sprintf(CFG.phonetrans[1].match, "31-");
                sprintf(CFG.phonetrans[1].repl, "0");
                sprintf(CFG.phonetrans[2].repl, "00");
                CFG.Speed = 9600;
                CFG.dialdelay = 60;
                sprintf(CFG.Flags, "CM,XX,IBN,IFC,ITN");
                CFG.cico_loglevel = DLOG_ALLWAYS | DLOG_ERROR | DLOG_ATTENT | DLOG_NORMAL | DLOG_VERBOSE;

                /*
                 * FTP Server
                 */
                CFG.ftp_limit = 20;
                CFG.ftp_loginfails = 2;
                CFG.ftp_compress = TRUE;
                CFG.ftp_tar = TRUE;
                CFG.ftp_log_cmds = TRUE;
                CFG.ftp_anonymousok = TRUE;
                CFG.ftp_mbseok = FALSE;
                sprintf(CFG.ftp_readme_login, "README*");
                sprintf(CFG.ftp_readme_cwd, "README*");
                sprintf(CFG.ftp_msg_login, "/welcome.msg");
                sprintf(CFG.ftp_msg_cwd, ".message");
                sprintf(CFG.ftp_msg_shutmsg, "/etc/nologin");
                sprintf(CFG.ftp_upl_path, "%s/ftp/incoming", getenv("MBSE_ROOT"));
                sprintf(CFG.ftp_banner, "%s/etc/ftpbanner", getenv("MBSE_ROOT"));
                sprintf(CFG.ftp_email, "sysop@%s", CFG.sysdomain);
                sprintf(CFG.ftp_pth_filter, "^[-A-Za-z0-9_\\.]*$  ^\\.  ^-");
                sprintf(CFG.ftp_pth_message, "%s/etc/pathmsg", getenv("MBSE_ROOT"));

		/*
		 *  WWW defaults
		 */
        	sprintf(CFG.www_root, "/var/www/htdocs");
        	sprintf(CFG.www_link2ftp, "files");
        	sprintf(CFG.www_url, "http://%s", CFG.sysdomain);
        	sprintf(CFG.www_charset, "ISO 8859-1");
        	sprintf(CFG.www_tbgcolor, "Silver");
        	sprintf(CFG.www_hbgcolor, "Aqua");
        	sprintf(CFG.www_author, "Your Name");
        	sprintf(CFG.www_convert,"/usr/X11R6/bin/convert -geometry x100");
        	sprintf(CFG.www_icon_home, "up.gif");
        	sprintf(CFG.www_name_home, "Home");
        	sprintf(CFG.www_icon_back, "back.gif");
        	sprintf(CFG.www_name_back, "Back");
        	sprintf(CFG.www_icon_prev, "left.gif");
        	sprintf(CFG.www_name_prev, "Previous page");
        	sprintf(CFG.www_icon_next, "right.gif");
        	sprintf(CFG.www_name_next, "Next page");
        	CFG.www_files_page = 10;

		CFG.maxarticles = 500;

                if ((fp = fopen(cfgfn, "a+")) == NULL) {
			perror("");
                        fprintf(stderr, "Can't create %s\n", cfgfn);
                        exit(2);
                }
                fwrite(&CFG, sizeof(CFG), 1, fp);
                fclose(fp);
		chmod(cfgfn, 0640);
        } else {
                fread(&CFG, sizeof(CFG), 1, fp);
                fclose(fp);
        }

        cfg_time = file_time(cfgfn);
}



/*
 *  Load task configuration data.
 */
void load_taskcfg(void)
{
	FILE	*fp;

	if ((fp = fopen(tcfgfn, "r")) == NULL) {
		memset(&TCFG, 0, sizeof(TCFG));
		TCFG.maxload = 1.50;
		sprintf(TCFG.zmh_start, "02:30");
		sprintf(TCFG.zmh_end, "03:30");
		sprintf(TCFG.cmd_mailout,  "%s/bin/mbfido scan web -quiet", getenv("MBSE_ROOT"));
		sprintf(TCFG.cmd_mailin,   "%s/bin/mbfido tic toss web -quiet", getenv("MBSE_ROOT"));
		sprintf(TCFG.cmd_newnews,  "%s/bin/mbfido news web -quiet", getenv("MBSE_ROOT"));
		sprintf(TCFG.cmd_mbindex1, "%s/bin/mbindex -quiet", getenv("MBSE_ROOT"));
		sprintf(TCFG.cmd_mbindex2, "%s/bin/goldnode -f -q", getenv("MBSE_ROOT"));
		sprintf(TCFG.cmd_msglink,  "%s/bin/mbmsg link -quiet", getenv("MBSE_ROOT"));
		sprintf(TCFG.cmd_reqindex, "%s/bin/mbfile index -quiet", getenv("MBSE_ROOT"));
		TCFG.debug    = FALSE;
		TCFG.max_tcp  = 0;
		sprintf(TCFG.isp_ping1, "192.168.1.1");
		sprintf(TCFG.isp_ping2, "192.168.1.1");
		if ((fp = fopen(tcfgfn, "a+")) == NULL) {
			tasklog('?', "$Can't create %s", tcfgfn);
			die(2);
		}
		fwrite(&TCFG, sizeof(TCFG), 1, fp);
		fclose(fp);
		chmod(tcfgfn, 0640);
		tasklog('+', "Created new %s", tcfgfn);
	} else {
		fread(&TCFG, sizeof(TCFG), 1, fp);
		fclose(fp);
	}

	tcfg_time = file_time(tcfgfn);
}



/*
 *  Launch an external program in the background.
 *  On success add it to the tasklist and return
 *  the pid. Set the pause timer.
 */
pid_t launch(char *cmd, char *opts, char *name, int tasktype)
{
	char	buf[PATH_MAX];
	char	*vector[16];
	int	i, rc = 0;
	pid_t	pid = 0;

	if (checktasks(0) >= MAXTASKS) {
		tasklog('?', "Launch: can't execute %s, maximum tasks reached", cmd);
		return 0;
	}

	if (opts == NULL)
		sprintf(buf, "%s", cmd);
	else
		sprintf(buf, "%s %s", cmd, opts);

	i = 0;
	vector[i++] = strtok(buf," \t\n\0");
	while ((vector[i++] = strtok(NULL," \t\n")) && (i<16));
	vector[15] = NULL;

	if (file_exist(vector[0], X_OK)) {
		tasklog('?', "Launch: can't execute %s, command not found", vector[0]);
		return 0;
	}

	pid = fork();
	switch (pid) {
	case -1:
		tasklog('?', "$Launch: error, can't fork grandchild");
		return 0;
	case 0:
		/* From Paul Vixies cron: */
		(void)setsid(); /* It doesn't seem to help */
		close(0);
		if (open("/dev/null", O_RDONLY) != 0) {
			tasklog('?', "$Launch: \"%s\": reopen of stdin to /dev/null failed", buf);
			_exit(-1);
		}
		close(1);
		if (open("/dev/null", O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
			tasklog('?', "$Launch: \"%s\": reopen of stdout to /dev/null failed", buf);
			_exit(-1);
		}
		close(2);
		if (open("/dev/null", O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
			tasklog('?', "$Launch: \"%s\": reopen of stderr to /dev/null failed", buf);
			_exit(-1);
		}
		errno = 0;
		rc = execv(vector[0],vector);
		tasklog('?', "$Launch: execv \"%s\" failed, returned %d", cmd, rc);
		_exit(-1);
	default:
		/* grandchild's daddy's process */
		break;
	}

	/*
	 *  Add it to the tasklist.
	 */
	for (i = 0; i < MAXTASKS; i++) {
		if (strlen(task[i].name) == 0) {
			strcpy(task[i].name, name);
			strcpy(task[i].cmd, cmd);
			if (opts)
				strcpy(task[i].opts, opts);
			task[i].pid = pid;
			task[i].status = 0;
			task[i].running = TRUE;
			task[i].rc = 0;
			task[i].tasktype = tasktype;
			break;
		}
	}

	ptimer = PAUSETIME;

	if (opts)
		tasklog('+', "Launch: task %d \"%s %s\" success, pid=%d", i, cmd, opts, pid);
	else
		tasklog('+', "Launch: task %d \"%s\" success, pid=%d", i, cmd, pid);
	return pid;
}



/*
 *  Count specific running tasks
 */
int runtasktype(int tasktype)
{
	int	i, count = 0;

	for (i = 0; i < MAXTASKS; i++) {
		if (strlen(task[i].name) && task[i].running && (task[i].tasktype == tasktype))
			count++;
	}
	return count;
}



/*
 *  Check all running tasks registered in the tasklist.
 *  Report programs that are stopped. If signal is set
 *  then send that signal.
 */
int checktasks(int onsig)
{
    int	i, j, rc, count = 0, first = TRUE, status;

    for (i = 0; i < MAXTASKS; i++) {
	if (strlen(task[i].name)) {

	    if (onsig) {
		if (kill(task[i].pid, onsig) == 0)
		    tasklog('+', "%s to %s (pid %d) succeeded", SigName[onsig], task[i].name, task[i].pid);
		else
		    tasklog('+', "%s to %s (pid %d) failed", SigName[onsig], task[i].name, task[i].pid);
	    }

	    task[i].rc = wait4(task[i].pid, &status, WNOHANG | WUNTRACED, NULL);
	    if (task[i].rc) {
		task[i].running = FALSE;
		/*
		 * If a mailer call is finished, set the global rescan flag.
		 */
		if (task[i].tasktype == CM_POTS || task[i].tasktype == CM_ISDN || task[i].tasktype == CM_INET)
		    rescan = TRUE;
		ptimer = PAUSETIME;
	    }

	    if (first && task[i].rc) {
		first = FALSE;
		tasklog('t', "Task             Type      pid stat status      rc    status");
		tasklog('t', "---------------- ------- ----- ---- ----------- ----- --------");
		for (j = 0; j < MAXTASKS; j++)
		    if (strlen(task[j].name))
			tasklog('t', "%-16s %s %5d %s %-11d %5d %08x", task[j].name, callmode(task[j].tasktype), 
				task[j].pid, task[j].running?"runs":"stop", task[j].status, task[j].rc, task[j].status);
	    }

	    switch (task[i].rc) {
		case -1:
			if (errno == ECHILD)
			    tasklog('+', "Task %d \"%s\" is ready", i, task[i].name);
			else
			    tasklog('+', "Task %d \"%s\" is ready, error: %s", i, task[i].name, strerror(errno));
			break;
		case 0:
			/*
			 * Update last known status when running.
			 */
			task[i].status = status;
			count++;
			break;
		default:
			tasklog('+', "errno=%d %s", errno, strerror(errno));
			if (WIFEXITED(task[i].status)) {
			    rc = WEXITSTATUS(task[i].status);
			    if (rc)
				tasklog('+', "Task %s is ready, error=%d", task[i].name, rc);
			    else
				tasklog('+', "Task %s is ready", task[i].name);
			} else if (WIFSIGNALED(task[i].status)) {
			    rc = WTERMSIG(task[i].status);
			    if (rc <= 31)
				tasklog('+', "Task %s terminated on signal %s (%d)", task[i].name, SigName[rc], rc);
			    else
				tasklog('+', "Task %s terminated with error nr %d", task[i].name, rc);
			} else if (WIFSTOPPED(task[i].status)) {
			    rc = WSTOPSIG(task[i].status);
			    tasklog('+', "Task %s stopped on signal %s (%d)", task[i].name, SigName[rc], rc);
			} else {
			    tasklog('+', "FIXME: 1");
			}
			break;
	    }

	    if (!task[i].running) {
		for (j = 0; j < MAXTASKS; j++) {
		    if (calllist[j].taskpid == task[i].pid) {
			calllist[j].calling = FALSE;
			calllist[j].taskpid = 0;
			rescan = TRUE;
		    }
		}
		memset(&task[i], 0, sizeof(onetask));
	    }
	}
    }

    return count;
}



void die(int onsig)
{
	int	i, count;

	signal(onsig, SIG_IGN);
	tasklog('+', "Shutting down on signal %s", SigName[onsig]);

	/*
	 *  First check if there are tasks running, if so try to stop them
	 */
	if ((count = checktasks(0))) {
		tasklog('+', "There are %d tasks running, sending SIGTERM", count);
		checktasks(SIGTERM);
		for (i = 0; i < 15; i++) {
			sleep(1);
			count = checktasks(0);
			if (count == 0)
				break;
		}
		if (count) {
			/*
			 *  There are some diehards running...
			 */
			tasklog('+', "There are %d tasks running, sending SIGKILL", count);
			count = checktasks(SIGKILL);
		}
		if (count) {
			sleep(1);
			count = checktasks(0);
			if (count)
				tasklog('?', "Still %d tasks running, giving up", count);
		}
	}

	ulocktask();
	if (sock != -1)
		close(sock);
	if (ping_isocket != -1)
		close(ping_isocket);
	if (!file_exist(spath, R_OK)) {
		unlink(spath);
	}
	tasklog(' ', "MBTASK finished");
	exit(onsig);
}



/*
 *  Put a lock on this program.
 */
int locktask(char *root)
{
	char    Tmpfile[81];
	FILE    *fp;
	pid_t   oldpid;

	sprintf(Tmpfile, "%s/var/", root);
	strcpy(lockfile, Tmpfile);
	sprintf(Tmpfile + strlen(Tmpfile), "%s%u", TMPNAME, getpid());
	sprintf(lockfile + strlen(lockfile), "%s", LCKNAME);

	if ((fp = fopen(Tmpfile, "w")) == NULL) {
		perror("mbtask");
		printf("Can't create lockfile \"%s\"\n", Tmpfile);
		return 1;
	}
	fprintf(fp, "%10u\n", getpid());
	fclose(fp);

	while (TRUE) {
		if (link(Tmpfile, lockfile) == 0) {
			unlink(Tmpfile);
			return 0;
		}
		if ((fp = fopen(lockfile, "r")) == NULL) {
			perror("mbtask");
			printf("Can't open lockfile \"%s\"\n", Tmpfile);
			unlink(Tmpfile);
			return 1;
		}
		if (fscanf(fp, "%u", &oldpid) != 1) {
			perror("mbtask");
			printf("Can't read old pid from \"%s\"\n", Tmpfile);
			fclose(fp);
			unlink(Tmpfile);
			return 1;
		}
		fclose(fp);
		if (kill(oldpid,0) == -1) {
			if (errno == ESRCH) {
				printf("Stale lock found for pid %u\n", oldpid);
				unlink(lockfile);
				/* no return, try lock again */  
			} else {
				perror("mbtask");
				printf("Kill for %u failed\n",oldpid);
				unlink(Tmpfile);
				return 1;
			}
		} else {
			printf("Another mbtask is already running, pid=%u\n", oldpid);
			unlink(Tmpfile);
			return 1;
		}
	}
}



void ulocktask(void)
{
	if (lockfile)
		(void)unlink(lockfile);
}



/*
 *  External Semafore Checks
 */
void test_sema(char *);
void test_sema(char *sema)
{
	if (IsSema(sema)) {
		RemoveSema(sema);
		tasklog('s', "Semafore %s detected", sema);
		sem_set(sema, TRUE);
	}
}



/*
 *  Check semafore's, system status flags etc. This is called
 *  each second to test for condition changes.
 */
void check_sema(void);
void check_sema(void)
{
	/*
	 * Check UPS status.
	 */
        if (IsSema((char *)"upsalarm")) {
                if (!UPSalarm)
                        tasklog('!', "UPS: power failure");
		UPSalarm = TRUE;
	} else {
                if (UPSalarm)
                        tasklog('!', "UPS: the power is back");
		UPSalarm = FALSE; 
	}
	if (IsSema((char *)"upsdown")) {
		tasklog('!', "UPS: power failure, starting shutdown");
		/*
		 *  Since the upsdown semafore is permanent, the system WILL go down
		 *  there is no point for this program to stay. Signal all tasks and stop.
		 */
		die(SIGTERM);
	}

	/*
	 *  Check Zone Mail Hour
	 */
	get_zmh();

	/*
	 *  Semafore's that still can be detected, usefull for
	 *  external programs that create them.
	 */
	test_sema((char *)"newnews");
	test_sema((char *)"mailout");
	test_sema((char *)"mailin");
	test_sema((char *)"scanout");
}



void scheduler(void)
{
    struct passwd   *pw;
    int		    running = 0, rc, i, rlen, found;
    static int      LOADhi = FALSE, oldmin = 70, olddo = 70, oldsec = 70;
    char            *cmd = NULL, opts[41], port[21];
    static char	    doing[32], buf[2048];
    time_t          now;
    struct tm       *tm, *utm;
#if defined(__linux__)
    FILE	    *fp;
#endif
    struct pollfd   pfd;
    int		    call_work = 0;
    static int	    call_entry = MAXTASKS;
    double	    loadavg[3];
    pp_list	    *tpl;

    InitFidonet();

    /*
     * Registrate this server for mbmon in slot 0.
     */
    reginfo[0].pid = getpid();
    strcpy(reginfo[0].tty,   "-");
    strcpy(reginfo[0].uname, "mbse");
    strcpy(reginfo[0].prg,   "mbtask");
    strcpy(reginfo[0].city,  "localhost");
    strcpy(reginfo[0].doing, "Start");
    reginfo[0].started = time(NULL);

    Processing = TRUE;
    TouchSema((char *)"mbtask.last");
    pw = getpwuid(getuid());

    /*
     * Setup UNIX Datagram socket
     */
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
	tasklog('?', "$Can't create socket");
	die(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, spath);

    if (bind(sock, &servaddr, sizeof(servaddr)) < 0) {
	close(sock);
	sock = -1;
	tasklog('?', "$Can't bind socket %s", spath);
	die(1);
    }

    pingresult[1] = TRUE;
    pingresult[2] = TRUE;

    /*
     * The flag masterinit is set if a new config.data is created, this
     * is true if mbtask is started the very first time. Then we run
     * mbsetup init to create the default databases.
     */
    if (masterinit) {
	cmd = xstrcpy(pw->pw_dir);
	cmd = xstrcat(cmd, (char *)"/bin/mbsetup");
	launch(cmd, (char *)"init", (char *)"mbsetup", MBINIT);
	free(cmd);
	sleep(2);
	masterinit = FALSE;
    }

    initnl();
    sem_set((char *)"scanout", TRUE);
    if (!TCFG.max_tcp && !pots_lines && !isdn_lines) {
	tasklog('?', "ERROR: this system cannot connect to other systems, check setup");
    }
	
    /*
     * Enter the mainloop (forever)
     */
    do {
	/*
	 *  Poll UNIX Datagram socket until the defined timeout of one second.
	 *  This means we listen of a MBSE BBS client program has something
	 *  to tell.
	 */
	pfd.fd = sock;
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;
	rc = poll(&pfd, 1, 1000);
	if (rc == -1) {
	    /*
	     *  Poll can be interrupted by a finished child so that's not a real error.
	     */
	    if (errno != EINTR) {
		tasklog('?', "$poll() rc=%d sock=%d, events=%04x", rc, sock, pfd.revents);
	    }
	} else if (rc) {
	    if (pfd.revents & POLLIN) {
		/*
		 * Process the clients request
		 */
		memset(&buf, 0, sizeof(buf));
		fromlen = sizeof(from);
		rlen = recvfrom(sock, buf, sizeof(buf) -1, 0, &from, &fromlen);
		do_cmd(buf);
	    } else {
		tasklog('-', "Return poll rc=%d, events=%04x", rc, pfd.revents);
	    }
	}

	/*
	 * Check all registered connections and semafore's
	 */
	reg_check();
	check_sema();
	check_ports();

	/*
	 * Check the systems load average.
	 */
	Load = loadavg[0] = loadavg[1] = loadavg[2] = 0.0;
#if defined(__FreeBSD__) || defined(__NetBSD__)
	if (getloadavg(loadavg, 3) == 3) {
	    Load = loadavg[0];
	}
#elif defined(__linux__)
	if ((fp = fopen((char *)"/proc/loadavg", "r"))) {
	    if (fscanf(fp, "%lf %lf %lf", &loadavg[0], &loadavg[1], &loadavg[2]) == 3) {
		Load = loadavg[0];
	    } else {
		tasklog('-', "error");
	    }
	    fclose(fp);
	}
#endif
	if (Load >= TCFG.maxload) {
	    if (!LOADhi) {
		tasklog('!', "System load too high: %2.2f (%2.2f)", Load, TCFG.maxload);
		LOADhi = TRUE;
	    }
	} else {
	    if (LOADhi) {
		tasklog('!', "System load normal: %2.2f (%2.2f)", Load, TCFG.maxload);
		LOADhi = FALSE;
	    }
	}

	/*
	 * Report to the system monitor. 
	 */
	memset(&doing, 0, sizeof(doing));
	if ((running = checktasks(0)))
	    sprintf(doing, "Run %d tasks", running);
	else if (UPSalarm)
	    sprintf(doing, "UPS alarm");
	else if (!s_bbsopen)
	    sprintf(doing, "BBS is closed");
	else if (Processing)
	    sprintf(doing, "Waiting (%d)", oldmin);
	else
	    sprintf(doing, "Overload %2.2f", Load);

	sprintf(reginfo[0].doing, "%s", doing);
	reginfo[0].lastcon = time(NULL);

	/*
	 *  Touch the mbtask.last semafore to prove this daemon
	 *  is actually running.
	 *  Reload configuration data if some file is changed.
	 */
	now = time(NULL);
	tm = localtime(&now);
	utm = gmtime(&now);
	if (tm->tm_min != olddo) {
	    /*
	     * Each minute we execute this part
	     */
	    olddo = tm->tm_min;
	    TouchSema((char *)"mbtask.last");
	    if (file_time(tcfgfn) != tcfg_time) {
		tasklog('+', "Task configuration changed, reloading");
		load_taskcfg();
		sem_set((char *)"scanout", TRUE);
	    }
	    if (file_time(cfgfn) != cfg_time) {
		tasklog('+', "Main configuration changed, reloading");
		load_maincfg();
		sem_set((char *)"scanout", TRUE);
	    }
	    if (file_time(ttyfn) != tty_time) {
		tasklog('+', "Ports configuration changed, reloading");
		load_ports();
		check_ports();
		sem_set((char *)"scanout", TRUE);
	    }

	    /*
	     * If the next event time is reached, rescan the outbound
	     */
	    if ((utm->tm_hour == nxt_hour) && (utm->tm_min == nxt_min)) {
		tasklog('+', "It is now %02d:%02d UTC, starting new event", utm->tm_hour, utm->tm_min);
		sem_set((char *)"scanout", TRUE);
	    }
	}

	if (s_bbsopen && !UPSalarm && !LOADhi) {

	    /*
	     * Check Pause Timer, make sure it's only checked
	     * once each second.
	     */
	    if (tm->tm_sec != oldsec) {
		oldsec = tm->tm_sec;
		if (ptimer)
		    ptimer--;
	    }

	    if (!Processing) {
		tasklog('+', "Resuming normal operations");
		Processing = TRUE;
	    }

	    /*
	     *  Here we run all normal operations.
	     */
	    running = checktasks(0);

	    if (s_mailout && (!ptimer) && (!runtasktype(MBFIDO))) {
		launch(TCFG.cmd_mailout, NULL, (char *)"mailout", MBFIDO);
		running = checktasks(0);
		s_mailout = FALSE; 
	    }

	    if (s_mailin && (!ptimer) && (!runtasktype(MBFIDO))) {
		launch(TCFG.cmd_mailin, NULL, (char *)"mailin", MBFIDO);
		running = checktasks(0);
		s_mailin = FALSE;
	    }

	    if (s_newnews && (!ptimer) && (!runtasktype(MBFIDO))) {
		launch(TCFG.cmd_newnews, NULL, (char *)"newnews", MBFIDO);
		running = checktasks(0);
		s_newnews = FALSE; 
	    }

	    /*
	     *  Only run the nodelist compiler if nothing else
	     *  is running. There's no hurry to compile the
	     *  new lists. If more then one compiler is defined,
	     *  start them in parallel.
	     */
	    if (s_index && (!ptimer) && (!running)) {
		if (strlen(TCFG.cmd_mbindex1))
		    launch(TCFG.cmd_mbindex1, NULL, (char *)"compiler 1", MBINDEX);
		if (strlen(TCFG.cmd_mbindex2))
		    launch(TCFG.cmd_mbindex2, NULL, (char *)"compiler 2", MBINDEX);
		if (strlen(TCFG.cmd_mbindex3))
		    launch(TCFG.cmd_mbindex3, NULL, (char *)"compiler 3", MBINDEX);
		running = checktasks(0);
		s_index = FALSE;
	    }

	    /*
	     *  Linking messages is also only done when there is
	     *  nothing else to do.
	     */
	    if (s_msglink && (!ptimer) && (!running)) {
		launch(TCFG.cmd_msglink, NULL, (char *)"msglink", MBFIDO);
		running = checktasks(0);
		s_msglink = FALSE;
	    }

	    /*
	     *  Creating filerequest indexes, also only if nothing to do.
	     */
	    if (s_reqindex && (!ptimer) && (!running)) {
		launch(TCFG.cmd_reqindex, NULL, (char *)"reqindex", MBFILE);
		running = checktasks(0);
		s_reqindex = FALSE;
	    }

	    if ((tm->tm_sec / SLOWRUN) != oldmin) {

		/*
		 *  These tasks run once per 20 seconds.
		 */
		oldmin = tm->tm_sec / SLOWRUN;

		check_ping();

		/*
		 * Update outbound status if needed.
		 */
		if (rescan) {
		    rescan = FALSE;
		    outstat();
		    call_work = check_calllist();
		}

		/*
		 * Launch the systems to call, start one system each time.
		 * Set the safety counter to MAXTASKS + 1, this forces that
		 * the counter really will advance to the next node in case
		 * of failing sessions.
		 */
		i = MAXTASKS + 1;
		found = FALSE;
		if (call_work) {
		    while (TRUE) {
			/*
			 * Rotate the call entries
			 */
			if (call_entry == MAXTASKS)
			    call_entry = 0;
			else
			    call_entry++;

			/*
			 * If a valid entry, and not yet calling, and the retry time is reached,
			 * then launch a callprocess for this node.
			 */
			if (calllist[call_entry].addr.zone && !calllist[call_entry].calling && 
				(calllist[call_entry].cst.trytime < now)) {
			    if ((calllist[call_entry].callmode == CM_INET) && (ipmailers < TCFG.max_tcp) && internet) {
				found = TRUE;
				break;
			    }
			    if ((calllist[call_entry].callmode == CM_ISDN) && (runtasktype(CM_ISDN) < isdn_free)) {
				found = TRUE;
				break;
			    }
			    if ((calllist[call_entry].callmode == CM_POTS) && (runtasktype(CM_POTS) < pots_free)) {
				found = TRUE;
				break;
			    }
			}

			/*
			 * Safety counter, if all systems are already calling, we should
			 * never break out of this loop anymore.
			 */
			i--;
			if (!i)
			    break;
		    }
		    if (found) {
			cmd = xstrcpy(pw->pw_dir);
			cmd = xstrcat(cmd, (char *)"/bin/mbcico");
			/*
			 * For ISDN or POTS, select a free tty device.
			 */
			switch (calllist[call_entry].callmode) {
			    case CM_ISDN:   for (tpl = pl; tpl; tpl = tpl->next) {
						if (!tpl->locked && (tpl->dflags  & calllist[call_entry].diflags)) {
						    sprintf(port, "-l %s ", tpl->tty);
						    break;
						}
					    }
					    break;
			    case CM_POTS:   for (tpl = pl; tpl; tpl = tpl->next) {
						if (!tpl->locked && (tpl->mflags  & calllist[call_entry].moflags)) {
						    sprintf(port, "-l %s ", tpl->tty);
						    break;
						}
					    }
					    break;
			    default:	    port[0] = '\0';
					    break;
			}
			sprintf(opts, "%sf%u.n%u.z%u", port, calllist[call_entry].addr.node, calllist[call_entry].addr.net,
				calllist[call_entry].addr.zone);
			calllist[call_entry].taskpid = launch(cmd, opts, (char *)"mbcico", calllist[call_entry].callmode);
			if (calllist[call_entry].taskpid)
			    calllist[call_entry].calling = TRUE;
			running = checktasks(0);
			rescan = TRUE;
			free(cmd);
			cmd = NULL;
		    }
		}
	    }

	    /*
	     * PING state changes
	     */
	    state_ping();

	} else {
	    if (Processing) {
		tasklog('+', "Suspending operations");
		Processing = FALSE;
	    }
	}
    } while (TRUE);
}



int main(int argc, char **argv)
{
        struct passwd   *pw;
        int             i;
        pid_t           frk;
        FILE            *fp;

       /*
         * Print copyright notices and setup logging.
         */
        printf("MBTASK: MBSE BBS v%s Task Manager Daemon\n", VERSION);
        printf("        %s\n\n", COPYRIGHT);

        /*
         *  Catch all the signals we can, and ignore the rest. Note that SIGKILL can't be ignored
         *  but that's live. This daemon should only be stopped by SIGTERM.
         */
        for(i = 0; i < NSIG; i++) {
                if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
                        signal(i, (void (*))die);
                else
                        signal(i, SIG_IGN);
        }

	/*
	 *  Create the ping socket while we are still root.
	 */
	if ((ping_isocket = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("");
		printf("socket init failed, is mbtask not installed setuid root?\n");
		exit(1);
	}

	/*
	 *  mbtask is setuid root, drop privileges to user mbse.
	 *  This will stay forever like this, no need to become
	 *  root again. The child can't even become root anymore.
	 */
        pw = getpwnam((char *)"mbse");
	if (setuid(pw->pw_uid)) {
		perror("");
		printf("can't setuid to mbse\n");
		close(ping_isocket);
		exit(1);
	}
	if (setgid(pw->pw_gid)) {
		perror("");
		printf("can't setgid to bbs\n");
		close(ping_isocket);
		exit(1);
	}

	umask(007);
        if (locktask(pw->pw_dir)) {
		close(ping_isocket);
                exit(1);
        }

	sprintf(cfgfn, "%s/etc/config.data", getenv("MBSE_ROOT"));
	load_maincfg();

        tasklog(' ', " ");
        tasklog(' ', "MBTASK v%s", VERSION);
	sprintf(tcfgfn, "%s/etc/task.data", getenv("MBSE_ROOT"));
        load_taskcfg();
        status_init();

        memset(&task, 0, sizeof(task));
	memset(&reginfo, 0, sizeof(reginfo));
	memset(&calllist, 0, sizeof(calllist));
	sprintf(spath, "%s/tmp/mbtask", getenv("MBSE_ROOT"));

	sprintf(ttyfn, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	load_ports();
	check_ports();

	/*
	 * Now that init is complete and this program is locked, it is
	 * safe to remove a stale socket if it is there after a crash.
	 */
        if (!file_exist(spath, R_OK))
                unlink(spath);

        /*
         * Server initialization is complete. Now we can fork the 
         * daemon and return to the user. We need to do a setpgrp
         * so that the daemon will no longer be assosiated with the
         * users control terminal. This is done before the fork, so
         * that the child will not be a process group leader. Otherwise,
         * if the child were to open a terminal, it would become
         * associated with that terminal as its control terminal.
         */
	if ((pgrp = setpgid(0, 0)) == -1) {
		tasklog('?', "$setpgid failed");
		die(0);
	}

	frk = fork();
        switch (frk) {
        case -1:
                tasklog('?', "$Unable to fork daemon");
                die(0);
        case 0:
                /*
                 *  Starting the deamon child process here. 
                 */
                fclose(stdin);
		fclose(stdout);
                fclose(stderr);
                scheduler();
		/* Not reached */
        default:
                /*
                 * Here we detach this process and let the child
                 * run the deamon process. Put the child's pid
                 * in the lockfile before leaving.
                 */
                if ((fp = fopen(lockfile, "w"))) {
                        fprintf(fp, "%10u\n", frk);
                        fclose(fp);
                }
                tasklog('+', "Starting daemon with pid %d", frk);
                exit(0);
        }

        /*
         *  Not reached
         */
        return 0;
}



