/*
 * config.c	Read and write the configuration file(s).
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1995 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * // fmg 12/20/93 - Added color selection to Screen & Keyboard menu
 * // fmg 2/15/94 - Added macro filename & Macro define selection to
 *                  Screen & Keyboard menu. Added window for macro
 *                  definition.
 */
#include "port.h"
#include "remote.h"

#if _HAVE_MACROS
/* Prefix a non-absolute file with the home directory. */
static char *pfix_home(s)
char *s;
{
  static char buf[256];

  if (s && *s != '/') {
	sprintf(buf, "%s/%s", homedir, s);
	return(buf);
  }
  return(s);
}
#endif

/* Read in parameters. */
void read_parms()
{
  FILE *fp;
  int f;
  char buf[64];
  char *p;

  /* Read global parameters */
  if ((fp = fopen(parfile, "r")) == (FILE *)NULL) {
  	if (real_uid == remote_uid ) {
  		fprintf(stderr,
  	"remote: WARNING: configuration file not found, using defaults\n");
  		sleep(2);
  		return;
  	}
  	fprintf(stderr,
	"remote: there is no global configuration file %s\n", parfile);
  	fprintf(stderr, "Ask your sysadm to create one (with remote -s).\n");
  	exit(1);
  }
  readpars(fp, 1);
  fclose(fp);
  /* Read personal parameters */
  if ((fp = sfopen(pparfile, "r")) != (FILE *)NULL) {
	readpars(fp, 0);
	fclose(fp);
  }

  /* fmg - set colors from readin values (Jcolor Xlates name to #) */
  mfcolor = Jcolor(P_MFG); mbcolor = Jcolor(P_MBG);
  tfcolor = Jcolor(P_TFG); tbcolor = Jcolor(P_TBG);
  sfcolor = Jcolor(P_SFG); sbcolor = Jcolor(P_SBG);
 
#if _HAVE_MACROS
  /* fmg - Read personal macros */
  if (P_MACROS[0] != 0) { /* fmg - null length? */
	if ((fp = sfopen(pfix_home(P_MACROS), "r")) == NULL) {
		if (errno != ENOENT) {
                	fprintf(stderr,
                "remote: cannot open macro file %s\n", pfix_home(P_MACROS));
                	sleep(1); /* fmg - give the "slow" ones time to read :-) */
		}
        } else {
                readmacs(fp, 0);
                fclose(fp);
        }
  } /* fmg - but it's perfectly OK if macros file name is NULL... */
#endif

  /* This code is to use old configuration files. */
  for(f = PROTO_BASE; f < MAXPROTO; f++) {
	if (P_PNAME(f)[0] && P_PIORED(f) != 'Y' && P_PIORED(f) != 'N') {
		strcpy(buf, P_PNAME(f) - 2);
		strcpy(P_PNAME(f), buf);
		P_PIORED(f) = 'Y';
		P_PFULL(f) = 'N';
	}
  }
  p = mbasename(P_LOCK);
  if (strncmp(p, "LCK", 3) == 0) *p = 0;
}

/*
 * fmg - Convert color word to number
 */
int Jcolor(s)
char *s;
{
        char c1, c3;

        c1 = toupper(s[0]); /* fmg - it's already up but why tempt it? */
        c3 = toupper(s[2]);

        switch (c1)
        {
                case 'G'        : return (GREEN);
                case 'Y'        : return (YELLOW);
                case 'W'        : return (WHITE);
                case 'R'        : return (RED);
                case 'M'        : return (MAGENTA);
                case 'C'        : return (CYAN);
                case 'B'        : if (c3 == 'A')
                                        return (BLACK);
                                  if (c3 == 'U')
                                        return (BLUE);
                                  else
                                        break;
        }
        return (-1); /* fmg - should never get here */
}
 
/*
 * See if we have write access to a file.
 * If it is not there, see if the directory is writable.
 */
int waccess(s)
char *s;
{
  char *p;
  char buf[128];
  struct stat stt;

  /* We use stat instead of access(s, F_OK) because I couldn't get
   * that to work under BSD 4.3 ...
   */
  if (stat(s, &stt) == 0) {
	if (access(s, W_OK) == 0)
		return(XA_OK_EXIST);
	return(-1);
  }
  strcpy(buf, s);
  if((p = strrchr(buf, '/')) == (char *)NULL)
  	strcpy(buf, ".");
  else
  	*p = '\0';
  if (access(buf, W_OK) == 0)
	return(XA_OK_NOTEXIST);
  return(-1);
}

#if _HAVE_MACROS
/*
 * fmg - Read in a macro, but first check to see if it's
 * allowed to do so.
 *
 * TODO: have System macros and user macros (in theory it's already there
 * since user can specify their own macros file (unless root makes it
 * private... that's silly) ... anyways, you know what I mean...)
 */
