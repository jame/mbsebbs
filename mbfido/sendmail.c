/*****************************************************************************
 *
 * File ..................: tosser/sendmail.c
 * Purpose ...............: Output a netmail to one of our links.
 * Last modification date : 11-Mar-2001
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
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/dbnode.h"
#include "../lib/clcomm.h"
#include "../lib/dbmsgs.h"
#include "addpkt.h"
#include "rollover.h"
#include "sendmail.h"



/*
 *  Start a netmail to one of our nodes in the setup.
 *  Return a file descriptor if success else NULL.
 *  Later the pack routine will add these mails to the outbound.
 */
FILE *SendMgrMail(faddr *t, int Keep, int FileAttach, char *bymgr, char *subj, char *reply)
{
	FILE		*qp;
	time_t		Now;
	fidoaddr	Orig, Dest;
	faddr		From;
	unsigned	flags = M_PVT;
	char		ext[4];

	From = *bestaka_s(t);
	memset(&Orig, 0, sizeof(Orig));
	Orig.zone  = From.zone;
	Orig.net   = From.net;
	Orig.node  = From.node;
	Orig.point = From.point;
	sprintf(Orig.domain, "%s", From.domain);

	memset(&Dest, 0, sizeof(Dest));
	Dest.zone  = t->zone;
	Dest.net   = t->net;
	Dest.node  = t->node;
	Dest.point = t->point;
	sprintf(Dest.domain, "%s", t->domain);

	if (!SearchNode(Dest)) {
		Syslog('m', "Can't find node %s", aka2str(Dest));
		return NULL;
	}

	Syslog('-', "  Netmail from %s to %s", aka2str(Orig), ascfnode(t, 0x1f));

	Now = time(NULL) - (gmt_offset((time_t)0) * 60);
	flags |= (nodes.Crash)            ? M_CRASH    : 0;
	flags |= (FileAttach)             ? M_FILE     : 0;
	flags |= (!Keep)                  ? M_KILLSENT : 0;
	flags |= (nodes.Hold)             ? M_HOLD     : 0;

	/*
	 *  Increase counters, update record and reload.
	 */
	StatAdd(&nodes.MailSent, 1L);
	UpdateNode();
	SearchNode(Dest);

	memset(&ext, 0, sizeof(ext));
	if (nodes.PackNetmail)
		sprintf(ext, (char *)"qqq");
	else if (nodes.Crash)
		sprintf(ext, (char *)"ccc");
	else if (nodes.Hold)
		sprintf(ext, (char *)"hhh");
	else
		sprintf(ext, (char *)"nnn");

	if ((qp = OpenPkt(Orig, Dest, (char *)ext)) == NULL)
		return NULL;

	if (AddMsgHdr(qp, &From, t, flags, 0, Now, nodes.Sysop, tlcap(bymgr), subj)) {
		fclose(qp);
		return NULL;
	}

	if (Dest.point)
		fprintf(qp, "\001TOPT %d\r", Dest.point);
	if (Orig.point)
		fprintf(qp, "\001FMPT %d\r", Orig.point);

	fprintf(qp, "\001INTL %d:%d/%d %d:%d/%d\r", Dest.zone, Dest.net, Dest.node, Orig.zone, Orig.net, Orig.node);

	/*
	 * Add MSGID, REPLY and PID
	 */
	fprintf(qp, "\001MSGID: %s %08lx\r", aka2str(Orig), sequencer());
	if (reply != NULL)
		fprintf(qp, "\001REPLY: %s\r", reply);
	fprintf(qp, "\001PID: MBSE-FIDO %s\r", VERSION);
	fprintf(qp, "\001TZUTC: %s\r", gmtoffset(Now));
	return qp;
}



void CloseMail(FILE *qp, faddr *t)
{
	time_t		Now;
	struct tm	*tm;

	putc('\r', qp);
	Now = time(NULL);
	tm = gmtime(&Now);
	fprintf(qp, "\001Via %s @%d%02d%02d.%02d%02d%02d.02.UTC %s\r",
		ascfnode(bestaka_s(t), 0x1f), tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, 
		tm->tm_hour, tm->tm_min, tm->tm_sec, VERSION);

	putc(0, qp);
	fclose(qp);
}



