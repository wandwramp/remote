/*
 * dial.c	Functions to dial, retry etc. Als contains the dialing
 *		directory code, _and_ the famous tu-di-di music.
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1995 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#include "port.h"
#include "remote.h"

#if VC_MUSIC
#  if defined(__linux__)
#    include <sys/ioctl.h>
#    include <sys/kd.h>
#    include <sys/time.h>
#  endif
#  if defined(_COH42) || defined(_SCO)
#    include <sys/vtkd.h>
#  endif
#endif

/* We want the ANSI offsetof macro to do some dirty stuff. */
#ifndef offsetof
#  define offsetof(type, member) ((int) &((type *)0)->member)
#endif

/* Values for the "flags". */
#define FL_ECHO		0x01	/* Local echo on/off. */
#define FL_DEL		0x02	/* Backspace or DEL */
#define FL_WRAP		0x04	/* Use autowrap. */
#define FL_ANSI		0x08	/* Type of term emulation */
#define FL_TAG		0x80	/* This entry is tagged. */
#define FL_SAVE		0x0f	/* Which portions of flags to save. */

/* Dialing directory. */
struct v1_dialent {
  char name[32];
  char number[16];
  char script[16];
  char username[32];
  char password[32];
  char term;
  char baud[8];
  char parity[2];
  char dialtype;
  char flags; /* Localecho in v0 */
  char bits[2];
  struct dialent *next;
};

struct dialent {
  char name[32];
  char number[32];
  char script[32];
  char username[32];
  char password[32];
  char term;
  char baud[8];
  char parity[2];
  char dialtype;
  char flags;
  char bits[2];
  struct dialent *next;
};

/* Version info. */
#define DIALMAGIC 0x55AA
struct dver {
  short magic;
  short version;
  short size;
  short res1;
  short res2;
  short res3;
  short res4;
};

#define dialentno(di, no) ((struct dialent *)((char *)(di) + ((no) * sizeof(struct dialent))))  

static struct dialent *dialents;
static struct dialent *d_man;
static int nrents = 1;
static int newtype;
/* Access to ".dialdir" denied? */
static int dendd = 0;
static char *tagged;

/*
 * Functions to talk to the modem.
 */
 
/*
 * Send a string to the modem.
 * If how == 0, '~'  sleeps 1 second.
 * If how == 1, "^~" sleeps 1 second.
 */
void mputs(s, how)
char *s;
int how;
{
  char c;

  while(*s) {
  	if (*s == '^' && (*(s + 1))) {
  		s++;
  		if (*s == '^')
  			c = *s;
  		else if (how == 1 && *s == '~') {
			sleep(1);
			s++;
			continue;
		} else
  			c = (*s) & 31;
  	} else
  		c = *s;
	if (how == 0 && c == '~')
  		sleep(1);
  	else	
  		write(portfd, &c, 1);
  	s++;
  }
}
  
/*
 * Initialize the modem.
 */ 
void modeminit()
{
  WIN *w;

  if (P_MINIT[0] == '\0') return;

  w = tell("Initializing Modem");
  m_dtrtoggle(portfd);
  mputs(P_MINIT, 0);
  wclose(w, 1);
}

/*
 * Reset the modem.
 */
void modemreset()
{
  WIN *w;

  if (P_MRESET[0] == '\0') return;

  w = tell("Resetting Modem");
  mputs(P_MRESET, 0);
  sleep(1);
  wclose(w, 1);
}

/*
 * Hang the line up.
 */
void hangup()
{
  WIN *w;

  w = tell("Hanging up");

  if (P_MDROPDTR[0] == 'Y') {
  	m_dtrtoggle(portfd);
  } else {
  	mputs(P_MHANGUP, 0);
  	sleep(1);
  }
  /* If we don't have DCD support fake DCD dropped */
  bogus_dcd = 0;
  wclose(w, 1);
}

/*
 * This seemed to fit best in this file
 * Send a break signal.
 */
void sendbreak()
{
  WIN *w;
  
  w = tell("Sending BREAK");
  wcursor(w, CNONE);

  m_break(portfd);
  wclose(w, 1);
}
  
WIN *dialwin;
int dialtime;

#if VC_MUSIC

/*
 * Play music until key is pressed.
 */
