/*
 * minicom.h	Constants, defaults, globals etc.
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1995 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * fmg 1/11/94 colors
 *
 */

/* First include all other application-dependant include files. */
#include "config.h"
#include "configsym.h"
#include "window.h"
#include "keyboard.h"
#include "vt100.h"

#define XA_OK_EXIST	1
#define XA_OK_NOTEXIST	2

#ifndef EXTERN
# define EXTERN extern
#endif

EXTERN char stdattr;	/* Standard attribute */

EXTERN WIN *us;		/* User screen */
EXTERN WIN *st;		/* Status Line */

EXTERN int real_uid;	/* Real uid */
EXTERN int real_gid;	/* Real gid */
EXTERN int eff_uid;	/* Effective uid */
EXTERN int eff_gid;	/* Effective gid */
EXTERN int remote_uid;  /*Uid of remote user*/
EXTERN short terminal;	/* terminal type */
EXTERN time_t online;	/* Time online in minutes */
EXTERN short portfd;	/* File descriptor of the serial port. */
EXTERN short lines;	/* Nr. of lines on the screen */
EXTERN short cols;	/* Nr. of cols of the screen */
EXTERN int keypadmode;	/* Mode of keypad */
EXTERN int cursormode;	/* Mode of cursor (arrow) keys */

EXTERN int docap;	/* Capture data to capture file */
EXTERN FILE *capfp;	/* File to capture to */
EXTERN int addlf;	/* Add LF after CR */
EXTERN int tempst;	/* Status line is temporary */
EXTERN int escape;	/* Escape code. */

EXTERN char lockfile[128]; /* UUCP lock file of terminal */
EXTERN char homedir[256];  /* Home directory of user */
EXTERN char username[16];  /* Who is using remote? */

EXTERN int bogus_dcd;	/* This indicates the dcd status if no 'real' dcd */
EXTERN int alt_override;/* -m option */

EXTERN char parfile[256]; /* Global parameter file */
EXTERN char pparfile[256]; /* Personal parameter file */

EXTERN char scr_name[33];   /* Name of last script */
EXTERN char scr_user[33];   /* Login name to use with script */
EXTERN char scr_passwd[33]; /* Password to use with script */

EXTERN char termtype[32];  /* Value of getenv("TERM"); */
EXTERN char *dial_tty;     /* tty to use. */

EXTERN int portuid, portgid; /* Uid and Gid of /dev/ttyS... */
EXTERN char *dial_name;	    /* System we're conneced to */
EXTERN char *dial_number;   /* Number we've dialed. */

/* fmg 1/11/94 colors */

EXTERN int mfcolor;     /* Menu Foreground Color */
EXTERN int mbcolor;     /* Menu Background Color */
EXTERN int tfcolor;     /* Terminal Foreground Color */
EXTERN int tbcolor;     /* Terminal Background Color */
EXTERN int sfcolor;     /* Status Bar Foreground Color */
EXTERN int sbcolor;     /* Status Bar Background Color */
EXTERN int st_attr;	/* Status Bar attributes. */

/* MARK updated 02/17/95 - history buffer */
EXTERN int num_hist_lines;  /* History buffer size */

/* fmg 1/11/94 colors - convert color word to # */

_PROTO(int Jcolor, ( char * ));

EXTERN int size_changed;    /* Window size has changed */
extern char *Version;	    /* Remote verson */

#if _SELECT
EXTERN int local_echo;      /* Local echo on/off. */
#endif

/* Forward declaration. */
struct dialent;

/* Global functions */

/*      Prototypes from file: " config.c "      */

_PROTO(void read_parms, ( void ));
_PROTO(int waccess, ( char *s ));
_PROTO(int config, ( int setup ));
_PROTO(void get_bbp, ( char *ba , char *bi , char *pa , int curr_ok ));
_PROTO(char *yesno, ( int k ));
_PROTO(int dotermmenu, ( void ));
_PROTO(int dodflsave, ( void ));	/* fmg - need it */
_PROTO(void vdodflsave, ( void ));	/* fmg - need it */
_PROTO(int domacsave, ( void ));	/* fmg - need it */

/*      Prototypes from file: " dial.c "      */

_PROTO(void mputs, ( char *s , int how ));
_PROTO(void modeminit, ( void ));
_PROTO(void modemreset, ( void ));
_PROTO(void hangup, ( void ));
_PROTO(void sendbreak, ( void ));
_PROTO(int dial, ( struct dialent *d ));
_PROTO(int readdialdir, ( void ));
_PROTO(void dialone, ( char *entry ));
_PROTO(void dialdir, ( void ));