static void mgets(w, x, y, s, len, maxl)
WIN *w;
int x, y;
char *s;
int len;
int maxl;
{
  struct macs *m = (struct macs *)s;

  if ((m->flags & PRIVATE) && real_uid != remote_uid ) {
        werror("You are not allowed to change this parameter");
        return;
  }
  wlocate(w, x, y);
  (void) wgets(w, s, len, maxl);
  m->flags |= CHANGED;
}
#endif

/*
 * Read in a string, but first check to see if it's
 * allowed to do so.
 */
static void pgets(w, x, y, s, len, maxl)
WIN *w;
int x, y;
char *s;
int len;
int maxl;
{
  struct pars *p = (struct pars *)s;

  if ((p->flags & PRIVATE) && real_uid != remote_uid ) {
  	werror("You are not allowed to change this parameter");
  	return;
  }
  wlocate(w, x, y);
  (void) wgets(w, s, len, maxl);
  p->flags |= CHANGED;
}

/*
 * Mark a variable as changed.
 */
static void markch(s)
char *s;
{
  struct pars *p = (struct pars *)s;

  p->flags |= CHANGED;
}

/*
 * Set a string to a given value, but only if we're allowed to.
 */
static void psets(s, w)
char *s, *w;
{
  struct pars *p = (struct pars *)s;

  if ((p->flags & PRIVATE) && real_uid != remote_uid ) {
  	werror("You are not allowed to change this parameter");
  	return;
  }
  strcpy(s, w);
  p->flags |= CHANGED;
}

/*
 * Get a a character from the keyboard. Translate lower
 * to uppercase and '\r' to '\n'.
 */
static int rwxgetch()
{
  int c;

  c = wxgetch();
  if (islower(c)) c = toupper(c);
  if (c == '\n' || c == '\r' || c == '\033') return('\n');
  return(c);
}