void music()
{
  int x, i, k;
  int consolefd = 0;

  /* If we're in X, we have to explicitly use the console */
  if (strncmp(getenv("TERM"), "xterm", 5) == 0 &&
	(strcmp(getenv("DISPLAY"), ":0.0") == 0 ||
	(strcmp(getenv("DISPLAY"), ":0") == 0))) {
	consolefd = open("/dev/console", O_WRONLY);
	if (consolefd < 0) consolefd = 0;
  }

  /* Tell keyboard handler what we want. */
  keyboard(KSIGIO, 0);

  /* And loop forever :-) */
  for(i = 0; i < 9; i++) {
	k = 2000 - 200 * (i % 3);
	(void) ioctl(consolefd, KIOCSOUND, k);

	/* Check keypress with timeout 160 ms */
	x = check_io(-1, 0, 160, NULL, NULL);
	if (x & 2) break;
  }
  (void) ioctl(consolefd, KIOCSOUND, 0);
  if (consolefd) close(consolefd);

  /* Wait for keypress and absorb it */
  while((x & 2) == 0) x = check_io(-1, 0, 10000, NULL, NULL);
  (void) keyboard(KGETKEY, 0);
}
#endif

/*
 * The dial has failed. Tell user.
 * Count down until retrytime and return.
 */
static int dialfailed(s, rtime)
char *s;
int rtime;
{
  int f, x;
  int ret = 0;

  wlocate(dialwin, 1, 5);
  wprintf(dialwin, "    No connection: %s.      \n", s);
  if (rtime < 0) {
  	wprintf(dialwin, "   Press any key to continue..    ");
	if (check_io(-1, 0, 10000, NULL, NULL) & 2) 
		(void) keyboard(KGETKEY, 0);
  	return(0);
  }
  wprintf(dialwin, "     Retry in %2d seconds             ", rtime);
  
  for(f = rtime - 1; f >= 0; f--) {
	x = check_io(-1, 0, 1000, NULL, NULL);
	if (x & 2) {
		/* Key pressed - absorb it. */
		x = keyboard(KGETKEY, 0);
		if (x != ' ') ret = -1;
		break;
	}
  	wlocate(dialwin, 14, 6);
  	wprintf(dialwin, "%2d ", f);
  }
#ifdef __linux__
  /* MARK updated 02/17/94 - Min dial delay set to 0.35 sec instead of 1 sec */
  if (f < 0) usleep(350000); /* Allow modem time to hangup if redial time == 0 */
#else
  if (f < 0) sleep(1);
#endif
  wlocate(dialwin, 1, 5);
  wprintf(dialwin, "                              \n");
  wprintf(dialwin, "                        ");
  return(ret);
}

/*
 * Dial a number, and display the name.
 */
int dial(d)
struct dialent *d;
{
  char *s = 0, *t = 0;
  int f, x = 0, nb, retst = -1;
  int modidx, retries = 0;
  int maxretries = 1, rdelay = 45;
  char *reason = "Max retries";
  time_t now, last;
  char buf[128];
  char modbuf[128];

