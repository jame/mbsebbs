/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Display and handle the menus.
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
#include "oneline.h"
#include "mail.h"
#include "bbslist.h"
#include "change.h"
#include "chat.h"
#include "file.h"
#include "funcs.h"
#include "input.h"
#include "misc.h"
#include "safe.h"
#include "timeout.h"
#include "menu.h"
#include "page.h"
#include "pinfo.h"
#include "bye.h"
#include "timecheck.h"
#include "whoson.h"
#include "language.h"
#include "offline.h"
#include "email.h"
#include "door.h"
#include "dispfile.h"
#include "userlist.h"
#include "timestats.h"
#include "logentry.h"
#include "morefile.h"
#include "lastcallers.h"
#include "signature.h"



/*
 * Menu stack, 50 levels deep.
 */
char	Menus[50][15];
int	MenuLevel;
int	MenuError;


void InitMenu()
{
	int	i;

	for (i = 0; i < 50; i++) 
		memset(Menus[i], 0, 51);
	MenuLevel = 0;
	MenuError = 0;
	sprintf(Menus[0], "%s", CFG.default_menu);
}



void menu()
{
    FILE    *pMenuFile;
    int	    iFoundKey = FALSE, Key, IsANSI;
    char    *Input, *Semfile, *sMenuPathFileName;

    Input = calloc(PATH_MAX, sizeof(char));
    sMenuPathFileName = calloc(PATH_MAX, sizeof(char));

    /* 
     * Loop forever, this is what a BBS should do until a user logs out.
     */
    while (TRUE) {

	WhosDoingWhat(BROWSING);

	/*
	 * Open menufile, first users language menu, if it fails
	 * try to open the default menu.
	 */
	sprintf(sMenuPathFileName,"%s/%s", lang.MenuPath, Menus[MenuLevel]);
	if ((pMenuFile = fopen(sMenuPathFileName, "r")) == NULL) {
	    sprintf(sMenuPathFileName,"%s/%s", CFG.bbs_menus, Menus[MenuLevel]);
	    pMenuFile = fopen(sMenuPathFileName,"r");
	    if (pMenuFile != NULL)
		Syslog('+', "Menu %s (Default)", Menus[MenuLevel]);
	} else {
	    Syslog('+', "Menu %s (%s)", Menus[MenuLevel], lang.Name);
	}

	if (pMenuFile == NULL) {
	    clear();
	    WriteError("Can't open menu file: %s", sMenuPathFileName);
	    MenuError++;

	    /*
	     * Is this the last attempt to open the default menu?
	     */
	    if (MenuError == 10) {
		WriteError("FATAL ERROR: Too many menu errors");
		printf("Too many menu errors, notifying Sysop\n\n");
		sleep(3);
		die(MBERR_CONFIG_ERROR);
	    }

	    /*
	     * Switch back to the default menu
	     */
	    MenuLevel = 0;
	    strcpy(Menus[0], CFG.default_menu);
	} else {
	    /*
	     * Display Menu Text Fields and Perform all autoexec menus in order of menu file.
	     * First check if there are any ANSI menus, if not, send a clearscreen first.
	     */
	    IsANSI = FALSE;
	    while (fread(&menus, sizeof(menus), 1, pMenuFile) == 1) {
		if ( Access(exitinfo.Security, menus.MenuSecurity) && (UserAge >= menus.Age)){
		    if ((menus.MenuType == 5) || (menus.MenuType == 19) || (menus.MenuType == 20))
			IsANSI = TRUE;
		}
	    }
	    fseek(pMenuFile, 0, SEEK_SET);
	    if (! IsANSI)
		clear();

	    while (fread(&menus, sizeof(menus), 1, pMenuFile) == 1) {
		if ( Access(exitinfo.Security, menus.MenuSecurity) && (UserAge >= menus.Age)){
		    if ( menus.AutoExec ) {
			DoMenu( menus.MenuType );
		    }
		    DisplayMenu( ); 
		}
	    }

	    /*
	     * Check if the BBS closed down for Zone Mail Hour or
	     * system shutdown. If so, we run the Goodbye show.
	     */
	    if (CheckStatus() == FALSE) {
		fclose(pMenuFile);
		Syslog('+', "Kicking user out, the BBS is closed.");
		sleep(3);
		Good_Bye(MBERR_OK);
	    }

	    /*
	     * Check the upsdown semafore
	     */
	    Semfile = calloc(PATH_MAX, sizeof(char));
	    sprintf(Semfile, "%s/sema/upsdown", getenv("MBSE_ROOT"));
	    if (file_exist(Semfile, R_OK) == 0) {
		fclose(pMenuFile);
		Syslog('+', "Kicking user out, upsdown semafore detected");
		printf("System power failure, closing the bbs\n\n");
		free(Semfile);
		sleep(3);
		Good_Bye(MBERR_OK);
	    }
	    free(Semfile);

	    /*
	     * Check if SysOp wants to chat to user everytime user
	     * gets prompt. Make sure /tmp/chatdev exists before
	     * before calling chat(). Make sure if a second user
	     * logs in, that .BusyChatting does exist.
	     */
	    if(CFG.iChatPromptChk && (access("/tmp/chatdev", R_OK) == 0) && (access("/tmp/.BusyChatting", F_OK) != 0))
		Chat();

	    /*
	     * Check users timeleft
	     */
	    TimeCheck();

	    alarm_on();

	    if (exitinfo.HotKeys) {
		fflush(stdout);
		Key = Getone();
		sprintf(Input, "%c", Key);
		printf("\n");
	    } else {
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(Input, 80);
	    }

	    if ((strcmp(Input, "")) != 0) {

		fseek(pMenuFile, 0, SEEK_SET);

		while (fread(&menus, sizeof(menus), 1, pMenuFile) == 1) {
		 
		    if ((strcmp(tu(Input), menus.MenuKey)) == 0) {
			if ((Access(exitinfo.Security, menus.MenuSecurity)) && (UserAge >= menus.Age)) {
			    Syslog('b', "Menu[%d] %d=(%s), Opt: '%s'", MenuLevel, menus.MenuType, 
					menus.TypeDesc, menus.OptionalData);
			    if (menus.MenuType == 13) {
				/*
				 *  Terminate call, cleanup here
				 */
				free(Input);
				free(sMenuPathFileName);
				fclose(pMenuFile);
			    }
			    DoMenu(menus.MenuType);
			    iFoundKey = TRUE;
			    break;
			}
		    }
		}
	    }
	    fclose(pMenuFile);

	} /* If menu open */
    } /* while true */
}



