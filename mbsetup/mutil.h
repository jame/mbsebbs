#ifndef	_MUTIL_H
#define	_MUTIL_H

/* $Id$ */

unsigned char	readkey(int, int, int, int);
unsigned char	testkey(int, int);
int		newpage(FILE *, int);
void		addtoc(FILE *, FILE *, int, int, int, char *);
void		disk_reset(void);
FILE		*open_webdoc(char *, char *, char *);
void		close_webdoc(FILE *);
void		add_webtable(FILE *, char *, char *);
void		add_webdigit(FILE *, char *, int);

#endif