  dialwin = wopen(18, 9, 62, 15, BSINGLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wtitle(dialwin, TMID, "Autodial");
  wcursor(dialwin, CNONE);

  wputs(dialwin, "\n");
  wprintf(dialwin, " Dialing : %s\n", d->name);
  wprintf(dialwin, "      At : %s\n", d->number);
  wredraw(dialwin, 1);

  /* Tell keyboard routines we need them. */
  keyboard(KSIGIO, 0);

  maxretries = atoi(P_MRETRIES);
  if (maxretries <= 0) maxretries = 1;
  rdelay = atoi(P_MRDELAY);
  if (rdelay < 0) rdelay = 0;

  /* Main retry loop of dial() */
MainLoop:
  while(++retries <= maxretries) {

	/* See if we need to try the next tagged entry. */
	if (retries > 1 && (d->flags & FL_TAG)) {
		do {
			d = d->next;
			if (d == (struct dialent *)NULL) d = dialents;
		} while(!(d->flags & FL_TAG));
		wlocate(dialwin, 0, 1);
		wprintf(dialwin, " Dialing : %s", d->name);
		wclreol(dialwin);
		wprintf(dialwin, "\n      At : %s", d->number);
		wclreol(dialwin);
	}

	/* Calculate dial time */
	dialtime = atoi(P_MDIALTIME);
	if (dialtime == 0) dialtime = 45;
	time(&now);
	last = now;

  	/* Show used time */
	wlocate(dialwin, 0, 3);
	wprintf(dialwin, "    Time : %-3d", dialtime);
	if (maxretries > 1) wprintf(dialwin, "     Attempt #%d", retries);
	wputs(dialwin, "\n\n\n Escape to cancel, space to retry.");
	
	/* Start the dial */
	m_flush(portfd);
	switch(d->dialtype) {
		case 0:
			mputs(P_MDIALPRE, 0);
			mputs(d->number, 0);
			mputs(P_MDIALSUF, 0);
			break;
		case 1:
			mputs(P_MDIALPRE2, 0);
			mputs(d->number, 0);
			mputs(P_MDIALSUF2, 0);
			break;
		case 2:
			mputs(P_MDIALPRE3, 0);
			mputs(d->number, 0);
			mputs(P_MDIALSUF3, 0);
			break;
	}

	/* Wait 'till the modem says something */
	modbuf[0] = 0;
	modidx = 0;
	s = buf;
	buf[0] = 0;
	while(dialtime > 0) {
	    if (*s == 0) {
		x = check_io(portfd, 0, 1000, buf, NULL);
		s = buf;
	    }
	    if (x & 2) {
		f = keyboard(KGETKEY, 0);
		/* Cancel if escape was pressed. */
		if (f == K_ESC) mputs(P_MDIALCAN, 0);

		/* On space retry. */
		if (f == ' ') {
			mputs(P_MDIALCAN, 0);
			dialfailed("Cancelled", 4);
			m_flush(portfd);
			break;
		}
		(void) keyboard(KSTOP, 0);
		wclose(dialwin, 1);
		return(retst);
	    }
	    if (x & 1) {
		/* Data available from the modem. Put in buffer. */
		if (*s == '\r' || *s == '\n') {
			/* We look for [\r\n]STRING[\r\n] */
			modbuf[modidx] = 0;
			modidx = 0;
		} else if (modidx < 127) {
			/* Normal character. Add. */
			modbuf[modidx++] = *s;
			modbuf[modidx] = 0;
		}
		/* Skip to next received char */
		if (*s) s++;
		/* Only look when we got a whole line. */
		if (modidx == 0 &&
		    !strncmp(modbuf, P_MCONNECT, strlen(P_MCONNECT))) {
			retst = 0;
			/* Try to do auto-bauding */
			if (sscanf(modbuf + strlen(P_MCONNECT), "%d", &nb) == 1)
				retst = nb;

			/* Try to figure out if this system supports DCD */
			f = m_getdcd(portfd);
			bogus_dcd = 1;

			wlocate(dialwin, 1, 6);
			if (d->script[0] == 0) {
				wputs(dialwin,
				     "Connected. Press any key to continue");
#if VC_MUSIC
				if (P_SOUND[0] == 'Y')
					music();
				else {
					x = check_io(-1, 0, 0, NULL, NULL);
					if ((x & 2) == 2)
						(void) keyboard(KGETKEY, 0);
				}
#else
				/* MARK updated 02/17/94 - If VC_MUSIC is not */
				/* defined, then at least make some beeps! */
				if (P_SOUND[0] == 'Y') 
					wputs(dialwin,"\007\007\007");
#endif
				x = check_io(-1, 0, 0, NULL, NULL);
				if ((x & 2) == 2)
					(void) keyboard(KGETKEY, 0);
			}
			keyboard(KSTOP, 0);
			wclose(dialwin, 1);
			/* Print out the connect strings. */
			wprintf(us, "\r\n%s\r\n", modbuf);
			dialwin = NIL_WIN;

			/* Un-tag this entry. */
			d->flags &= ~FL_TAG;
			return(retst);
		}
		for(f = 0; f < 3; f++) {
			if (f == 0) t = P_MNOCON1;
			if (f == 1) t = P_MNOCON2;
			if (f == 2) t = P_MNOCON3;
			if (f == 3) t = P_MNOCON4;
			if ((*t) && (!strncmp(modbuf, t, strlen(t)))) {
				if (retries < maxretries) {
					x = dialfailed(t, rdelay);
					if (x < 0) {
						keyboard(KSTOP, 0);
						wclose(dialwin, 1);
						return(retst);
					}
				}
				if (maxretries == 1) reason = t;
				goto MainLoop;
			}
		}
	    }

	    /* Do timer routines here. */
	    time(&now);
	    if (last != now) {
		dialtime -= (now - last);
		if (dialtime < 0) dialtime = 0;
		wlocate(dialwin, 11, 3);
		wprintf(dialwin, "%-3d  ", dialtime);
		if (dialtime <= 0) {
			mputs(P_MDIALCAN, 0);
			reason = "Timeout";
			retst = -1;
			if (retries < maxretries) {
				x = dialfailed(reason, rdelay);
				if (x < 0) {
					keyboard(KSTOP, 0);
					wclose(dialwin, 1);
					return(retst);
				}
			}
		}
	    }
	    last = now;
	}
  } /* End of main while cq MainLoop */	
  dialfailed(reason, -1);
  keyboard(KSTOP, 0);
  wclose(dialwin, 1);
  return(retst);
}

/*
 * Create an empty entry.
 */
static struct dialent *mkstdent()
{
  struct dialent *d;
  
