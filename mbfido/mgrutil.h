/* $Id$ */

#ifndef	_MGRUTIL_H
#define	_MGRUTIL_H


#define LIST_LIST   0
#define LIST_NOTIFY 1
#define LIST_QUERY  2
#define LIST_UNLINK 3



/*
 * Linked list for atea areas create
 */
typedef struct _AreaList {
	struct _AreaList    *next;
	char		    Name[51];
	int		    IsPresent;
	int		    DoDelete;
} AreaList;


void MacroRead(FILE *, FILE *);
int  MsgResult(const char *, FILE *, char);
void GetRpSubject(const char *, char*);

void WriteMailGroups(FILE *, faddr *);
void WriteFileGroups(FILE *, faddr *);
void CleanBuf(char *);
void ShiftBuf(char *, int);
void MgrPasswd(faddr *, char *, FILE *, int, int);
void MgrNotify(faddr *, char *, FILE *, int);
int  UplinkRequest(faddr *, faddr *, int, char *);
int  Areas(void);


#endif

