/*****************************************************************************
 *
 * File ..................: mbcico/respfreq.c
 * Purpose ...............: Fidonet mailer 
 * Last modification date : 08-Aug-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "session.h"
#include "lutil.h"
#include "config.h"
#include "atoul.h"
#include "respfreq.h"
#include "filelist.h"


#ifndef S_ISDIR
#define S_ISDIR(st_mode)       (((st_mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(st_mode)       (((st_mode) & S_IFMT) == S_IFREG)
#endif


extern char *freqname;

static void		attach_report(file_list**);
static void		add_report(char *, ...);
static char		*report_text = NULL;
static unsigned long	report_total = 0L;
static unsigned long	report_count = 0L;
static int		no_more = FALSE;



file_list *respond_wazoo(void)
{
	char		buf[256];
	char		*nm, *pw, *dt, *p;
	file_list	*fl=NULL, **tmpl;
	FILE		*fp;

	if (freqname == NULL) 
		return NULL;

	if ((fp=fopen(freqname,"r")) == NULL) {
		WriteError("$cannot open received wazoo freq \"%s\"",freqname);
		unlink(freqname);
		free(freqname);
		freqname=NULL;
		return NULL;
	}

	tmpl=&fl;
	while (fgets(buf,sizeof(buf)-1,fp)) {
		nm = NULL;
		pw = NULL;
		dt = NULL;
		p = strtok(buf," \n\r");
		if ((p == NULL) || (*p == '\0')) 
			continue;
		nm = p;
		p = strtok(NULL," \n\r");
		if (p && (*p == '!')) 
			pw = p+1;
		else 
			if (p && ((*p == '+') || (*p == '-'))) 
				dt = p;
		p = strtok(NULL," \n\r");
		if (p && (*p == '!')) 
			pw = p+1;
		else 
			if (p && ((*p == '+') || (*p == '-'))) 
				dt = p;
		*tmpl = respfreq(nm, pw, dt);
		while (*tmpl) tmpl=&((*tmpl)->next);
		if (no_more)
			break;
	}

	fclose(fp);
	unlink(freqname);
	free(freqname);
	freqname = NULL;
	for (tmpl = &fl; *tmpl; tmpl = &((*tmpl)->next)) {
		Syslog('F', "resplist: %s",MBSE_SS((*tmpl)->local));
	}
	attach_report(&fl);
	return fl;
}



file_list *respond_bark(char *buf)
{
	char		*nm, *pw, *dt, *p;
	file_list	*fl;

	nm = buf;
	pw = (char *)"";
	dt = (char *)"0";
	while (isspace(*nm)) 
		nm++;
	for (p = nm; *p && (!isspace(*p)); p++);
	if (*p) {
		*p++ = '\0';
		dt = p;
		while (isspace(*dt)) 
			dt++;
		for (p = dt; *p && (!isspace(*p)); p++);
		if (*p) {
			*p++ = '\0';
			pw = p;
			while (isspace(*pw)) 
				pw++;
			for (p = pw; *p && (!isspace(*p)); p++);
			*p = '\0';
		}
	}
	fl = respfreq(nm, pw, dt);
	attach_report(&fl);
	return fl;
}



file_list *respfreq(char *nm, char *pw, char *dt)
{
	file_list		*fl = NULL;
	struct stat		st;
	char			mask[256], *p, *q;
	char			*tnm, *t;
	time_t			upd = 0L;
	int			newer = 1;
	FILE			*fa, *fb, *fi;
	long			Area;
	int			Send;
	struct FILEIndex	idx;

	if (localoptions & NOFREQS) {
		Syslog('+', "File requests disabled");
		add_report((char *)"ER: \"%s\" denied: file requests disabled", nm);
		return NULL;
	}

	if (strchr(nm, '/') || strchr(nm, '\\') || strchr(nm, ':')) {
		Syslog('+', "Illegal characters in request");
		add_report((char *)"ER: \"%s\" denied: illegal characters", nm);
		return NULL;
	}

	if (dt) {
		if (*dt == '+') {
			newer = 1;
			dt++;
		} else if (*dt == '-') {
			newer = 0;
			dt++;
		} else 
			newer = 1;
		upd=atoul(dt);
	}

	if (strlen(CFG.req_magic)) {
		/*
		 * Check for uppercase and lowercase magic names.
		 */
		tnm = xstrcpy(CFG.req_magic);
		tnm = xstrcat(tnm,(char *)"/");
		tnm = xstrcat(tnm,tl(xstrcpy(nm)));
		if ((stat(tnm, &st) == 0) &&
		    (S_ISREG(st.st_mode))) {
			if (access(tnm, X_OK) == 0) {
				return respmagic(tnm);
				/* respmagic will free(tnm) */
			} else if (access(tnm, R_OK) == 0) {
				return resplist(tnm, pw, dt);
				/* resplist will free(tnm) */
			} else 
				free(tnm);
		} else 
			free(tnm);

		tnm = xstrcpy(CFG.req_magic);
		tnm = xstrcat(tnm,(char *)"/");
		t = xstrcpy(nm);
		tnm = xstrcat(tnm,tu(t));
		free(t);
		if ((stat(tnm, &st) == 0) &&
		    (S_ISREG(st.st_mode))) {
			if (access(tnm, X_OK) == 0) {
				return respmagic(tnm);
				/* respmagic will free(tnm) */
			} else if (access(tnm, R_OK) == 0) {
				return resplist(tnm, pw, dt);
				/* resplist will free(tnm) */
			} else 
				free(tnm);
		} else free(tnm);
	}

	Syslog('+', "File request : %s (update (%s), password \"%s\")",MBSE_SS(nm),MBSE_SS(dt),MBSE_SS(pw));
	add_report((char *)"RQ: Regular file \"%s\"",nm);
	p = tl(nm);
	q = mask;
	*q++ = '^';
	while ((*p) && (q < (mask + sizeof(mask) - 4))) {
		switch (*p) {
			case '\\':	*q++ = '\\'; *q++ = '\\'; break;
			case '?':	*q++ = '.';  break;
			case '.':	*q++ = '\\'; *q++ = '.'; break;
			case '+':	*q++ = '\\'; *q++ = '+'; break;
			case '*':	*q++ = '.';  *q++ = '*'; break;
			default:	*q++ = toupper(*p);   break;
		}
		p++;
	}
	*q++ = '$';
	*q = '\0';
	Syslog('f', "Search mask \"%s\"", mask);
	re_comp(mask);

	/*
	 * Open the areas database and request index.
	 */
	p = xstrcpy(getenv("MBSE_ROOT"));
	p = xstrcat(p, (char *)"/etc/fareas.data");
	if ((fa = fopen(p, "r")) == NULL) {
		WriteError("$Can't open %s", p);
		return NULL;
	}
	free(p);
	fread(&areahdr, sizeof(areahdr), 1, fa);

	p = xstrcpy(getenv("MBSE_ROOT"));
	p = xstrcat(p, (char *)"/etc/request.index");
	if ((fi = fopen(p, "r")) == NULL) {
		WriteError("$Can't open %s", p);
		return NULL;
	}
	Area = 0L;
	free(p);

	Syslog('f', "Start search ...");
	while (!no_more && (fread(&idx, sizeof(idx), 1, fi) == 1)) {
		if (re_exec(idx.Name) || re_exec(idx.LName)) {
			Syslog('f', "Index found %s area %d record %d", idx.LName, idx.AreaNum, idx.Record);
			if (fseek(fa, ((idx.AreaNum - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET) == -1) {
				WriteError("$Can't seek in fareas.data");
				return NULL;
			}
			if (fread(&area, areahdr.recsize, 1, fa) != 1) {
				WriteError("$Can't read record %d in fareas.data", idx.AreaNum);
				return NULL;
			}
			Syslog('f', "Area %s", area.Name);
			p = calloc(128, sizeof(char));
			sprintf(p, "%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), idx.AreaNum);
			if ((fb = fopen(p, "r+")) == NULL) {
				WriteError("$Can't open %s", p);
				free(p);
			} else {
				free(p);
				if (fseek(fb, idx.Record * sizeof(file), SEEK_SET) == -1) {
					WriteError("$Can't seek filerecord %d", idx.Record);
				} else {
					if (fread(&file, sizeof(file), 1, fb) != 1) {
						WriteError("$Can't read filerecord %d", idx.Record);
					} else {
						Send = FALSE;
						Syslog('f', "Found \"%s\" in %s", file.Name, area.Name);
						tnm = xstrcpy(area.Path);
						tnm = xstrcat(tnm, (char *)"/");
						tnm = xstrcat(tnm, file.Name);
						if ((stat(tnm, &st) == 0) &&
						    (S_ISREG(st.st_mode)) &&
						    (access(tnm, R_OK) == 0) &&
						    ((upd == 0L) ||
						     ((newer) && (st.st_mtime <= upd)))) {
							Send = TRUE;
						}

						/*
						 * If no password is given, we do not respond
						 * on requests to password protected areas
						 * or files in case it was a wildcard request.
						 * Wrong passwords are honored with an error
						 * response.
						 */
						if (Send && strlen(area.Password)) {
							if (pw != NULL) {
								if (strcasecmp(area.Password, pw)) {
									Send = FALSE;
									Syslog('+', "Bad password for area %s", area.Name);
									add_report((char *)"ER: bad password for area %s", area.Name);
								}
							} else {
								Send = FALSE;
							}
						}

						if (Send && strlen(file.Password)) {
							if (pw != NULL) {
								if (strcasecmp(file.Password, pw)) {
									Send = FALSE;
									Syslog('+', "Bad password for file %s", file.Name);
									add_report((char *)"ER: bad password for file %s", file.Name);
								}
							} else {
								Send = FALSE;
							}
						}

						if (Send && CFG.Req_Files) {
							if (report_count >= CFG.Req_Files) {
								Send = FALSE;
								add_report((char *)"ER: too many files requested");
								Syslog('+', "Exceeding maximum files limit");
								no_more = TRUE;
							}
						}

						if (Send && CFG.Req_MBytes) {
							if ((st.st_size + report_total) > (CFG.Req_MBytes * 1048576)) {
								Send = FALSE;
								add_report((char *)"ER: file %s will exceed the request limit", file.Name);
								Syslog('+', "Exceeding request size limit");
								no_more = TRUE;
							}
						}

						if (Send) {
							Syslog('f', "Will send %s", file.LName);
							report_total += st.st_size;
							report_count++;
							add_report((char *)"OK: Sending \"%s\" (%lu bytes)", file.Name, file.Size);
							add_list(&fl, tnm, file.Name, 0, 0L, NULL, 1);
							/*
							 * Update file information
							 */
							file.TimesReq++;
							file.LastDL = time(NULL);
							fseek(fb, - sizeof(file), SEEK_CUR);
							fwrite(&file, sizeof(file), 1, fb);
						}
						free(tnm);
					}
				}
				fclose(fb);
			}
		}
	}

 	fclose(fa);
	fclose(fi);

	if (fl == NULL)
		add_report((char *)"ER: No matching files found");

	return fl;
}



#define MAXRECURSE 5
static int recurse=0;

/*
 * Magic filerquests.
 */
file_list *resplist(char *listfn, char *pw, char *dt)
{
	FILE		*fp;
	char		buf[256], *p;
	file_list	*fl = NULL, **pfl;

	Syslog('+', "Magic request: %s (update (%s), password \"%s\")", 
		strrchr(xstrcpy(listfn), '/')+1, MBSE_SS(dt), MBSE_SS(pw));

	if (++recurse > MAXRECURSE) {
		WriteError("Excessive recursion in file lists for \"%s\"", MBSE_SS(listfn));
		add_report((char *)"ER: Exessive recursion for magic filename \"%s\", contact sysop", listfn);
		recurse = 0;
		free(listfn);
		return NULL;
	}

	pfl = &fl;
	if ((fp = fopen(listfn,"r")) == NULL) {
		WriteError("$cannot open file list \"%s\"",listfn);
		add_report((char *)"ER: Could not open magic file \"%s\", contact sysop", listfn);
		free(listfn);
		recurse--;
		return NULL;
	}

	p = xstrcpy(listfn);
	add_report((char *)"RQ: Magic filename \"%s\"", strrchr(p, '/')+1);
	free(p);

	while (fgets(buf, sizeof(buf)-1, fp)) {
		if ((p = strchr(buf, '#'))) 
			*p = '\0';
		if ((p = strtok(buf," \t\n\r"))) {
			*pfl = respfreq(p, pw, dt);
			while (*pfl) 
				pfl = &((*pfl)->next);
		}
	}
	fclose(fp);

	free(listfn);
	recurse--;
	return fl;
}



file_list *respmagic(char *cmd) /* must free(cmd) before exit */
{
	struct stat	st;
	char		cmdbuf[256];
	char		tmpfn[PATH_MAX], tmptx[PATH_MAX];
	char		remname[32], *p, *q, *z, *buf;
	int		i, escaped;
	file_list	*fl = NULL;
	FILE		*fp, *ft;
	long		zeroes = 0L;
	ftnmsg		fmsg;
	char		*svname;

	Syslog('+', "Magic execute: %s", strrchr(xstrcpy(cmd), '/')+1);
	add_report((char *)"RQ: Magic \"%s\"",cmd);
	sprintf(tmpfn, "%s/tmp/%08lX", getenv((char *)"MBSE_ROOT"), (unsigned long)sequencer());
	Syslog('+', "tmpfn \"%s\"", tmpfn);
	if ((p = strrchr(cmd,'/'))) 
		p++;
	else 
		p = cmd;
	strncpy(remname, p, sizeof(remname)-1);
	remname[sizeof(remname)-1] = '\0';
	if (remote->addr->name == NULL) 
		remote->addr->name = xstrcpy((char *)"Sysop");
	strncpy(cmdbuf, cmd, sizeof(cmdbuf)-2);
	cmdbuf[sizeof(cmdbuf)-2] = '\0';
	q = cmdbuf + strlen(cmdbuf);
	z = cmdbuf + sizeof(cmdbuf)-2;
	*q++ = ' ';
	escaped = 0;
	for (p = ascfnode(remote->addr,0x7f); *p && (q < z); p++) {
		if (escaped) {
			escaped = 0;
		} else switch (*p) {
			case '\\': escaped = 1; break;
			case '\'':
			case '`':
			case '"':
			case '(':
			case ')':
			case '<':
			case '>':
			case '|':
			case ';':
			case '$': *q++ = '\\'; break;
		}
		*q++ = *p;
	}
	*q++ = '\0';

	/*
	 *  Execute the shell, output goes into tmpfn
	 */
	if (execsh(cmdbuf,(char *)"/dev/null",tmpfn,(char *)"/dev/null")) {
		WriteError("$Execute magic error");
		add_report((char *)"ER: Magic command execution failed");
		unlink(tmpfn);
	} else {
		if (stat(tmpfn, &st) == 0) {
			sprintf(tmptx, "%s/tmp/%08lX", getenv((char *)"MBSE_ROOT"), (unsigned long)sequencer());
			Syslog('+', "tmptx \"%s\"", tmptx);

			if ((fp = fopen(tmptx, "w"))) {
				fmsg.flags = M_PVT|M_KILLSENT;
				fmsg.from = bestaka_s(remote->addr);
				svname = fmsg.from->name;
				fmsg.from->name = (char *)"mbcico FREQ processor";
				fmsg.to = remote->addr;
				fmsg.date = time((time_t*)NULL);
				fmsg.subj = strrchr(xstrcpy(cmd), '/')+1;
				fmsg.msgid_s = NULL;
				fmsg.msgid_a = xstrcpy(ascfnode(fmsg.from, 0x1f));
				fmsg.msgid_n = sequencer();
				fmsg.reply_s = NULL;
				fmsg.reply_a = NULL;
				fmsg.reply_n = 0L;
				fmsg.origin  = NULL;
				fmsg.area    = NULL;
				(void)ftnmsghdr(&fmsg, fp, NULL, 'f', (char *)"MBSE-CICO");
				free(fmsg.msgid_a);

				if ((ft = fopen(tmpfn, "r")) == NULL) {
					WriteError("$Can't open %s", tmpfn);
				} else {
					buf = calloc(2049, sizeof(char));
					while ((fgets(buf, 2048, ft)) != NULL) {
						for (i = 0; i < strlen(buf); i++) {
							if (*(buf + i) == '\0')
								break;
							if (*(buf + i) == '\n')
								*(buf + i) = '\r';
						}
						fputs(buf, fp);
					}
					fprintf(fp, "\r--- mbcico v%s\r", VERSION);
					free(buf);
				}
				fwrite(&zeroes, 1, 3, fp);
				fclose(fp);
				sprintf(remname, "%08lX.PKT", (unsigned long)sequencer());

				add_list(&fl, tmptx, remname, KFS, 0L, NULL, 0);
				fmsg.from->name = svname;
				add_report((char *)"OK: magic output is send by mail");
				unlink(tmpfn);
			} else {
				WriteError("$cannot open temp file \"%s\"",MBSE_SS(tmpfn));
			}
		} else {
			WriteError("$cannot stat() magic stdout \"%s\"",tmpfn);
			add_report((char *)"ER: Could not get magic command output, contact sysop");
		}
	}

	free(cmd);
	return fl;
}



static void attach_report(file_list **fl)
{
	FILE	*fp;
	char	tmpfn[L_tmpnam];
	char	remname[14];
	long	zeroes = 0L;
	ftnmsg	fmsg;
	char	*svname;

	if (report_text == NULL) {
		WriteError("Empty FREQ report");
		add_report((char *)"ER: empty request report, contact sysop");
	}

	add_report((char *)"\rTotal to send: %lu files, %lu bytes", report_count, report_total);

	if (!CFG.Req_Files)
		add_report((char *)"Maximum files: unlimited");
	else
		add_report((char *)"Maximum files: %d", CFG.Req_Files);
	if (!CFG.Req_MBytes)
		add_report((char *)"Maximum size : unlimited");
	else
		add_report((char *)"Maximum size : %d MBytes", CFG.Req_MBytes);

	add_report((char *)"\r--- mbcico v%s\r", VERSION);

	(void)tmpnam(tmpfn);

	if ((fp = fopen(tmpfn,"w"))) {
		fmsg.flags = M_PVT|M_KILLSENT;
		fmsg.from = bestaka_s(remote->addr);
		svname = fmsg.from->name;
		fmsg.from->name = (char *)"mbcico FREQ processor";
		fmsg.to = remote->addr;
		/*
		 * If we don't know the sysops name, fake it.
		 */
		if (fmsg.to->name == NULL)
			fmsg.to->name = xstrcpy((char *)"Sysop");
		fmsg.date = time((time_t*)NULL);
		fmsg.subj = (char *)"File request status report";
		fmsg.msgid_s = NULL;
		fmsg.msgid_a = xstrcpy(ascfnode(fmsg.from, 0x1f));
		fmsg.msgid_n = sequencer();
		fmsg.reply_s = NULL;
		fmsg.reply_a = NULL;
		fmsg.reply_n = 0L;
		fmsg.origin  = NULL;
		fmsg.area    = NULL;
		(void)ftnmsghdr(&fmsg, fp, NULL, 'f', (char *)"MBSE-CICO");
		free(fmsg.msgid_a);
		fwrite(report_text, 1, strlen(report_text), fp);
		fwrite(&zeroes, 1, 3, fp);
		fclose(fp);
		sprintf(remname, "%08lX.PKT", (unsigned long)sequencer());
		add_list(fl, tmpfn, remname, KFS, 0L, NULL, 0);
		fmsg.from->name = svname;
	} else {
		WriteError("$cannot open temp file \"%s\"",MBSE_SS(tmpfn));
	}

	report_total = 0L;
	free(report_text);
	report_text = NULL;
}



static void add_report(char *format, ...)
{
	va_list	va_ptr;
	char	buf[1024];

	if (report_text == NULL) {
		sprintf(buf,
"                    Status of file request\r\
                    ======================\r\r\
                    Received By: %s\r\
",
			ascfnode(bestaka_s(remote->addr),0x1f));
		sprintf(buf+strlen(buf),
"                           From: %s\r\
                             On: %s\r\r\
",
			ascfnode(remote->addr,0x1f),
			date(0L));
		report_text = xstrcat(report_text,buf);
	}

	va_start(va_ptr, format);
	vsprintf(buf, format, va_ptr);
	va_end(va_ptr);
	strcat(buf,"\r");
	report_text = xstrcat(report_text,buf);
}