  d = (struct dialent *)malloc(sizeof (struct dialent));
  
  if (d == (struct dialent *)0) return(d);

  d->name[0] = 0;
  d->number[0] = 0;
  d->script[0] = 0;
  d->username[0] = 0;
  d->password[0] = 0;
  d->term = 1;
  d->dialtype = 0;
  d->flags = FL_DEL;
  strcpy(d->baud, "Curr");
  strcpy(d->bits, "8");
  strcpy(d->parity, "N");
  d->next = (struct dialent *)0;

  return(d);
}

/* Read version 3 of the dialing directory. */
void v3_read(fp, d)
struct dialent *d;
FILE *fp;
{
  (void) fread((char *)d, sizeof(struct dialent), (size_t)1, fp);
}

/* Read version 2 of the dialing directory. */
void v2_read(fp, d)
struct dialent *d;
FILE *fp;
{
  (void) fread((char *)d, sizeof(struct dialent), (size_t)1, fp);
  if (d->flags & FL_ANSI) d->flags |= FL_WRAP;
}

/* Read version 1 of the dialing directory. */
void v1_read(fp, d)
FILE *fp;
struct dialent *d;
{
  struct v1_dialent v1;

  fread((char *)&v1, sizeof(v1), (size_t)1, fp);
  memcpy(d->username, v1.username, sizeof(v1) - offsetof(struct v1_dialent, username));
  strcpy(d->name, v1.name);
  strcpy(d->number, v1.number);
  strcpy(d->script, v1.script);
}

/* Read version 0 of the dialing directory. */
void v0_read(fp, d)
FILE *fp;
struct dialent *d;
{
  v1_read(fp, d);
  d->dialtype = 0;
  d->flags = 0;
}

/*
 * Read in the dialing directory from $HOME/.dialdir
 */
int readdialdir()
{
  long size;
  FILE *fp;
  char dfile[256];
  static int didread = 0;
  int f;
  struct dialent *d = NULL, *prev = (struct dialent *)0;
  struct dver dial_ver;

  if (didread) return(0);
  didread = 1;
  nrents = 1;
  tagged = (char *)malloc(1);
  tagged[0] = 0;

  /* Make the manual dial entry. */
  d_man = mkstdent();
  strcpy(d_man->name, "Manually entered number");

  /* Construct path */
  sprintf(dfile, "%s/.dialdir", homedir);

  /* Try to open ~/.dialdir */
  if ((fp = sfopen(dfile, "r")) == (FILE *)NULL) {
	if (errno == EPERM) {
		werror("Cannot open ~/.dialdir: permission denied");
		dialents = mkstdent();
		dendd = 1;
		return(0);
	}
  	dialents = mkstdent();
  	return(0);
  }

  /* Get size of the file */
  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);
  if (size == 0) {
  	dialents = mkstdent();
  	fclose(fp);
  	return(0);
  }

  /* Get version of the dialing directory */
  fseek(fp, 0L, SEEK_SET);
  fread(&dial_ver, sizeof(dial_ver), 1, fp);
  if (dial_ver.magic != DIALMAGIC) {
	/* First version without version info. */
	dial_ver.version = 0;
	fseek(fp, 0L, SEEK_SET);
  } else
	size -= sizeof(dial_ver);

  /* See if the size of the file is allright. */
  switch(dial_ver.version) {
	case 0:
	case 1:
		dial_ver.size = sizeof(struct v1_dialent);
		break;
	case 2:
	case 3:
		dial_ver.size = sizeof(struct dialent);
		break;
  }

  if (size % dial_ver.size != 0) {
  	werror("Phonelist garbled (?)");
  	fclose(fp);
#if 0 /* This should be less facistic now. */
  	unlink(dfile);
#else
	dendd = 1;
#endif
  	dialents = mkstdent();
  	return(-1);
  }

