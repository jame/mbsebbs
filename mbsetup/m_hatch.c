/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Hatch Setup
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_fgroup.h"
#include "m_ticarea.h"
#include "m_hatch.h"


int	HatchUpdated = 0;
char	*Days[]  = {(char *)"Sun",(char *)"Mon",(char *)"Tue",(char *)"Wed", 
		    (char *)"Thu",(char *)"Fri",(char *)"Sat"};
char	*Month[] = {(char *)"1", (char *)"2", (char *)"3", (char *)"4",  
		    (char *)"5", (char *)"6", (char *)"7", (char *)"8",  
		    (char *)"9", (char *)"10",(char *)"11",(char *)"12", 
		    (char *)"13",(char *)"14",(char *)"15",(char *)"16", 
		    (char *)"17",(char *)"18",(char *)"19",(char *)"20",
		    (char *)"21",(char *)"22",(char *)"23",(char *)"24", 
		    (char *)"25",(char *)"26",(char *)"27",(char *)"28", 
		    (char *)"29",(char *)"30",(char *)"31",(char *)"Last"};

/*
 * Count nr of hatch records in the database.
 * Creates the database if it doesn't exist.
 */
int CountHatch(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/hatch.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			hatchhdr.hdrsize = sizeof(hatchhdr);
			hatchhdr.recsize = sizeof(hatch);
			hatchhdr.lastupd = time(NULL);
			fwrite(&hatchhdr, sizeof(hatchhdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 0;
		} else
			return -1;
	}

	fread(&hatchhdr, sizeof(hatchhdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - hatchhdr.hdrsize) / hatchhdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenHatch(void);
int OpenHatch(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;
	int	FieldPatch = FALSE;

	sprintf(fnin,  "%s/etc/hatch.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/hatch.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&hatchhdr, sizeof(hatchhdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize    = hatchhdr.recsize;
			if (oldsize != sizeof(hatch)) {
				HatchUpdated = 1;
				Syslog('+', "Updated %s, format changed", fnin);
				if ((oldsize + 8) == sizeof(hatch)) {
					FieldPatch = TRUE;
					Syslog('?', "Hatch: performing FieldPatch");
				}
			} else
				HatchUpdated = 0;
			hatchhdr.hdrsize = sizeof(hatchhdr);
			hatchhdr.recsize = sizeof(hatch);
			fwrite(&hatchhdr, sizeof(hatchhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&hatch, 0, sizeof(hatch));
			while (fread(&hatch, oldsize, 1, fin) == 1) {
				if (FieldPatch) {
					memmove(&hatch.Replace, &hatch.Name[13], oldsize-12);
					memset(&hatch.Name[13], 0, 8);
				}
				fwrite(&hatch, sizeof(hatch), 1, fout);
				memset(&hatch, 0, sizeof(hatch));
			}
			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseHatch(int);
void CloseHatch(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*hat = NULL, *tmp;

	sprintf(fin, "%s/etc/hatch.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/hatch.temp", getenv("MBSE_ROOT"));

	if (HatchUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&hatchhdr, hatchhdr.hdrsize, 1, fi);
			fwrite(&hatchhdr, hatchhdr.hdrsize, 1, fo);

			while (fread(&hatch, hatchhdr.recsize, 1, fi) == 1)
				if (!hatch.Deleted)
					fill_stlist(&hat, hatch.Spec, ftell(fi) - hatchhdr.recsize);
			sort_stlist(&hat);

			for (tmp = hat; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&hatch, hatchhdr.recsize, 1, fi);
				fwrite(&hatch, hatchhdr.recsize, 1, fo);
			}

			tidy_stlist(&hat);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"hatch.data\"");
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendHatch(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	i;

	sprintf(ffile, "%s/etc/hatch.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&hatch, 0, sizeof(hatch));
		/*
		 * Fill in default values
		 */
		hatch.DupeCheck = TRUE;
		for (i = 0; i < 7; i++)
			hatch.Days[i] = TRUE;
		fwrite(&hatch, sizeof(hatch), 1, fil);
		fclose(fil);
		HatchUpdated = 1;
		return 0;
	} else
		return -1;
}



void HatchScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "10.3 EDIT HATCH MANAGER");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Mask");
	mvprintw( 8, 2, "2.  Area");
	mvprintw( 9, 2, "3.  Replace");
	mvprintw(10, 2, "4.  Magic");
	mvprintw(11, 2, "5.  Desc");
	mvprintw(12, 2, "6.  Dupe");
	mvprintw(13, 2, "7.  Active");
	mvprintw(14, 2, "8.  Deleted");
	mvprintw(15, 2, "9.  Days");
	mvprintw(16, 2, "10. Month");
}



void EditDates(void);
void EditDates(void)
{
	int	i, x, y;

	clr_index();
	for (;;) {
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "10.3.9 EDIT DATES IN MONTH");
		set_color(CYAN, BLACK);
		y = 7;
		x = 5;
		for (i = 0; i < 32; i++) {
			mvprintw(y, x, (char *)"%2d.  %s", i+1, Month[i]);
			y++;
			if (y == 17) {
				y = 7;
				x += 20;
			}
		}
		set_color(WHITE, BLACK);
		y = 7;
		x = 15;
		for (i = 0; i < 32; i++) {
			show_bool(y,x, hatch.Month[i]);
			y++;
			if (y == 17) {
				y = 7;
				x += 20;
			}
		}

		i = select_menu(32);
		if (i == 0)
			return;
		if (i < 11) {
			y = 6 + i;
			x = 15;
		} else if (i < 21) {
				y = i - 4;
				x = 35;
			} else if (i < 31) {
					y = i - 14;
					x = 55;
				} else {
					y = i - 24;
					x = 75;
				}
		if (i == 32) 
			hatch.Month[i-1] = edit_bool(y, x, hatch.Month[i-1], (char *)"Hatch file in the ^last^ day of the month");
		else
			hatch.Month[i-1] = edit_bool(y, x, hatch.Month[i-1], (char *)"Hatch file on this date");
	}
}



void EditDays(void);
void EditDays(void)
{
	int	i;

	clr_index();
	for (;;) {
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "10.3.8 EDIT DAYS IN WEEK");
		set_color(CYAN, BLACK);
		for (i = 0; i < 7; i++)
			mvprintw(7+i, 6, (char *)"%d.  %s", i+1, Days[i]);
		set_color(WHITE, BLACK);
		for (i = 0; i < 7; i++)
			show_bool(7+i,14, hatch.Days[i]);

		i = select_menu(7);
		if (i == 0)
			return;
		hatch.Days[i-1] = edit_bool(6+i, 14, hatch.Days[i-1], (char *)"Hatch file on this day");
	}
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditHatchRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX];
	static char	*tmp = NULL;
	long		offset;
	unsigned long	crc, crc1;
	int		i, All;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Hatch");

	sprintf(mfile, "%s/etc/hatch.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	fread(&hatchhdr, sizeof(hatchhdr), 1, fil);
	offset = hatchhdr.hdrsize + ((Area -1) * hatchhdr.recsize);
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&hatch, hatchhdr.recsize, 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&hatch, crc, hatchhdr.recsize);
	working(0, 0, 0);

	for (;;) {
		HatchScreen();
		set_color(WHITE, BLACK);
		show_str(  7,18,55, hatch.Spec);
		show_str(  8,18,20, hatch.Name);
		show_str(  9,18,14, hatch.Replace);
		show_str( 10,18,14, hatch.Magic);
		show_str( 11,18,55, hatch.Desc);
		show_bool(12,18,    hatch.DupeCheck);
		show_bool(13,18,    hatch.Active);
		show_bool(14,18,    hatch.Deleted);

		for (i = 0; i < 7; i++)
			if (hatch.Days[i]) {
				if (tmp == NULL) {
					tmp = xstrcpy(Days[i]);
				} else {
					tmp = xstrcat(tmp, (char *)", ");
					tmp = xstrcat(tmp, Days[i]);
				}
			}
		if (tmp == NULL)
			tmp = xstrcpy((char *)"None");
		show_str( 15,18,55, tmp);
		if (tmp != NULL) {
			free(tmp);
			tmp = NULL;
		}
		All = TRUE;
		for (i = 0; i < 32; i++)
			if (!hatch.Month[i])
				All = FALSE;
		if (!All) {
			for (i = 0; i < 32; i++)
				if (hatch.Month[i]) {
					if (tmp == NULL) {
						tmp = xstrcpy(Month[i]);
					} else {
						tmp = xstrcat(tmp, (char *)", ");
						tmp = xstrcat(tmp, Month[i]);
					}
				}
		} else
			tmp = xstrcpy((char *)"All dates");
		if (tmp == NULL)
			tmp = xstrcpy((char *)"None");
		show_str( 16,18,55, tmp);
		if (tmp != NULL) {
			free(tmp);
			tmp = NULL;
		}

		switch(select_menu(10)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&hatch, crc1, hatchhdr.recsize);
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&hatch, hatchhdr.recsize, 1, fil);
					fclose(fil);
					HatchUpdated = 1;
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;

		case 1:	E_STR(  7,18,55, hatch.Spec,   "Hatch ^path/filespec^. ^?^ is any char, '@' is any alpha, '#' is any number")
		case 2: strcpy(hatch.Name, PickTicarea((char *)"10.3.2"));
			break;
		case 3: E_UPS(  9,18,14, hatch.Replace,   "The ^filename^ to replace by this file")
		case 4: E_UPS( 10,18,14, hatch.Magic,     "The ^magic^ filename for this file")
		case 5: E_STR( 11,18,55, hatch.Desc,      "The ^description^ for this file")
		case 6: E_BOOL(12,18,    hatch.DupeCheck, "Check if this files is a ^duplicate^ hatch")
		case 7: E_BOOL(13,18,    hatch.Active,    "If this file is ^active^")
		case 8: E_BOOL(14,18,    hatch.Deleted,   "If this record is ^Deleted^")
		case 9: EditDays();
			break;
		case 10:EditDates();
			break;
		}
	}
}



