/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Menu Utils
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/diesel.h"
#include "screen.h" 
#include "mutil.h"



unsigned char readkey(int y, int x, int fg, int bg)
{
	int		rc = -1, i;
	unsigned char 	ch = 0;

	working(0, 0, 0);
	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 9");
		exit(MBERR_TTYIO_ERROR);
	}
	Setraw();

	i = 0;
	while (rc == -1) {
		if ((i % 10) == 0)
			show_date(fg, bg, 0, 0);

		locate(y, x);
		fflush(stdout);
		rc = Waitchar(&ch, 5);
		if ((rc == 1) && (ch != KEY_ESCAPE))
			break;

		if ((rc == 1) && (ch == KEY_ESCAPE))
			rc = Escapechar(&ch);

		if (rc == 1)
			break;
		i++;
		Nopper();
	}

	Unsetraw();
	close(ttyfd);

	return ch;
}



unsigned char testkey(int y, int x)
{
	int		rc;
	unsigned char	ch = 0;

	locate(y, x);
	fflush(stdout);

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 9");
		exit(MBERR_TTYIO_ERROR);
	}
	Setraw();

	rc = Waitchar(&ch, 100);
	if (rc == 1) {
		if (ch == KEY_ESCAPE)
			rc = Escapechar(&ch);
	}

	Unsetraw();
	close(ttyfd);

	if (rc == 1)
		return ch;
	else
		return '\0';
}



int newpage(FILE *fp, int page)
{
	page++;
	fprintf(fp, "\f   MBSE BBS v%-53s   page %d\n", VERSION, page);
	return page;
}



void addtoc(FILE *fp, FILE *toc, int chapt, int par, int page, char *title)
{
	char	temp[81];
	char	*tit;

	sprintf(temp, "%s ", title);
	tit = xstrcpy(title);
	tu(tit);

	if (par) {
		fprintf(toc, "     %2d.%-3d   %s %d\n", chapt, par, padleft(temp, 50, '.'), page);
		fprintf(fp, "\n\n   %d.%d. %s\n\n", chapt, par, tit);
	} else {
		fprintf(toc, "     %2d     %s %d\n", chapt, padleft(temp, 52, '.'), page);
		fprintf(fp, "\n\n   %d. %s\n", chapt, tit);
	}
	free(tit);
}



void disk_reset(void)
{
    SockS("DRES:0;");
}



FILE *open_webdoc(char *filename, char *title, char *title2)
{
    char    *p, *temp;
    FILE    *fp;
    time_t  now;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/doc/html/%s", getenv("MBSE_ROOT"), filename);
    mkdirs(temp, 0755);

    if ((fp = fopen(temp, "w+")) == NULL) {
	WriteError("$Can't create %s", temp);
    } else {
	fprintf(fp, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(fp, "<HTML>\n");
	fprintf(fp, "<HEAD>\n");
	fprintf(fp, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\">\n");
	fprintf(fp, "<META NAME=\"Language\" content=\"en\">\n");
	if (title2)
	    fprintf(fp, "<TITLE>%s: %s</TITLE>\n", title, title2);
	else
	    fprintf(fp, "<TITLE>%s</TITLE>\n", title);
	fprintf(fp, "<STYLE type=\"text/css\">\n");
	fprintf(fp, "BODY      { font: 12pt sans-serif,helvetica,arial; }\n");
	fprintf(fp, "H1        { color: red; font: 16pt sans-serif,helvetica,arial;  font-weight: bold }\n");
	fprintf(fp, "H2        { color: red; font: 14pt sans-serif,helvetica,arial;  font-weight: bold }\n");
	fprintf(fp, "H3        { position: relative; left: 60px;  }\n");
	fprintf(fp, "H5        { color: black; font: 10pt sans-serif,helvetica,arial; }\n");
	fprintf(fp, "A:link    { color: blue }\n");
	fprintf(fp, "A:visited { color: blue }\n");
	fprintf(fp, "A:active  { color: red  }\n");
	fprintf(fp, "TH        { font-weight: bold; }\n");
	fprintf(fp, "PRE       { font-size: 10pt; color: blue; font-family: fixed; }\n");
	fprintf(fp, "</STYLE>\n");
	fprintf(fp, "</HEAD>\n");
	fprintf(fp, "<BODY bgcolor=\"#DDDDAA\" link=\"blue\" alink=\"red\" vlink=\"blue\">\n");
	fprintf(fp, "<A NAME=\"_top\"></A>\n");
	fprintf(fp, "<BLOCKQUOTE>\n");
	if (title2)
	    fprintf(fp, "<DIV Align=\"center\"><H1>%s: %s</H1></DIV>\n", title, title2);
	else
	    fprintf(fp, "<DIV Align=\"center\"><H1>%s</H1></DIV>\n", title);
	now = time(NULL);
	p = ctime(&now);
	Striplf(p);
	fprintf(fp, "<DIV align=\"right\"><H5>Created %s</H5></DIV>\n", p);
    }

    free(temp);
    return fp;
}


void close_webdoc(FILE *fp)
{
    fprintf(fp, "</BLOCKQUOTE>\n");
    fprintf(fp, "</BODY>\n");
    fprintf(fp, "</HTML>\n");
    fclose(fp);
}


void add_webtable(FILE *fp, char *hstr, char *dstr)
{
    char    left[1024], right[1024];

    html_massage(hstr, left);
    if (strlen(dstr))
	html_massage(dstr, right);
    else
	sprintf(right, "&nbsp;");
    fprintf(fp, "<TR><TH align='left'>%s</TH><TD>%s</TD></TR>\n", left, right);
}



void add_webdigit(FILE *fp, char *hstr, int digit)
{
    char    left[1024];

    html_massage(hstr, left);
    fprintf(fp, "<TR><TH align='left'>%s</TH><TD>%d</TD></TR>\n", left, digit);
}