  /* Read in the dialing entries */
  nrents = size / dial_ver.size;
  if (nrents == 0) {
	dialents = mkstdent();
	nrents = 1;
	fclose(fp);
	return(0);
  }
  for(f = 1; f <= nrents; f++) {
  	if ((d = (struct dialent *)malloc(sizeof (struct dialent))) ==
  		(struct dialent *)0) {
  			if(f == 1)
				dialents = mkstdent();
			else
				prev->next = (struct dialent *)0;

  			werror("Out of memory while reading dialing directory");
			fclose(fp);
  			return(-1);
  	}
	switch(dial_ver.version) {
		case 0:
			v0_read(fp, d);
			break;
		case 1:
			v1_read(fp, d);
			break;
		case 2:
			v2_read(fp, d);
			break;
		case 3:
			v3_read(fp, d);
			break;
	}
	/* MINIX terminal type is obsolete */
	if (d->term == 2) d->term = 1;

  	if (prev != (struct dialent *)0)
  		prev->next = d;
  	else
  		dialents = d;
  	prev = d;
  }
  d->next = (struct dialent *)0;
  fclose(fp);
  return(0);
}

/*
 * Write the new $HOME/.dialdir
 */
static void writedialdir()
{
  struct dialent *d;
  char dfile[256];
  FILE *fp;
  struct dver dial_ver;
  char oldfl;

  /* Make no sense if access denied */
  if (dendd) return;

  sprintf(dfile, "%s/.dialdir", homedir);

  if ((fp = sfopen(dfile, "w")) == (FILE *)0) {
  	werror("Can't write to ~/.dialdir");
	dendd = 1;
  	return;
  }

  d = dialents;
  /* Set up version info. */
  dial_ver.magic = DIALMAGIC;
  dial_ver.version = 3;
  dial_ver.size = sizeof(struct dialent);
  fwrite(&dial_ver, sizeof(dial_ver), (size_t)1, fp);

  /* Write dialing directory */
  while(d) {
	oldfl = d->flags;
	d->flags &= FL_SAVE;
  	if (fwrite(d, sizeof(struct dialent), (size_t)1, fp) != 1) {
  		werror("Error writing ~/.dialdir!");
  		fclose(fp);
  		return;
  	}
	d->flags = oldfl;
  	d = d->next;
  }
  fclose(fp);
}


/*
 * Get entry "no" in list.
 */
static struct dialent *getno(no)
int no;
{
  struct dialent *d;
  
  d = dialents;
  
  if (no >= nrents) return((struct dialent *)NULL);

  while(no--) d = d->next;
  return(d);
}

/* Note: Minix does not exist anymore. */
static char *te[] = { "VT102", "MINIX", "ANSI " };

/*
 * Edit an entry.
 */
static void dedit(d)
struct dialent *d;
{
  WIN *w;
  int c;
  