static void dopath()
{
  WIN *w;
  int c;
  
  w = wopen(5, 5, 75, 11, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wprintf(w, " A - Download directory : %.44s\n", P_DOWNDIR);
  wprintf(w, " B -   Upload directory : %.44s\n", P_UPDIR);
  wprintf(w, " C -   Script directory : %.44s\n", P_SCRIPTDIR);
  wprintf(w, " D -     Script program : %.44s\n", P_SCRIPTPROG);
  wprintf(w, " E -     Kermit program : %.44s\n", P_KERMIT);
  wlocate(w, 4, 7);
  wputs(w, "Change which setting? ");

  wredraw(w, 1);

  while(1) {
      wlocate(w, 26, 7);
      c = rwxgetch();
      switch(c) {
  	case '\n':
  		wclose(w, 1);
  		return;
  	case 'A':
  		pgets(w, 26, 0, P_DOWNDIR, 64, 64);
  		break;
  	case 'B':
  		pgets(w, 26, 1, P_UPDIR, 64, 64);
  		break;
  	case 'C':
  		pgets(w, 26, 2, P_SCRIPTDIR, 64, 64);
  		break;
  	case 'D':
  		pgets(w, 26, 3, P_SCRIPTPROG, 64, 64);
  		break;
  	case 'E':
  		pgets(w, 26, 4, P_KERMIT, 64, 64);
  		break;
  	default:
  		break;
      }
  }
}

char *yesno(k)
int k;
{
  return(k ? "Yes" : "No ");
}

/*
 * Input the definition of an up/download protocol.
 */
static void inputproto(w, n)
WIN *w;
int n;
{
  int c = 0;

  mpars[PROTO_BASE + n].flags |= CHANGED;

  if (P_PNAME(n)[0] == '\0') {
  	P_PNN(n) = 'Y';
  	P_PUD(n) = 'U';
	P_PFULL(n) = 'N';
	P_PPROG(n)[0] = 0;
	P_PIORED(n) = 'Y';
  	wlocate(w, 4, n+1);
  	wputs(w, "       ");
  }
  wlocate(w, 4, n+1);
  (void ) wgets(w, P_PNAME(n), 10, 64);
  pgets(w, 15, n+1, P_PPROG(n), 31, 64);
  do {
	wlocate(w, 48, n+1);
	wprintf(w, "%c", P_PNN(n));
	c = rwxgetch();
	if (c == 'Y') P_PNN(n) = 'Y';
	if (c == 'N') P_PNN(n) = 'N';
  } while(c != '\r' && c != '\n');
  do {
	wlocate(w, 56, n+1);
	wprintf(w, "%c", P_PUD(n));
	c = rwxgetch();
	if (c == 'U') P_PUD(n) = 'U';
	if (c == 'D') P_PUD(n) = 'D';
  } while(c != '\r' && c != '\n');
  do {
	wlocate(w, 64, n+1);
	wprintf(w, "%c", P_PFULL(n));
	c = rwxgetch();
	if (c == 'Y') P_PFULL(n) = 'Y';
	if (c == 'N') P_PFULL(n) = 'N';
  } while(c != '\r' && c != '\n');
  do {
	wlocate(w, 72, n+1);
	wprintf(w, "%c", P_PIORED(n));
	c = rwxgetch();
	if (c == 'Y') P_PIORED(n) = 'Y';
	if (c == 'N') P_PIORED(n) = 'N';
  } while(c != '\r' && c != '\n');
}

static void doproto()
{
  WIN *w;
  int f, c;

  w = wopen(1, 4, 78, 19, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wputs(w, "     Name             Program");
  wlocate(w, 44, 0);
  wputs(w, "Need name Up/Down FullScr IO-Red.");
  for(f = 0; f < 12; f++) {
     wlocate(w, 1, f+1);
     if (P_PNAME(f)[0])
  	wprintf(w, "%c  %-10.10s %-31.31s  %c       %c       %c       %c",
		'A' + f,
  		P_PNAME(f), P_PPROG(f),
  		P_PNN(f), P_PUD(f),
		P_PFULL(f), P_PIORED(f));
     else
        wprintf(w, "%c    -", 'A' + f);
  }
  wlocate(w, 1, 13);
  wprintf(w, "M  Zmodem download string activates... %c", P_PAUTO[0]);

  wlocate(w, 3, 15);
  wputs(w, "Change which setting? (SPACE to delete) ");
  wredraw(w, 1);

  do {
  	wlocate(w, 43, 15);
  	c = rwxgetch();
  	if (c >= 'A' && c <= 'L') inputproto(w, c - 'A');
  	if (c == ' ') {
  		wlocate(w, 3, 15);
  		wputs(w, "Delete which protocol? ");
  		wclreol(w);
  		c = rwxgetch();
  		if (c >= 'A' && c <= 'L') {
  			P_PNAME(c - 'A')[0] = '\0';
  			mpars[PROTO_BASE + (c - 'A')].flags |= CHANGED;
  			wlocate(w, 3, c - 'A' + 1);
  			wclreol(w);
  			wputs(w, "   -");
  		}
		wlocate(w, 3, 15);
		wputs(w, "Change which setting? (SPACE to delete) ");
		c = ' ';
	}
	if (c == 'M') {
		wlocate(w, 40, 13);
		wprintf(w, " \b");
		c = rwxgetch();
		if (c >= 'A' && c <= 'L') {
			P_PAUTO[0] = c;
			markch(P_PAUTO);
			wprintf(w, "%c", c);
		} else if (c == '\n' || c == ' ') {
			P_PAUTO[0] = ' ';
			markch(P_PAUTO);
		} else {
			wprintf(w, "%c", P_PAUTO[0]);
		}
		c = 0;
	}
  } while(c != '\n');
  wclose(w, 1);
}

static void doserial()
{
  WIN *w;

  w = wopen(5, 4, 75, 12, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wprintf(w, " A -    Serial Device      : %.41s\n", P_PORT);
  wprintf(w, " B - Lockfile Location     : %.41s\n", P_LOCK);
  wprintf(w, " C -   Callin Program      : %.41s\n", P_CALLIN);
  wprintf(w, " D -  Callout Program      : %.41s\n", P_CALLOUT);
  wprintf(w, " E -    Baud/Par/Bits      : %s %s%s1\n", P_BAUDRATE,
  	P_BITS, P_PARITY);
  wprintf(w, " F - Hardware Flow Control : %s\n", P_HASRTS);
  wprintf(w, " G - Software Flow Control : %s\n", P_HASXON);
  wlocate(w, 4, 8);
  wputs(w, "Change which setting? ");
  wredraw(w, 1);

  while(1) {
      wlocate(w, 26, 8);
      switch(rwxgetch()) {
  	case '\n':
  		wclose(w, 1);
  		return;
  	case 'A':
  		pgets(w, 29, 0, P_PORT, 64, 64);
  		break;
  	case 'B':
  		pgets(w, 29, 1, P_LOCK, 64, 64);
  		break;
  	case 'C':
  		pgets(w, 29, 2, P_CALLIN, 64, 64);
  		break;
  	case 'D':
  		pgets(w, 29, 3, P_CALLOUT, 64, 64);
  		break;
  	case 'E':
  		get_bbp(P_BAUDRATE, P_BITS, P_PARITY, 0);
  		if (portfd >= 0) port_init();
  		wlocate(w, 29, 4);
		wprintf(w, "%s %s%s1  \n", P_BAUDRATE, P_BITS, P_PARITY);
		if (st != NIL_WIN) mode_status();
		markch(P_BAUDRATE);
		markch(P_BITS);
		markch(P_PARITY);
		break;
	case 'F':
		strcpy(P_HASRTS, yesno(P_HASRTS[0] == 'N'));
		wlocate(w, 29, 5);
		wprintf(w, "%s ", P_HASRTS);
  		if (portfd >= 0) port_init();
		markch(P_HASRTS);
		break;
	case 'G':
		strcpy(P_HASXON, yesno(P_HASXON[0] == 'N'));
		wlocate(w, 29, 6);
		wprintf(w, "%s ", P_HASXON);
  		if (portfd >= 0) port_init();
		markch(P_HASXON);
		break;
  	default:
  		break;
      }
  }
}

/*
 * Screen and keyboard menu.
 */
static void doscrkeyb()
{
  WIN *w, *w1;
  int c;
  int once = 0;
  int clr = 1;
  int tmp_c;    /* fmg - need it to color keep in sanity checks */
  char buf[16];
  int miny = 4, 
  maxy = 17;
  int old_stat = P_STATLINE[0];
#if _HAVE_MACROS
  FILE	*fp;

  miny = 3;
  maxy = 19;
#endif

  w = wopen(15, miny, 69, maxy, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);

  wtitle(w, TMID, "Screen and keyboard");

  wprintf(w, "\n A - Command key is         : %s\n", P_ESCAPE);
  wprintf(w, " B - Backspace key sends    : %s\n", P_BACKSPACE);
  wprintf(w, " C - Status line is         : %s\n", P_STATLINE);
  wprintf(w, " D - Alarm sound            : %s\n", P_SOUND);
  /* fmg - colors support */
  wprintf(w, " E - Foreground Color (menu): %s\n", J_col[mfcolor]);
  wprintf(w, " F - Background Color (menu): %s\n", J_col[mbcolor]);
  wprintf(w, " G - Foreground Color (term): %s\n", J_col[tfcolor]);
  wprintf(w, " H - Background Color (term): %s\n", J_col[tbcolor]);
  wprintf(w, " I - Foreground Color (stat): %s\n", J_col[sfcolor]);
  wprintf(w, " J - Background Color (stat): %s\n", J_col[sbcolor]);
  
  /* MARK updated 02/17/95 - Configurable history buffer size */
  wprintf(w, " K - History Buffer Size    : %s\n", P_HISTSIZE);

#if _HAVE_MACROS
  /* fmg - macros support */
  wprintf(w, " L - Macros file            : %s\n", P_MACROS);
  wprintf(w, " M - Edit Macros\n");
  wprintf(w, " N - Macros enabled         : %s\n", P_MACENAB);
#endif

  wredraw(w, 1);

  while(1) {
  	if (clr) {
  		wlocate(w, 2, maxy - miny);
		wputs(w, "Change which setting?  (Esc to exit) ");
		wclreol(w);
		clr = 0;
	} else
  		wlocate(w, 39, maxy - miny);

  	if (once) {	/* fmg - allow to force looping */
  		c = once;
  		once = 0;
	} else c = rwxgetch();
#if 0 /* This might save us someday */
	if (!usecolor && (c >= 'E' && c <= 'J')) {
		werror("You can't change colors in black and white mode");
		continue;
	}
#endif
  	switch(c) {
  		case '\n':
                 /* fmg - sanity checks... "we found the enemy and he is us" :-) */

                 if (mfcolor == mbcolor)   /* oops... */
                 {
                    tmp_c=mfcolor;      /* save color (same for both, right?) */
                    mfcolor=WHITE;      /* make sure they can see error :-) */
                    mbcolor=BLACK;
                    werror("Menu foreground == background color, change!");
                    mfcolor=tmp_c;      /* restore colors */
                    mbcolor=tmp_c;
                    break;
                 }
                 if (tfcolor == tbcolor)   /* oops... */
                 {
                    tmp_c=mfcolor;      /* save color (same for both, right?) */
                    mfcolor=WHITE;      /* make sure they can see error :-) */
                    mbcolor=BLACK;
                    werror("Terminal foreground == background color, change!");
                    mfcolor=tmp_c;      /* restore colors */
                    mbcolor=tmp_c;
                    break;
                 }
                 /* fmg - I'll let them change sfcolor=sbcolor because it's just
                          another way of turning "off" the status line... */

			/* MARK updated 02/17/95, Warn user to restart */
			/* remote if they changed history buffer size */
			if (atoi(P_HISTSIZE) != num_hist_lines) {
				w1 = wopen(14, 9, 70, 15, BSINGLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
				wtitle(w1, TMID, "History Buffer Size");
				wputs(w1, "\n\
  You have changed the history buffer size.\n\
  You will need to save the configuration file and\n\
  restart remote for the change to take effect.\n\n\
  Hit a key to Continue... ");
				wredraw(w1, 1);
				c = wxgetch();
				wclose(w1, 1);
			}

  			wclose(w, 1);
			/* If status line enabled/disabled resize screen. */
			if (P_STATLINE[0] != old_stat)
				init_emul(terminal, 0);
  			return;
  		case 'A':
  			w1 = wopen(11, 8, 73, 17, BSINGLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
			wtitle(w1, TMID, "Program new command key");
			wputs(w1, "\n Press the new command key. If you want to use\n");
			wputs(w1, " the META or ALT key enter:\n\n");
			wputs(w1, "  o SPACE if your meta key sets the 8th bit high\n");
			wputs(w1, "  o ESC   if your meta key sends the ESCAPE prefix (standard)\n");
			wputs(w1, "\n\n Press new command key: ");
			wredraw(w1, 1);
			c = wxgetch();
			wclose(w1, 1);
  			if (c  == ' ')
  				strcpy(buf, "Meta-8th bit ");
  			else if (c == 27)
				strcpy(buf, "Escape (Meta)");
			else
  				sprintf(buf, "^%c          ", (c & 0x1f) + 'A' - 1);
  			psets(P_ESCAPE, buf);
  			wlocate(w, 30, 1);
  			wputs(w, buf);
  			clr = 1;
  			alt_override = 0;
			switch(P_ESCAPE[0]) {
				case '^':
					c = P_ESCAPE[1] & 31;
					break;
				case 'E':
					c = 27;
					break;
				default:
					c = 128;
					break;
			}
			keyboard(KSETESC, c);
  			if (st) show_status();
  			break;
  		case 'B':
  			if (P_BACKSPACE[0] == 'D')
  				psets(P_BACKSPACE, "BS");
  			else
  				psets(P_BACKSPACE, "DEL");
  			wlocate(w, 30, 2);
  			wprintf(w, "%s ", P_BACKSPACE);
			keyboard(KSETBS, P_BACKSPACE[0] == 'B' ? 8 : 127);
  			break;
  		case 'C':
  			if (P_STATLINE[0] == 'e') {
  				psets(P_STATLINE, "disabled");
  				tempst = 1;
  			} else {
  				psets(P_STATLINE, "enabled");
  				/* See if it fits on screen */
  				if (LINES > MIN_LINES) tempst = 0;
  			}
  			wlocate(w, 30, 3);
  			wprintf(w, "%s ", P_STATLINE);
  			break;
		case 'D':
			psets(P_SOUND, yesno(P_SOUND[0] == 'N'));
			wlocate(w, 30, 4);
			wprintf(w, "%s", P_SOUND);
			break;
                case 'E': /* fmg - letters cycle colors */
                        if (mfcolor == WHITE)
                                mfcolor = BLACK;
                        else
                                mfcolor++;
                        psets(P_MFG, J_col[mfcolor]);
                        wlocate(w, 30, 5);
                        wprintf(w, "%s   ", J_col[mfcolor]);
                        break;
                case 'F': /* fmg - letters cycle colors */
                        if (mbcolor == WHITE)
                                mbcolor = BLACK;
                        else
                                mbcolor++;
                        psets(P_MBG, J_col[mbcolor]);
                        wlocate(w, 30, 6);
                        wprintf(w, "%s   ", J_col[mbcolor]);
                        break;
                case 'G': /* fmg - letters cycle colors */
                        if (tfcolor == WHITE)
                                tfcolor = BLACK;
                        else
                                tfcolor++;
                        psets(P_TFG, J_col[tfcolor]);
                        wlocate(w, 30, 7);
                        wprintf(w, "%s   ", J_col[tfcolor]);
			if (us) vt_pinit(us, tfcolor, tbcolor);
                        break;
                case 'H': /* fmg - letters cycle colors */
                        if (tbcolor == WHITE)
                                tbcolor = BLACK;
                        else
                                tbcolor++;
                        psets(P_TBG, J_col[tbcolor]);
                        wlocate(w, 30, 8);
                        wprintf(w, "%s   ", J_col[tbcolor]);
			if (us) vt_pinit(us, tfcolor, tbcolor);
                        break;
                case 'I': /* fmg - letters cycle colors & redraw stat line */
                        if (sfcolor == WHITE)
                                sfcolor = BLACK;
                        else
                                sfcolor++;

                        /* fmg - this causes redraw of status line (if any)
                                 in current color */

                        if (st)
                        {
                                wclose(st,0);
                                st = wopen(0, LINES - 1, COLS - 1, LINES - 1, BNONE,
                                         XA_NORMAL, sfcolor, sbcolor, 1, 0, 1);
                                show_status();
                        }
                        psets(P_SFG, J_col[sfcolor]);
                        wlocate(w, 30, 9);
                        wprintf(w, "%s   ", J_col[sfcolor]);
                        break;
                case 'J': /* fmg - letters cycle colors & redraw stat line */
                        if (sbcolor == WHITE)
                                sbcolor = BLACK;
                        else
                                sbcolor++;

                        /* fmg - this causes redraw of status line (if any)
                                 in current color */

                        if (st)
                        {
                                wclose(st,0);
                                st = wopen(0, LINES - 1, COLS - 1, LINES - 1, BNONE,
                                         XA_NORMAL, sfcolor, sbcolor, 1, 0, 0);
                                show_status();
                        }
                        psets(P_SBG, J_col[sbcolor]);
                        wlocate(w, 30, 10);
                        wprintf(w, "%s   ", J_col[sbcolor]);
                        break;
		case 'K': /* MARK updated 02/17/95 - Config history size */
#if HISTORY
                        pgets(w, 30, 11, P_HISTSIZE, 5, 5);
                        
                        /* In case gibberish or a value was out of bounds, */
                        /* limit history buffer size between 0 to 5000 lines */
                        /* 5000 line history at 80 columns consumes about */
                        /* 800 kilobytes including chars and attrs bytes! */
                        if (atoi(P_HISTSIZE) <= 0) 
                        	strcpy(P_HISTSIZE,"0");
                        else if (atoi(P_HISTSIZE) >= 5000)
                        	strcpy(P_HISTSIZE,"5000");
	                        
                        wlocate(w, 30, 11);
                        wprintf(w, "%s     ", P_HISTSIZE);
#else
			werror("This system does not support history");
#endif
			break;
#if _HAVE_MACROS
                case 'L': /* fmg - get local macros storage file */
                        pgets(w, 30, 12, P_MACROS, 64, 64);

			/* Try to open the file to read it in. */
			fp = sfopen(pfix_home(P_MACROS), "r+");
			if (fp == NULL) {
			    if (errno == EPERM) {
				/* Permission denied, hacker! */
				werror("ERROR: you do not have permission to create a file there!");
				once = 'J'; /* fmg - re-enter it! */
				continue;
			    }
			    if (errno != ENOENT) {
				/* File does exist, but cannot be opened. */
				werror("ERROR: cannot open macro file %s",
					pfix_home(P_MACROS));
			    }
			    continue;
			}
			/* Read macros from the file. */
			werror("Reading macros");
			readmacs(fp, 0);
			fclose(fp);
                        break;
                case 'M': /* fmg - Bring up macro editing window */
                        domacros();
                        break;
		case 'N':
			psets(P_MACENAB, yesno(P_MACENAB[0] == 'N'));
			wlocate(w, 30, 14);
			wprintf(w, "%s", P_MACENAB);
			break;
#endif
  	}
  }
}

/*
 * This is the 'T' menu - terminal parameters. Does NOT set the new
 * terminal type, but returns it to the calling functions that has
 * to call init_emul itself.
 */
int dotermmenu()
{
  WIN *w;
  int c;
  int new_term = -1;
  int old_stat = P_STATLINE[0];
  extern int use_status;

  w = wopen(20, 7, 59, 13, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wtitle(w, TMID, "Terminal settings");
  wprintf(w, "\n");
  wprintf(w, " A - Terminal emulation  : %s\n", terminal == VT100 ? "VT102" : "ANSI");
  wprintf(w, " B - Backspace key sends : %s\n", P_BACKSPACE);
  wprintf(w, " C -      Status line is : %s\n", P_STATLINE);
  wlocate(w, 4, 5);
  wputs(w, "Change which setting? ");

  wredraw(w, 1);

  while(1) {
      wlocate(w, 26, 5);
      c = rwxgetch();
      switch(c) {
  	case '\n':
  		wclose(w, 1);
		/* If status line enabled/disabled resize screen. */
		if (P_STATLINE[0] != old_stat && new_term < 0)
			init_emul(terminal, 0);
  		return(new_term);
  	case 'A':
		if (new_term < 0) new_term = terminal;
		if (new_term == VT100) {
			new_term = ANSI;
  			psets(P_BACKSPACE, "BS");
		} else {
			new_term = VT100;
  			psets(P_BACKSPACE, "DEL");
		}
		wlocate(w, 27, 1);
		wprintf(w, "%s ", new_term == VT100 ? "VT102" : "ANSI");
  		wlocate(w, 27, 2);
  		wprintf(w, "%s ", P_BACKSPACE);
		keyboard(KSETBS, P_BACKSPACE[0] == 'B' ? 8 : 127);
  		break;
  	case 'B':
  		if (P_BACKSPACE[0] == 'D')
  			psets(P_BACKSPACE, "BS");
  		else
  			psets(P_BACKSPACE, "DEL");
  		wlocate(w, 27, 2);
  		wprintf(w, "%s ", P_BACKSPACE);
		keyboard(KSETBS, P_BACKSPACE[0] == 'B' ? 8 : 127);
  		break;
  	case 'C':
		if (P_STATLINE[0] == 'e') {
			psets(P_STATLINE, "disabled");
			tempst = 1;
		} else {
			psets(P_STATLINE, "enabled");
			/* See if it fits on screen */
			if (LINES > MIN_LINES || use_status)
				tempst = 0;
		}
		wlocate(w, 27, 3);
		wprintf(w, "%s ", P_STATLINE);
		break;
  	default:
  		break;
      }
  }
}


/*
 * Save the configuration.
 */
void vdodflsave()
{
	dodflsave();
}

/*
 * Save the configuration.
 */
int dodflsave()
{
  FILE *fp;

  /* User remote saves new configuration */
  if (real_uid == remote_uid ) {
  	if ((fp = fopen(parfile, "w")) == (FILE *)NULL) {
  		werror("Cannot write to %s", parfile);
  		return(-1);
  	}
  	writepars(fp, 1);
	fclose(fp);
	werror("Global configuration saved");
  } else {
	/* Mortals save their own configuration */
	if ((fp = sfopen(pparfile, "w")) == (FILE *)NULL) {
  		werror("Cannot write to %s", pparfile);
  		return (-1);
	}
	writepars(fp, 0);
	fclose(fp);
	werror("Configuration saved");
  }
#if _HAVE_MACROS
  if (domacsave() < 0) /* fmg - something went wrong... */
	return(-1);
#endif
  return(0);
}

#if _HAVE_MACROS
/*
 * Save the macros. (fmg)
 */
int domacsave()
{
  FILE *fp;

  /* fmg - do some basic silly-mortal checks and allow for recovery */
  if (!strcmp(P_MACCHG,"CHANGED")) {
        if (strlen(P_MACROS) == 0) { /* fmg - they might want to know... */
                werror("ERROR: Macros have changed but no filename is set!");
                return(-1);
        } else {
		if ((fp = sfopen(pfix_home(P_MACROS), "w")) == (FILE *)NULL) {
			werror("Cannot write macros file %s",
				pfix_home(P_MACROS));
                        return(-1);
                }
		writemacs(fp, 0);
		fclose(fp);
		werror("Macros saved");
		strcpy(P_MACCHG,"SAVED"); /* fmg - reset after save */
		return(0);
	}
  }
  return(0);
}
#endif
 
/*
 * Save the configuration, ask a name for it.
 */
static void donamsave()
{
  char ifile[128];
  char *s;

  if (real_uid != remote_uid ) {
  	werror("You are not allowed to create a configuration");
	return;
  }

  ifile[0] = 0;
  s = input("Give name to save this configuration?", ifile);
  if (s != (char *)0 && *s != 0) {
  	sprintf(parfile, "%s/remoterc.%s", LIBDIR, s);
  dodflsave();
  }
}

static void (*funcs1[])() = {
  dopath,
  doproto,
  doserial,
  doscrkeyb,
  vdodflsave,
  donamsave,
  NIL_FUN,
  NIL_FUN
};

char some_string[32];

static char *menu1[] = {
  "Filenames and paths",
  "File transfer protocols",
  "Serial port setup",
  "Screen and keyboard",
   some_string,
  "Save setup as..",
  "Exit",
  MENU_END
};


int config(setup)
int setup;
{
  int c;

  /* Find out extension of parameter file */
  sprintf(some_string, "Save setup as %s default", 
      (real_uid == remote_uid ? "GLOBAL" : "your"));

  if (!setup) menu1[8] = MENU_END;

  c = wselect(13, 10, menu1, funcs1, "configuration", stdattr, mfcolor, mbcolor);
  if (c == 9) return(1);
  return(0);
}

/* fmg 1/11/94 Color names for menu */

static char *J_col[] =
  { "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN", "WHITE", };

static char *speeds[] =
   { "300", "1200", "2400", "9600", "19200", "38400", "57600", "115200", "Curr" };

/*
 * Ask user for Baudrate, Bits and Parity
 */
void get_bbp(ba, bi, pa, curr_ok)
char *ba;
char *bi;
char *pa;
int curr_ok;
{
  int c;
  WIN *w;
  int x, y;
  int max = m_getmaxspd();

  w = wopen(21, 4, 60, 20, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wtitle(w, TMID, "Comm Parameters");

  dirflush = 0;

  wlocate(w, 0, 3);
  wputs(w, "   Speed          Parity          Data\n\n");
  wputs(w, " A: 300           J: None         Q: 5\n");
  wputs(w, " B: 1200          K: Even         R: 6\n");
  wputs(w, " C: 2400          L: Odd          S: 7\n");
  wputs(w, " D: 9600          M: Mark         T: 8\n");
  if (max > 96)
	wputs(w, " E: 19200         N: Space\n");
  else
	wputs(w, "                  N: Space\n");
  if (max > 192)
	wputs(w, " F: 38400\n");
  else
	wputs(w, "\n");
  if (max > 384)
	wputs(w, " G: 57600\n");
  else
	wputs(w, "\n");
  if (max > 576)
	wputs(w, " H: 115200        O: 8-N-1\n");
  else
	wputs(w, "                  O: 8-N-1\n");
  if (curr_ok)
	wputs(w, " I: Current       P: 7-E-1\n");
  else
	wputs(w, "                  P: 7-E-1\n");
  wputs(w, "\n Choice, or <Enter> to exit? ");
  x = w->curx;
  y = w->cury;

  bi[1] = 0;
  pa[1] = 0;

  wredraw(w, 1);

  while(1) {
  	wlocate(w, 1, 1);
  	wprintf(w, "Current: %5s %s%s1  ", ba, bi, pa);
  	wlocate(w, x, y);
  	wflush();
  	c = wxgetch();
  	if (c >= 'a') c -= 32;
  	switch(c) {
  		case 'H':
			if (max < 1152) break;
  		case 'G':
			if (max < 576) break;
		case 'F':
			if (max < 384) break;
  		case 'E':
			if (max < 192) break;
  		case 'A':
  		case 'B':
  		case 'C':
  		case 'D':
		case 'I':
			if (c == 'I' && !curr_ok) break;
  			strcpy(ba, speeds[c - 'A']);
  			break;
  		case 'J':
  			pa[0] = 'N';
  			break;
  		case 'K':
  			pa[0] = 'E';
  			break;
  		case 'L':
  			pa[0] = 'O';
  			break;
		case 'M':
			pa[0] = 'M';
			break;
		case 'N':
			pa[0] = 'S';
			break;
  		case 'O':
  			pa[0] = 'N';
  			bi[0] = '8';
  			break;
  		case 'P':
  			pa[0] = 'E';
  			bi[0] = '7';
  			break;
  		case 'Q':
  			bi[0] = '5';
  			break;
  		case 'R':
  			bi[0] = '6';
  			break;
  		case 'S':
  			bi[0] = '7';
  			break;
  		case 'T':
  			bi[0] = '8';
  			break;
  		case 27:
  		case '\n':
  		case '\r':
  			dirflush = 1;
  			wclose(w, 1);
  			return;
  		default:
  			break;
  	}
  }
}

#if _HAVE_MACROS
/*
 * fmg - part of the Macros menu, "[none]" beats (null) :-)
 */
static void out_mac(w, s, n)
WIN *w;
char *s;
char n;
{
  wprintf(w, " %c : %.67s\n", n, s ? s : "[none]");
}

/*
 * fmg - Macros editing window
 */
void domacros()
{
  WIN   *w;
  int   clr = 1;
  int   Jch='1', Jm=0; /* fmg - ok, so I was lazy.. */

  w = wopen(3, 7, 75, 21 , BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wtitle(w, TMID, "F1 to F10 Macros");

  wprintf(w, "\n");
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  out_mac(w,mmacs[Jm++].value, Jch++);
  Jch = 'A'; /* fmg - ran out of single digits... */
  out_mac(w,mmacs[Jm++].value, Jch++);

  wredraw(w, 1);

  while(1) {
        wlocate(w, 1, 14);
        wputs(w, " (LEGEND: ^M = C-M, ^L = C-L, ^G = C-G, ^R = C-R, ^~ = pause 1 second)");
        if (clr) {
                wlocate(w, 1, 12);
                wputs(w, "Change which setting?  (Esc to exit) ");
                wclreol(w);
                clr = 0;
        } else wlocate(w, 38, 12);

        switch(rwxgetch()) {
                case '\n': wclose(w, 1); return;
                case '1':
                        mgets(w, 5, 1, P_MAC1, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ...I'm ashamed */
                        wlocate(w, 0, 1);
                        out_mac(w,P_MAC1, '1');
                        break;
                case '2':
                        mgets(w, 5, 2, P_MAC2, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... really, I am */
                        wlocate(w, 0, 2);
                        out_mac(w,P_MAC2, '2');
                        break;
                case '3':
                        mgets(w, 5, 3, P_MAC3, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... this is the */
                        wlocate(w, 0, 3);
                        out_mac(w,P_MAC3, '3');
                        break;
                case '4':
                        mgets(w, 5, 4, P_MAC4, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... first entry on */
                        wlocate(w, 0, 4);
                        out_mac(w,P_MAC4, '4');
                        break;
                case '5':
                        mgets(w, 5, 5, P_MAC5, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... my TODO list. */
                        wlocate(w, 0, 5);
                        out_mac(w,P_MAC5, '5');
                        break;
                case '6':
                        mgets(w, 5, 6, P_MAC6, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... and, come to think */
                        wlocate(w, 0, 6);
                        out_mac(w,P_MAC6, '6');
                        break;
                case '7':
                        mgets(w, 5, 7, P_MAC7, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... of it, I no longer */
                        wlocate(w, 0, 7);
                        out_mac(w,P_MAC7, '7');
                        break;
                case '8':
                        mgets(w, 5, 8, P_MAC8, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... even use this... */
                        wlocate(w, 0, 8);
                        out_mac(w,P_MAC8, '8');
                        break;
                case '9':
                        mgets(w, 5, 9, P_MAC9, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... [sigh] */
                        wlocate(w, 0, 9);
                        out_mac(w,P_MAC9, '9');
                        break;
                case 'A':
                        mgets(w, 5, 10, P_MAC10, 72, MAC_LEN);
                        strcpy(P_MACCHG,"CHANGED"); /* fmg - ... [sigh] */
                        wlocate(w, 0, 10);
                        out_mac(w,P_MAC10, 'A');
                        break;
        }
  }
}
#endif
