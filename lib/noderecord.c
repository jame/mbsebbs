/*****************************************************************************
 *
 * File ..................: noderecord.c
 * Purpose ...............: Load noderecord
 * Last modification date : 11-Mar-2001
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

#include "libs.h"
#include "structs.h"
#include "records.h"
#include "dbnode.h"
#include "common.h"



int noderecord(faddr *addr)
{
	fidoaddr	fa;

	memset(&fa, 0, sizeof(fa));
	fa.zone   = addr->zone;
	fa.net    = addr->net;
	fa.node   = addr->node;
	fa.point  = addr->point;

	if (!(TestNode(fa)))
		if (!SearchNode(fa)) {
			return FALSE;
		}

	return TRUE;
}