  w = wopen(5, 5, 75, 17, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wprintf(w, " A -  Name                : %s\n", d->name);
  wprintf(w, " B -  Number              : %s\n", d->number);
  wprintf(w, " C -  Dial string #       : %d\n", d->dialtype + 1);
  wprintf(w, " D -  Local echo          : %s\n", yesno(d->flags & FL_ECHO));
  wprintf(w, " E -  Script              : %s\n", d->script);
  wprintf(w, " F -  Username            : %s\n", d->username);
  wprintf(w, " G -  Password            : %s\n", d->password);
  wprintf(w, " H -  Terminal Emulation  : %s\n", te[d->term - 1]);
  wprintf(w, " I -  Backspace key sends : %s\n",
	d->flags & FL_DEL ? "Delete" : "Backspace");
  wprintf(w, " J -  Linewrap            : %s\n",
	d->flags & FL_WRAP ? "On" : "Off");
  wprintf(w, " K -  Line Settings       : %s %s%s1",
  	d->baud, d->bits, d->parity);
  wlocate(w, 4, 13);
  wputs(w, "Change which setting? ");
  wredraw(w, 1);

  while(1) {
      wlocate(w, 26, 13);
      c = wxgetch();
      if (c >= 'a') c -= 32;
      switch(c) {
        case '\033':
        case '\r':
  	case '\n':
  		wclose(w, 1);
  		return;
  	case 'A':
  		wlocate(w, 28, 0);
  		(void) wgets(w, d->name, 31, 32);
  		break;
  	case 'B':
  		wlocate(w, 28, 1);
  		(void) wgets(w, d->number, 31, 32);
  		break;
	case 'C':
		d->dialtype = (d->dialtype + 1) % 3;
		wlocate(w, 28, 2);
		wprintf(w, "%d", d->dialtype + 1);
		wflush();
		break;
	case 'D':
		d->flags ^= FL_ECHO;
		wlocate(w, 28, 3);
		wprintf(w, "%s", yesno(d->flags & FL_ECHO));
		wflush();
		break;
  	case 'E':
  		wlocate(w, 28, 4);
  		(void) wgets(w, d->script, 31, 32);	
  		break;
  	case 'F':
  		wlocate(w, 28, 5);
  		(void) wgets(w, d->username, 31, 32);
  		break;
  	case 'G':
  		wlocate(w, 28, 6);
  		(void) wgets(w, d->password, 31, 32);
  		break;	
  	case 'H':
  		d->term = (d->term % 3) + 1;
		/* MINIX == 2 is obsolete. */
		if (d->term == 2) d->term = 3;
  		wlocate(w, 28, 7);
  		wputs(w, te[d->term - 1]);	

		/* Also set backspace key. */
		if (d->term == ANSI) {
			d->flags &= ~FL_DEL;
			d->flags |= FL_WRAP;
		} else {
			d->flags &= ~FL_WRAP;
			d->flags |= FL_DEL;
		}
		wlocate(w, 28, 8);
		wputs(w, d->flags & FL_DEL ? "Delete   " : "Backspace");
		wlocate(w, 28, 9);
		wputs(w, d->flags & FL_WRAP ? "On " : "Off");
  		break;
	case 'I':
		d->flags ^= FL_DEL;
		wlocate(w, 28, 8);
		wputs(w, d->flags & FL_DEL ? "Delete   " : "Backspace");
		break;
	case 'J':
		d->flags ^= FL_WRAP;
		wlocate(w, 28, 9);
		wputs(w, d->flags & FL_WRAP ? "On " : "Off");
		break;
  	case 'K':
  		get_bbp(d->baud, d->bits, d->parity, 1);
  		wlocate(w, 28, 10);
  		wprintf(w, "%s %s%s1  ", d->baud, d->bits, d->parity);
  		break;
  	default:
  		break;
      }
  }
}

static WIN *dsub;
static char *what =  "  Dial    Find    Add     Edit   Remove  Manual ";
static int dprev;

/*
 * Highlight a choice in the horizontal menu.
 */
static void dhili(k)
int k;
{
  if (k == dprev) return;

  if (dprev >= 0) {
  	wlocate(dsub, 14 + 8*dprev, 0);
	if (!useattr) {
		wputs(dsub, " ");
	} else {
  		wsetattr(dsub, XA_REVERSE | stdattr);
  		wprintf(dsub, "%8.8s", what + 8*dprev);
	}
  }
  dprev = k;
  wlocate(dsub, 14 + 8*k, 0);
  if (!useattr) {
	wputs(dsub, ">");
  } else {
	wsetattr(dsub, stdattr);
	wprintf(dsub, "%8.8s", what + 8*k);
  }
}


static char *fmt = "\r %2d %c %-16.16s %-16.16s%5s %s%s1    %-6.6s %-15.15s\n";

/*
 * Print the dialing directory. Only draw from "cur" to bottom.
 */
static void prdir(dialw, top, cur)
WIN *dialw;
int top, cur;
{
  int f, start;
  struct dialent *d;

  start = cur - top;
  dirflush = 0;
  wlocate(dialw, 0, start + 1);
  for(f = start; f < dialw->ys - 2; f++) {
  	d = getno(f + top);
  	if (d == (struct dialent *)0) break;
  	wprintf(dialw, fmt, f+1+top, (d->flags & FL_TAG) ? 16 : ' ',
		d->name, d->number,
  		d->baud, d->bits, d->parity, te[d->term - 1], d->script);
  }
  dirflush = 1;
  wflush();
}

/* Little menu. */
static char *d_yesno[] = { "   Yes  ", "   No   ", CNULL };

/* Try to dial an entry. */
static void dial_entry(d)
struct dialent *d;
{
  int nb;

  /* Change settings for this entry. */
  if (atoi(d->baud) != 0) {
  	strcpy(P_BAUDRATE, d->baud);
	strcpy(P_PARITY, d->parity);
	strcpy(P_BITS, d->bits);
	port_init();
	mode_status();
  }
  newtype = d->term;
  vt_set(-1, d->flags & FL_WRAP, NULL, -1, -1, d->flags & FL_ECHO, -1);
#ifdef _SELECT
  local_echo = d->flags & FL_ECHO;
#endif
  if (newtype != terminal)
	init_emul(newtype, 1);

  /* Set backspace key. */
  keyboard(KSETBS, d->flags & FL_DEL ? 127 : 8);
  strcpy(P_BACKSPACE, d->flags & FL_DEL ? "DEL" : "BS");

  /* Now that everything has been set, dial. */
  if ((nb = dial(d)) < 0) return;

  /* Did we detect a baudrate , and can we set it? */
  if (P_MAUTOBAUD[0] == 'Y' && nb) {
	sprintf(P_BAUDRATE, "%d", nb);
	port_init();
	mode_status();
  }

