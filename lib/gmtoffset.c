/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Calculate UTC offset 
 * Source ................: Eugene G. Crosser's ifmail package.
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

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "common.h"



/*
 * Returns the offset from your location to UTC. So in the MET timezone
 * this returns -60 (wintertime). People in the USA get positive results.
 */
long gmt_offset(time_t now)
{
	struct tm	ptm;
	struct tm	gtm;
	long		offset;

	if (!now) 
		now = time(NULL);
	ptm = *localtime(&now);

	/* 
	 * To get the timezone, compare localtime with GMT.
	 */
	gtm = *gmtime(&now);

	/* 
	 * Assume we are never more than 24 hours away.
	 */
	offset = gtm.tm_yday - ptm.tm_yday;
	if (offset > 1)
	    offset = -24;
	else if (offset < -1)
	    offset = 24;
	else
	    offset *= 24;

	/* 
	 * Scale in the hours and minutes; ignore seconds.
	 */
	offset += gtm.tm_hour - ptm.tm_hour;
	offset *= 60;
	offset += gtm.tm_min - ptm.tm_min;

	return offset;
}



/*
 * Returns the TZUTC string, note that the sign is opposite from the
 * function above.
 */
char *gmtoffset(time_t now)
{
	static char	buf[6]="+0000";
	char		sign;
	int		hr, min;
	long		offset;

	offset = gmt_offset(now);

	if (offset <= 0) {
		sign = '+';
		offset = -offset;
	} else
		sign = '-';

	hr  = offset / 60L;
	min = offset % 60L;

	if (sign == '-')
		sprintf(buf, "%c%02d%02d", sign, hr, min);
	else
		sprintf(buf, "%02d%02d", hr, min);

	return(buf);
}



char *str_time(time_t total)
{
	static char	buf[10];
	int		h, m;

	memset(&buf, 0, sizeof(buf));

	/*
	 * 0 .. 59 seconds
	 */
	if (total < (time_t)60) {
		sprintf(buf, "%2d.00s", (int)total);
		return buf;
	}

	/*
	 * 1:00 .. 59:59 minutes:seconds
	 */
	if (total < (time_t)3600) {
		h = total / 60;
		m = total % 60;
		sprintf(buf, "%2d:%02d ", h, m);
		return buf;
	}

	/*
	 * 1:00 .. 23:59 hours:minutes
	 */
	if (total < (time_t)86400) {
		h = (total / 60) / 60;
		m = (total / 60) % 60;
		sprintf(buf, "%2d:%02dm", h, m);
		return buf;
	}

	/*
	 * 1/00 .. 30/23 days/hours
	 */
	if (total < (time_t)2592000) {
		h = (total / 3600) / 24;
		m = (total / 3600) % 24;
		sprintf(buf, "%2d/%02dh", h, m);
		return buf;
	}

	sprintf(buf, "N/A   ");
	return buf;
}



char *t_elapsed(time_t start, time_t end)
{
	return str_time(end - start);
}


