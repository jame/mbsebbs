/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Scan for virusses
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "virscan.h"


/*
 * Check for known viri, optional in a defined path.
 */
int VirScan(char *path)
{
    char    *pwd, *temp, *cmd = NULL;
    FILE    *fp;
    int	    rc = FALSE, has_scan = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/virscan.data", getenv("MBSE_ROOT"));

    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("No virus scanners defined");
	free(temp);
	return FALSE;
    }
    fread(&virscanhdr, sizeof(virscanhdr), 1, fp);

    while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
	if (virscan.available)
	    has_scan = TRUE;
    }
    if (!has_scan) {
	Syslog('+', "No active virus scanners, skipping scan");
	fclose(fp);
	free(temp);
	return FALSE;
    }
    
    pwd = calloc(PATH_MAX, sizeof(char));
    getcwd(pwd, PATH_MAX);
    if (path) {
	chdir(path);
	Syslog('+', "Start virusscan in %s", path);
    } else {
	Syslog('+', "Start virusscan in %s", pwd);
    }
    
    fseek(fp, virscanhdr.hdrsize, SEEK_SET);
    while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
        cmd = NULL;
        if (virscan.available) {
	    Altime(3600);
            cmd = xstrcpy(virscan.scanner);
            cmd = xstrcat(cmd, (char *)" ");
            cmd = xstrcat(cmd, virscan.options);
            if (execute(cmd, (char *)"*", (char *)NULL, (char *)"/dev/null", 
			     (char *)"/dev/null" , (char *)"/dev/null") != virscan.error) {
                Syslog('!', "Virus found by %s", virscan.comment);
		rc = TRUE;
            }
	    free(cmd);
	    Altime(0);
	    Nopper();
        }
    }
    fclose(fp);

    if (path)
	chdir(pwd);

    free(pwd);
    free(temp);
    return rc;
}