void EditHatch(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountHatch();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenHatch() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "10.3. HATCH MANAGER");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/hatch.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&hatchhdr, sizeof(hatchhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(hatchhdr) + (((o + i) - 1) * hatchhdr.recsize);
						fseek(fil, offset, 0);
						fread(&hatch, hatchhdr.recsize, 1, fil);
						if (hatch.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, hatch.Spec);
						temp[37] = 0;
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseHatch(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendHatch() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < records) 
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditHatchRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitHatch(void)
{
    CountHatch();
    OpenHatch();
    CloseHatch(TRUE);
}



int tic_hatch_doc(FILE *fp, FILE *toc, int page)
{
	char		temp[PATH_MAX], *tmp = NULL;
	FILE		*no;
	int		i, j, All;

	sprintf(temp, "%s/etc/hatch.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	page = newpage(fp, page);
	addtoc(fp, toc, 10, 3, page, (char *)"Hatch manager");
	j = 0;

	fprintf(fp, "\n\n");
	fread(&hatchhdr, sizeof(hatchhdr), 1, no);

	while ((fread(&hatch, hatchhdr.recsize, 1, no)) == 1) {

		if (j == 5) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
		}

		fprintf(fp, "     File spec         %s\n", hatch.Spec);
		fprintf(fp, "     File echo         %s\n", hatch.Name);
		fprintf(fp, "     Replace file      %s\n", hatch.Replace);
		fprintf(fp, "     Magic filename    %s\n", hatch.Magic);
		fprintf(fp, "     Description       %s\n", hatch.Desc);
		fprintf(fp, "     Dupe check        %s\n", getboolean(hatch.DupeCheck));
		fprintf(fp, "     Active            %s\n", getboolean(hatch.Active));
		tmp = NULL;
		for (i = 0; i < 7; i++)
			if (hatch.Days[i]) {
				if (tmp == NULL) {
					tmp = xstrcpy(Days[i]);
				} else {
					tmp = xstrcat(tmp, (char *)", ");
					tmp = xstrcat(tmp, Days[i]);
				}
			}
		if (tmp == NULL)
			tmp = xstrcpy((char *)"None");
		fprintf(fp, "     Hatch on days     %s\n", tmp);
		if (tmp != NULL) {
			free(tmp);
			tmp = NULL;
		}
		All = TRUE;
		for (i = 0; i < 32; i++)
			if (!hatch.Month[i])
				All = FALSE;
		if (!All) {
			for (i = 0; i < 32; i++)
				if (hatch.Month[i]) {
					if (tmp == NULL) {
						tmp = xstrcpy(Month[i]);
					} else {
						tmp = xstrcat(tmp, (char *)", ");
						tmp = xstrcat(tmp, Month[i]);
					}
				}
		} else
			tmp = xstrcpy((char *)"All dates");
		if (tmp == NULL)
			tmp = xstrcpy((char *)"None");
		fprintf(fp, "     Hatch on dates    %s\n", tmp);
		if (tmp != NULL) {
			free(tmp);
			tmp = NULL;
		}
		fprintf(fp, "\n\n");
		j++;
	}

	fclose(no);
	return page;
}