void DoMenu(int Type)
{
	int	Strlen, i, x;
	char	*DisplayF;
	char	*sPrompt;
	char	*sPromptBak;
	char	*temp;

	DisplayF   = calloc(81, sizeof(char));
	sPrompt    = calloc(81, sizeof(char));
	sPromptBak = calloc(81, sizeof(char));
	temp       = calloc(81, sizeof(char));

	TimeCheck();

	switch(Type) {
	case 0: /* Display Prompt Line Only */
		break;

	case 1:
		/* Goto another menu */
		strncpy(Menus[MenuLevel], menus.OptionalData, 14);
		break;

	case 2:
		/* Gosub another menu */
		if (MenuLevel < 49) {
			MenuLevel++;
			strncpy(Menus[MenuLevel], menus.OptionalData, 14);
		} else
			Syslog('?', "More than 50 menu levels");
		break;

	case 3:
		/* Return from gosub */
		if (MenuLevel > 0) 
			MenuLevel--;
		break;

	case 4:
		/* Return to top menu */
		MenuLevel = 0;
		break;

	case 5:
		/* Display .a?? file with controlcodes */
		DisplayFile(menus.OptionalData);
		break;

	case 6:
		/* Show menu prompt */
		Strlen = strlen(menus.OptionalData);
		for(x = 0; x < Strlen; x++) {
			if( menus.OptionalData[x] == '~') {
				strcat(sPrompt, sUserTimeleft);
			} else {
			 	sprintf(temp, "%c", menus.OptionalData[x]);
				strcat(sPrompt, temp);
			}
		}
		strcpy(sPromptBak, sPrompt);
		strcpy(sPrompt, "");
		Strlen = strlen(sPromptBak);
		for(x = 0; x < Strlen; x++) {
			if( *(sPromptBak + x) == '@')
				strcat(sPrompt, sAreaDesc);
			else 
				if ( *(sPromptBak + x) == '^')
					strcat(sPrompt, sMsgAreaDesc);
				else 
					if ( *(sPromptBak + x) == '#')
						sprintf(sPrompt, "%s%s", sPrompt, (char *) GetLocalHM()); 

					else  {
						sprintf(temp, "%c", *(sPromptBak + x));
						strcat(sPrompt, temp);
					}
		}
		if (menus.ForeGnd || menus.BackGnd)
		    pout(menus.ForeGnd, menus.BackGnd, sPrompt);
		else
		    pout(15, 0, sPrompt);
		break;

	case 7:
		/* Run external program */
		ExtDoor(menus.OptionalData, menus.NoDoorsys, menus.Y2Kdoorsys, menus.Comport, menus.NoSuid, menus.NoPrompt);
		break;

	case 8:
		/* Show product information */
		cr();
		break;

	case 9:
		/* display todays callers */
		LastCallers(menus.OptionalData);
		break;

	case 10:
		/* display userlist */
		UserList(menus.OptionalData);
		break;

	case 11:
		/* display time statistics */
		TimeStats();
		break;

	case 12:
		/* page sysop for chat */
		Page_Sysop(menus.OptionalData);
		break;

	case 13:
		/* terminate call */
		free(DisplayF);
		free(sPrompt);
		free(sPromptBak);
		free(temp);
		Good_Bye(MBERR_OK);
		break;

	case 14:
		/* make a log entry */
		LogEntry(menus.OptionalData);
		break;

	case 15:
		/* print text to screen */
		if (exitinfo.Security.level >= menus.MenuSecurity.level) {
			for(i = 0; i < strlen(menus.OptionalData); i++)
				if(*(menus.OptionalData + i) == '@')
				 	*(menus.OptionalData + i) = '\n';
			printf("%s\n", menus.OptionalData);
		}
		break;

	case 16:
		/* who's currently online */
		WhosOn(menus.OptionalData);
		Pause();
		break;

	case 17:
		/* comment to sysop */
		SysopComment((char *)"Comment to Sysop");
		break;

	case 18:
		/* send on-line message */
		SendOnlineMsg(menus.OptionalData);
		break;
		
	case 19:
		/* display Textfile with more */
		MoreFile(menus.OptionalData);
		break;

	case 20:
		/* display a?? file with controlcode and wait for enter */
		DisplayFileEnter(menus.OptionalData);
 		break;

	case 21:
		/* display menuline only */
		break;

		/* Menus 22, 23 and 24 were obsolete doors */

	case 25:
		/* safe cracker door */
		Safe();
		break;

	case 101:
		FileArea_List(menus.OptionalData);
		break;

	case 102:
		File_List();
		break;

	case 103:
		ViewFile();
		break;

	case 104:
		Download();
		break;

	case 105:
		File_RawDir(menus.OptionalData);
		break;

	case 106:
		KeywordScan();
		break;

	case 107:
		FilenameScan();
		break;

	case 108:
		NewfileScan(TRUE);
		break;

	case 109:
		Upload();
		break;

	case 110:
		EditTaglist();
		break;

	case 111: /* View file in homedir */
		break;

	case 112:
		DownloadDirect(menus.OptionalData, TRUE);
		break;

	case 113:
		Copy_Home();
		break;

	case 114:
		List_Home();
		break;

	case 115:
		Delete_Home();
		break;

	/* 116 Unpack file in homedir */

	/* 117 Pack files in homedir */

	case 118:
		Download_Home();
		break;

	case 119:
		Upload_Home();
		break;

	case 201:
		MsgArea_List(menus.OptionalData);
		break;

	case 202:
		Post_Msg(); 
		break;

	case 203:
		Read_Msgs();
		break;

	case 204:
		CheckMail();
		break;

	case 205:
		QuickScan_Msgs();
		break;

	case 206:
		Delete_Msg();
		break;

	case 207:
		MailStatus();
		break;

	case 208:
		OLR_TagArea();
		break;

	case 209:
		OLR_UntagArea();
		break;

	case 210:
		OLR_ViewTags();
		break;

	case 211:
		OLR_RestrictDate();
		break;

	case 212:
		OLR_Upload();
		break;

	case 213:
		OLR_DownBW();
		break;

	case 214:
		OLR_DownQWK();
		break;

	case 215:
		OLR_DownASCII();
		break;

	case 216:
		Read_Email();
		break;

	case 217:
		Write_Email();
		break;

	case 218:
		Trash_Email();
		break;

	case 219:
		Choose_Mailbox(menus.OptionalData);
		break;

	case 220:
		QuickScan_Email();
		break;

	case 301:
		Chg_Protocol();
		break;

	case 302:
		Chg_Password();
		break;

	case 303:
		Chg_Location();
		break;

	case 304:
		Chg_Graphics();
		break;

	case 305:
		Chg_VoicePhone();
		break;

	case 306:
		Chg_DataPhone();
		break;

	case 307:
		Chg_News();
		break;

	case 308:
		Chg_ScreenLen();
		break;

	case 309:
		Chg_DOB();
		break;

	case 310:
		Chg_Language(FALSE);
		break;

	case 311:
		Chg_Hotkeys();
		break;

	case 312:
		Chg_Handle();
		break;

	case 313:
		Chg_MailCheck();
		break;

	case 314:
		Chg_Disturb();
		break;

	case 315:
		Chg_FileCheck();
		break;

	case 316:
		Chg_FsMsged();
		break;

	case 317:
		Chg_FsMsgedKeys();
		break;

	case 318:
		Chg_Address();
		break;

	case 319:
		signature();
		break;

	case 401:
		Oneliner_Add();
		break;

	case 402:
		Oneliner_List();
		break;

	case 403:
		Oneliner_Show();
		break;

	case 404:
		Oneliner_Delete();
		break;

	case 405:
		Oneliner_Print();
		break;

	case 501:
		BBS_Add();
		break;

	case 502:
		BBS_List();
		break;

	case 503:
		BBS_Show();
		break;

	case 504:
		BBS_Delete();
		break;

	case 506:
		BBS_Search();
		break;

	default:
		Enter(1);
		pout(15, 0, (char *) Language(339));
		Enter(2);
		Syslog('?', "Option: %s -> Unknown Menu Type: %d on %s", menus.MenuKey, Type, Menus[MenuLevel]); 
		Pause();
	}

	free(DisplayF);
	free(sPrompt);
	free(sPromptBak);
	free(temp);
}