/*      Prototypes from file: " fastsystem.c "      */

_PROTO(int fastexec, ( char *cmd ));
_PROTO(int fastsystem, ( char *cmd , char *in , char *out , char *err ));

/*      Prototypes from file: " help.c "      */

_PROTO(int help, ( void ));

/*      Prototypes from file: " ipc.c "      */

_PROTO(int check_io, ( int fd1, int fd2, int tmout, char *buf , int *buflen ));
_PROTO(int keyboard, ( int cmd , int arg ));

/*      Prototypes from file: " keyserv.c "      */

_PROTO(void handler, ( int dummy ));
_PROTO(void sendstr, ( char *s ));
_PROTO(int main, ( int argc , char **argv ));

/*      Prototypes from file: " main.c "      */

_PROTO(char *mbasename, ( char *s ));
_PROTO(void leave, ( char *s ));
_PROTO(char *esc_key, ( void ));
_PROTO(int open_term, ( int doinit ));
_PROTO(void init_emul, ( int type , int do_init ));
_PROTO(void timer_update, (void ));
_PROTO(void mode_status, ( void ));
_PROTO(void time_status, ( void ));
_PROTO(void curs_status, ( void ));
_PROTO(void show_status, ( void ));
_PROTO(void scriptname, ( char *s ));
_PROTO(int do_terminal, ( void ));

/*      Prototypes from file: " remote.c "      */

_PROTO(void port_init, ( ));

/*      Prototypes from file: " rwconf.c "      */

_PROTO(int writepars, ( FILE *fp , int all ));
_PROTO(int writemacs, ( FILE *fp , int all )); /* fmg */
_PROTO(int readpars, ( FILE *fp , int init ));
_PROTO(int readmacs, ( FILE *fp , int init )); /* fmg */

#if 0 /* Only if you don't have it already */
_PROTO(char *strtok, ( char *s , char *delim ));
#endif
#ifdef _SVR2
_PROTO(int dup2, ( int from, int to ));
#endif

/*      Prototypes from file: " sysdep1.c "      */

_PROTO(void m_sethwf, ( int fd , int on ));
_PROTO(void m_dtrtoggle, ( int fd ));
_PROTO(void m_break, ( int fd ));
_PROTO(int m_getdcd, ( int fd ));
_PROTO(void m_setdcd, ( int fd , int what ));
_PROTO(void m_savestate, ( int fd ));
_PROTO(void m_restorestate, ( int fd ));
_PROTO(void m_nohang, ( int fd ));
_PROTO(void m_hupcl, ( int fd, int on ));
_PROTO(void m_flush, ( int fd ));
_PROTO(int m_getmaxspd, ( void ));
_PROTO(void m_setparms, ( int fd , char *baudr , char *par , char *bits, int hwf, int swf ));
_PROTO(int m_wait, ( int *st ));

/*      Prototypes from file: " sysdep2.c "      */

_PROTO(void getrowcols, ( int *rows , int *cols ));
_PROTO(int setcbreak, ( int mode ));
_PROTO(void enab_sig, ( int onoff , int intrchar ));
_PROTO(void drop_all_privs, ( void ));
_PROTO(void drop_privs, ( void ));
_PROTO(void set_privs, ( void ));
_PROTO(FILE *sfopen, ( char *file, char *mode ));

#if 0 /* Only if you don't have it already */
_PROTO(char *strtok, ( char *s , char *delim ));
#endif
#ifdef _SVR2
_PROTO(int dup2, ( int from, int to ));
#endif

/*      Prototypes from file: " updown.c "      */

_PROTO(void updown, ( int what, int nr ));
_PROTO(int mc_setenv, (char *, char *));
_PROTO(void kermit, ( void ));
_PROTO(void runscript, ( int ask, char *s , char *l , char *p ));

/*      Prototypes from file: " windiv.c "      */

/* Should use stdarg et al. */
WIN *tell();
void werror();
_PROTO(int ask, ( char *what , char *s []));
_PROTO(char *input, ( char *s , char *buf ));

/*      Prototypes from file: " wkeys.c "      */

_PROTO(int wxgetch, ( void ));

/*      Prototypes from file: " config.c "      */

_PROTO(void domacros, ( void ));