  /* Run script if needed. */
  if (d->script[0]) runscript(0, d->script, d->username, d->password);

  /* Remember _what_ we dialed.. */
  dial_name = d->name;
  dial_number = d->number;
  return;
}

/*
 * Dial an entry from the dialing directory; this
 * is used for the "-d" command line flag.
 */
void dialone(entry)
char *entry;
{
  int num;
  struct dialent *d;
  char buf[128];

  /* Find entry. */
  if ((num = atoi(entry)) != 0)
	d = getno(num - 1);
  else
	for(d = dialents; d; d = d->next)
		if (strstr(d->name, entry)) break;

  /* Not found. */
  if (d == NULL) {
	sprintf(buf, "Entry \"%s\" not found. Enter dialdir?", entry);
	if (ask(buf, d_yesno) != 0) return;
	dialdir();
	return;
  }
  /* Dial the number! */
  sleep(1);
  dial_entry(d);
}

/*
 * Draw the dialing directory.
 */
void dialdir()
{
  WIN *w;
  struct dialent *d = NULL, *d1, *d2;
  static int cur = 0;
  static int ocur = 0;
  int subm = 0;
  int quit = 0;
  static int top = 0;
  int c = 0;
  int pgud = 0;
  int first = 1;
  int x1, x2;
  char *s, dname[64];
  static char manual[64];
  int changed = 0;

  dprev = -1;
  dname[0] = 0;

  /* Allright, draw the dialing directory! */
  
  dirflush = 0;
  x1 = (COLS / 2) - 37;
  x2 = (COLS / 2) + 37;
  dsub = wopen(x1 - 1, LINES - 3, x2 + 1, LINES - 3, BNONE, 
  		XA_REVERSE | stdattr, mfcolor, mbcolor, 0, 0, 1);
  w = wopen(x1, 2, x2, LINES - 6, BSINGLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wcursor(w, CNONE);
  wtitle(w, TMID, "Dialing Directory");
  wputs(w, "     Name               Number       Line Format Terminal Script\n");
  wlocate(dsub, 14, 0);
  wputs(dsub, what);

  wsetregion(w, 1, w->ys - 1);
  w->doscroll = 0;

  prdir(w, top, top);  
  wlocate(w, 14, w->ys - 1);
  wputs(w, "( Escape to exit, Space to tag )");
  dhili(subm);
  dirflush = 1;
  wredraw(dsub, 1);

again:  
  wcurbar(w, cur + 1 - top, XA_REVERSE | stdattr);
  if (first) {
  	wredraw(w, 1);
  	first = 0;
  }
  while(!quit) {
  	d = getno(cur);
  	switch(c = wxgetch()) {
  		case K_UP:
  		case 'k':
  			cur -= (cur > 0);
  			break;
  		case K_DN:
  		case 'j':
  			cur += (cur < nrents - 1);
  			break;
  		case K_LT:
  		case 'h':
			subm--;
			if (subm < 0) subm = 5;
  			break;
  		case K_RT:
  		case 'l':
  			subm = (subm + 1) % 6;
  			break;
  		case K_PGUP:
  		case '\002': /* Control-B */
  			pgud = 1;
  			quit = 1;
  			break;	
  		case K_PGDN:
  		case '\006': /* Control-F */
  			pgud = 2;
  			quit = 1;
  			break;	
		case 'd':    /* Dial. */
			subm = 0;
			quit = 1;
			break;
		case 'f':    /* Find. */
			subm = 1;
			quit = 1;
			break;
		case 'a':    /* Add. */
			subm = 2;
			quit = 1;
			break;
		case 'e':    /* Edit. */
			subm = 3;
			quit = 1;
			break;
		case 'r':    /* Remove. */
			subm = 4;
			quit = 1;
			break;
		case 'm':    /* Manual. */
			subm = 5;
			quit = 1;
			break;
		case ' ':    /* Tag. */
  			wlocate(w, 4, cur + 1 - top);
			d->flags ^= FL_TAG;
			wsetattr(w, XA_REVERSE | stdattr);
			wprintf(w, "%c", d->flags & FL_TAG ? 16 : ' ');
			wsetattr(w, XA_NORMAL | stdattr);
  			cur += (cur < nrents - 1);
			break;
  		case '\033':
  		case '\r':
  		case '\n':
  			quit = 1;
  			break;
  		default:
  			break;
  	}
  	/* Decide if we have to delete the cursor bar */
  	if (cur != ocur || quit) wcurbar(w, ocur + 1 - top, XA_NORMAL | stdattr);
  		
  	if (cur < top) {
  		top--;
  		prdir(w, top, top);	
	}
	if (cur - top > w->ys - 3) {
		top++;
  		prdir(w, top, top);
	}
  	if (cur != ocur) wcurbar(w, cur + 1 - top, XA_REVERSE | stdattr);
  	ocur = cur;
  	dhili(subm);
  }
  quit = 0;
  /* ESC means quit */
  if (c == '\033') {
  	if (changed) writedialdir();
	wclose(w, 1);
	wclose(dsub, 1);
	return;
  }
  /* Page up or down ? */
  if (pgud == 1) { /* Page up */
  	ocur = top;
  	top -= w->ys - 2;
  	if (top < 0) top = 0;
  	cur = top;
  	pgud = 0;
  	if (ocur != top) prdir(w, top, cur);
  	ocur = cur;
	goto again;
  }
  if (pgud == 2) { /* Page down */
  	ocur = top;
  	if (top < nrents - w->ys + 2) {
		top += w->ys - 2;
		if (top > nrents - w->ys + 2) {
			top = nrents - w->ys + 2;
		}
		cur = top;
	} else
		cur = nrents - 1;
	pgud = 0;
	if (ocur != top) prdir(w, top, cur);
	ocur = cur;
	goto again;
  }
  
  /* Dial an entry */
  if (subm == 0) {
  	wclose(w, 1);
  	wclose(dsub, 1);
  	if (changed) writedialdir();

	/* See if any entries were tagged. */
	if (!(d->flags & FL_TAG)) {
		for(d1 = dialents; d1; d1 = d1->next)
			if (d1->flags & FL_TAG) {
				d = d1;
				break;
			}
	}
	dial_entry(d);
	return;
  }
  /* Find an entry */
  if (subm == 1) {
	s = input("Find an entry", dname);
	if (s == NULL || s[0] == 0) goto again;
	x1 = 0;
	for(d = dialents; d; d = d->next, x1++)
		if (strstr(d->name, s)) break;
	if (d == NULL) {
		wbell();
		goto again;
	}
	/* Set current to found entry. */
	ocur = top;
	cur = x1;
	/* Find out if it fits on screen. */
	if (cur < top || cur >= top + w->ys - 2) {
		/* No, try to put it in the middle. */
		top = cur - (w->ys / 2) + 1;
		if (top < 0) top = 0;
		if (top > nrents - w->ys + 2) {
			top = nrents - w->ys + 2;
		}
	}
	if (ocur != top) prdir(w, top, top);
	ocur = cur;
  }

  /* Add / insert an entry */
  if (subm == 2) {
  	d1 = mkstdent();
  	if (d1 == (struct dialent *)0) {
  		wbell();
  		goto again;
  	}
	changed++;
  	cur++;
  	ocur = cur;
  	d2 = d->next;
  	d->next = d1;
  	d1->next = d2;
  	
  	nrents++;
  	if (cur - top > w->ys - 3) {
  		top++;
  		prdir(w, top, top);
  	} else {
  		prdir(w, top, cur);
	}
  }

  /* Edit an entry */
  if (subm == 3) {
  	dedit(d);
	changed++;
  	wlocate(w, 0, cur + 1 - top);
  	wprintf(w, fmt, cur+1, (d->flags & FL_TAG) ? 16 : ' ', d->name,
	d->number, d->baud, d->bits, d->parity, te[d->term - 1], d->script);
  }
  
  /* Delete an entry from the list */
  if (subm == 4 && ask("Remove entry?", d_yesno) == 0) {
	changed++;
  	if (nrents == 1) {
  		free((char *)d);
  		d = dialents = mkstdent();
  		prdir(w, top, top);
  		goto again;
  	}
  	if (cur == 0)
  		dialents = d->next;
  	else
  		getno(cur - 1)->next = d->next;
  	free((char *)d);
  	nrents--;
  	if (cur - top == 0 && top == nrents) {
  		top--;
  		cur--;
  		prdir(w, top, top);
  	} else {
  		if (cur == nrents) cur--;
		prdir(w, top, cur);
  	}
	if (nrents - top <= w->ys - 3) {
		wlocate(w, 0, nrents - top + 1);
		wclreol(w);
	}
  	ocur = cur;
  }
  /* Dial a number manually. */
  if (subm == 5) {
	s = input("Enter number", manual);
	if (s && *s) {
  		if (changed) writedialdir();
		wclose(w, 1);
		wclose(dsub, 1);
		
		strcpy(d_man->number, manual);
  		(void) dial(d_man);
  		return;
	}
  }
  goto again;
}