void DisplayMenu ( void ) {
	
	/*
	* Display the Menu Display Text to the User, 
	* if the sysop puts a ';' (semicolon) at the end of the menu prompt,
	* the CR/LR combination will not be sent
	*/

	int maxdpos;
	int dpos;
	int escaped ;
	int skipCRLF ;
	int highlight ;

	/* Anything to process, if not; save CPU time, return */
	if (( strlen( menus.Display ) == 0 ) && (menus.MenuType != 21)) {
		return;
	}

	/* Send Display string, formated with ANSI codes as required */
	/* --- basically this with brains: printf("%s\n", menus.Display); */
	maxdpos = strlen(menus.Display);
	escaped = 0;
	skipCRLF = 0;
	highlight = 0;

	colour( menus.ForeGnd, menus.BackGnd );

	for ( dpos = 0; dpos < maxdpos ; dpos++ ){

		switch ( menus.Display[ dpos ] ) {

			case ';':	/* Semicolon, if not escaped and last char, not CRLF at end of line */
					if ( ( dpos + 1 ) == maxdpos ) {
						skipCRLF=1;
					} else {
						printf("%c",menus.Display[ dpos ]);
					}
					break;

			case '\\':	if ( !escaped ) {
						escaped = 1;
					} else {
						escaped = 0;
						printf("%c",menus.Display[ dpos ]);
					}
					break;

			case '^':	/* Highlight Toggle */

					if ( !escaped ) {
						if ( highlight == 0 ) { 
							highlight = 1;
							colour( menus.HiForeGnd, menus.HiBackGnd);
						} else {					
							highlight = 0 ;
							colour( menus.ForeGnd, menus.BackGnd );
						}
					} else {
						escaped=0;
						printf("%c",menus.Display[ dpos ]);
					}
					break;

			default:	/* all other characters */
					printf("%c",menus.Display[ dpos ]);
		}

	}

	if ( !skipCRLF ) {
		printf("\n");
	}

}
