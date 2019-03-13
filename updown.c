/*
########################################################################
# This file is part of the minicom communications package for WRAMP.
#
# Copyright 1991-1995 Miquel van Smoorenburg.
# Copyright (C) 2019 The University of Waikato, Hamilton, New Zealand.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
########################################################################
*/
#include "port.h"
#include "remote.h"

static int udpid;
static char fname_buf[256];

/*
 * Change to a directory.
 */
static int mcd(dir)
char *dir;
{
  char buf[256];
  char err[50];
  static char odir[256];
  static int init = 0;

  if (!init) {
  	if (*dir == 0) return(0);
  	init = 1;
#if defined (_COH3) || defined(NeXT)
	getwd(odir);
#else
  	getcwd(odir, 255);
#endif
  }
  if (*dir == 0) {
  	chdir(odir);
  	return(0);
  }
  
  if(*dir != '/') {
  	sprintf(buf, "%s/%s", homedir, dir);
  	dir = buf;
  }
  if (chdir(dir) < 0) {
  	sprintf(err, "Cannot chdir to %.30s", dir);
  	werror(err);
  	return(-1);
  }
  return(0);
}

/*
 * Catch the CTRL-C signal.
 */
/*ARGSUSED*/
static void udcatch(dummy)
int dummy;
{
  (void)dummy;
  signal(SIGINT, udcatch);
  if (udpid) kill((pid_t)udpid, SIGKILL);
}

/*
 * Translate %b to the current baudrate, and
 *           %l to the current tty port.
 */
static char *translate(s)
char *s;
{
  static char buf[128];
  int i;

  for(i = 0; *s && i < 127; i++, s++) {
  	if (*s != '%') {
  		buf[i] = *s;
  		continue;
  	}
  	switch(*++s) {
  		case 'l':
  			strcpy(buf + i, dial_tty);
  			i += strlen(dial_tty) - 1;
  			break;
  		case 'b':
  			strcpy(buf + i, P_BAUDRATE);
  			i += strlen(P_BAUDRATE) - 1;
  			break;
  		default:
  			buf[i++] = '%';
  			buf[i] = *s;
  			break;
  	}
  }
  buf[i] = 0;
  return(buf);
}

/*
 * Choose from numerous up and download protocols!
 */
void updown(what, nr)
int what;
int nr;
{
  char *name[13];
  int idx[13];
  int r, f, g = 0;
  char *t = what == 'U' ? "Upload" : "Download";
  static char buf[128] = "";
  char title[64];
  char *s ="";
  int pipefd[2];
  int n, status;
  char cmdline[128];
  WIN *win = (WIN *)NULL;
#if VC_MUSIC
  _PROTO(void music, (void));
#endif

  if (mcd(what == 'U' ? P_UPDIR : P_DOWNDIR) < 0)
  	return;

  /* Automatic? */
  if (nr == 0) {
	for(f = 0; f < 12; f++) {
  		if (P_PNAME(f)[0] && P_PUD(f) == what) {
  			name[g] = P_PNAME(f);
  			idx[g++] = f;
  		}
	}
	name[g] = CNULL;
	if (g == 0) return;

	if ( g == 1 )
           r = 0;
         else
           r = wselect(30, 7, name, NIL_FUNLIST, t, stdattr, mfcolor, mbcolor)
               - 1;
	if (r < 0) return;

	g = idx[r];
  } else g = nr;

  if (P_PNN(g) == 'Y') {
  	s = input("Please enter file name", fname_buf);
  	if (s == CNULL || *s == 0) return;
  strcpy(buf, fname_buf);
  }

  sprintf(cmdline, "%s %s", P_PPROG(g), s);

  if (P_PFULL(g) == 'N') {
	win = wopen(10, 7, 70, 13, BSINGLE, stdattr, mfcolor, mbcolor, 1, 0, 1);
	sprintf(title, "%.30s %s - Press CTRL-C to quit", P_PNAME(g),
		what == 'U' ? "upload" : "download");
	wtitle(win, TMID, title);
	pipe(pipefd);
  } else
	wleave();

  switch(udpid = fork()) {
  	case -1:
  		werror("Out of memory: could not fork()");
		if (win) {
  			close(pipefd[0]);
  			close(pipefd[1]);
	  		wclose(win, 1);
		} else
			wreturn();
  		(void) mcd("");
  		return;
  	case 0: /* Child */
		if (P_PIORED(g) == 'Y') {
  			dup2(portfd, 0);
  			dup2(portfd, 1);
		}
		if (win) {
  			dup2(pipefd[1], 2);
  			close(pipefd[0]);
  			if (pipefd[1] != 2) close(pipefd[1]);
  		}
  		for(n = 1; n < _NSIG; n++) signal(n, SIG_DFL);
  		
		set_privs();
		setgid((gid_t)real_gid);
  		setuid((uid_t)real_uid);
		if (fastexec(translate(cmdline)) == -1)
		{
			fprintf(stderr, "\n\n");
			fprintf(stderr, "                    Could not find down!\n");
			fprintf(stderr, "          Please contact your system administrator\n\n");
		}
  		exit(1);
  	default: /* Parent */
  		break;
  }
  if (win) {
	(void) setcbreak(1); /* Cbreak, no echo. */
	enab_sig(1, 0);	       /* But enable SIGINT */
  }
  signal(SIGINT, udcatch);
  if (P_PIORED(g) == 'Y') {
	close(pipefd[1]);
	while((n = read(pipefd[0], buf, 80)) > 0) {
	  	buf[n] = '\0';
  		wputs(win, buf);
		timer_update();
	}
  }
  while( udpid != m_wait(&status) );
  if (win) {
	enab_sig(0, 0);
	signal(SIGINT, SIG_IGN);
  }

  if (win == (WIN *)0) wreturn();

  /* MARK updated 02/17/94 - Flush modem port before displaying READY msg */
  /* because a BBS often displays menu text right after a download, and we */
  /* don't want the modem buffer to be lost while waiting for key to be hit */
  m_flush(portfd);
  port_init();
  (void) setcbreak(2); /* Raw, no echo. */
  close(pipefd[0]);
  (void) mcd("");

  {
  char line[132];

  strcpy(line, "\nDone: press enter to continue...");
  wprintf(win, line);
  line[0] = '\0';
  //  wgets(win, line, 132, 132);
  }
  if (P_SOUND[0] == 'Y') {
#if VC_MUSIC
  music();
#else
  /* MARK updated 02/17/94 - If there was no VC_MUSIC capability, */
  /* then at least make some beeps! */
  if (P_SOUND[0] == 'Y') wprintf(win, "\007\007\007");
  sleep(1);
#endif
		}
  if (win) wclose(win, 1);
  }

/*
 * Run kermit. Used to do this in the main window, but newer
 * versions of kermit are too intelligent and just want a tty
 * for themselves or they won't function ok. Shame.
 */
void kermit()
{
  int status;
  int pid, n;
  char buf[81];
  int fd;

  /* Clear screen, set keyboard modes etc. */
  wleave();

  switch(pid = fork()) {
  	case -1:
		wreturn();
  		werror("Out of memory: could not fork()");
  		return;
  	case 0: /* Child */
  		/* Remove lockfile */
		set_privs();
  		if (lockfile[0]) unlink(lockfile);
		setgid((gid_t)real_gid);
		setuid((uid_t)real_uid);

  		for(n = 0; n < _NSIG; n++) signal(n, SIG_DFL);

  		(void) fastexec(translate(P_KERMIT));
  		exit(1);
  	default: /* Parent */
  		break;
  }

  (void) m_wait(&status);

  /* Restore screen and keyboard modes */
  wreturn();

  /* Re-create lockfile */
  if (lockfile[0]) {
	set_privs();
	n = umask(022);
  	/* Create lockfile compatible with UUCP-1.2 */
  	if ((fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666)) < 0) {
  		werror("Cannot re-create lockfile!");
  	} else {
		(void) chown(lockfile, (uid_t)real_uid, (gid_t)real_gid);
  		sprintf(buf, "%05d remote %.20s\n", (int)getpid(), username);
  		write(fd, buf, strlen(buf));
  		close(fd);
  	}
	(void) umask(n);
	drop_privs();
  }
  m_flush(portfd);
  port_init();
}

/* ============ Here begins the setenv function ============= */
/*
 * Compare two strings up to '='
 */
static int varcmp(s1, s2)
char *s1, *s2;
{
  while(*s1 && *s2) {
  	if (*s1 == '=' && *s2 == '=') return(1);
  	if (*s1++ != *s2++) return(0);
  }
  return(1);
}

/*
 * Generate a name=value string.
 */
static char *makenv(name, value)
char *name, *value;
{
  char *p;
  
  if ((p = (char *)malloc(strlen(name) + strlen(value) + 3)) == CNULL)
	return(p);
  sprintf(p, "%s=%s", name, value);
  return(p);
}

/*
 * Set a environment variable. 
 */
int mc_setenv(name, value)
char *name, *value;
{
  static int init = 0;
  extern char **environ;
  char *p, **e, **newe;
  int count = 0;

  if ((p = makenv(name, value)) == CNULL) return(-1);

  for(e = environ; *e; e++) {
  	count++;
  	if(varcmp(name, *e)) {
  		*e = p;
  		return(0);
  	}
  }
  count += 2;
  if ((newe = (char **)malloc(sizeof(char *) * count)) == (char **)0) {
  	free(p);
  	return(-1);
  }
  memcpy((char *)newe, (char *)environ , (int) (count * sizeof(char *)));
  if (init) free((char *)environ);
  init = 1;
  environ = newe;
  for(e = environ; *e; e++)
  	;
  *e++ = p;
  *e = CNULL;
  return(0);
}

/* ============ This is the end of the setenv function ============= */

/*
 * Run an external script.
 * ask = 1 if first ask for confirmation.
 * s = scriptname, l=loginname, p=password.
 */
void runscript(ask, s, l, p)
int ask;
char *s, *l, *p;
{
  int status;
  int n;
  int pipefd[2];
  char buf[81];
  char cmdline[128];
  char *ptr;
  WIN *w;
  int done = 0;
  char *msg = "Same as last";

  if (ask) {
	w = wopen(10, 5, 70, 10, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
	wtitle(w, TMID, "Run a script");
	wputs(w, "\n");
	wprintf(w, " A -   Username        : %s\n",
					scr_user[0] ? msg : "");
	wprintf(w, " B -   Password        : %s\n",
					scr_passwd[0] ? msg : "");
	wprintf(w, " C -   Name of script  : %s\n", scr_name);
	wlocate(w, 4, 5);
	wputs(w, "Change which setting?     (Return to run, ESC to stop)");
	wredraw(w, 1);

	while(!done) {
	    wlocate(w, 26, 5);
	    n = wxgetch();
	    if (islower(n)) n = toupper(n);
	    switch(n) {
		case '\r':
		case '\n':
			if (scr_name[0] == '\0') {
				wbell();
				break;
			}
			wclose(w, 1);
			done = 1;
			break;
		case 27: /* ESC */
			wclose(w, 1);
			return;
		case 'A':
			wlocate(w, 25, 1);
			wclreol(w);
			scr_user[0] = 0;
			wgets(w, scr_user, 32, 32);
			break;
		case 'B':
			wlocate(w, 25, 2);
			wclreol(w);
			scr_passwd[0] = 0;
			wgets(w, scr_passwd, 32, 32);
			break;
		case 'C':
			wlocate(w, 25, 3);
			wgets(w, scr_name, 32, 32);
			break;
		default:
			break;
	    }
	}
  } else {
  	strcpy(scr_user, l);
  	strcpy(scr_name, s);
  	strcpy(scr_passwd, p);
  }

  /* Throw away status line if temporary */
  if (tempst) {
  	wclose(st, 1);
  	tempst = 0;
  	st = NIL_WIN;
  }
  scriptname(scr_name);
  
  pipe(pipefd);

  if (mcd(P_SCRIPTDIR) < 0) return;

  sprintf(cmdline, "%s %s", P_SCRIPTPROG, scr_name);

  switch(udpid = fork()) {
  	case -1:
  		werror("Out of memory: could not fork()");
  		close(pipefd[0]);
  		close(pipefd[1]);
  		(void) mcd("");
  		return;
  	case 0: /* Child */
  		dup2(portfd, 0);
  		dup2(portfd, 1);
  		dup2(pipefd[1], 2);
  		close(pipefd[0]);
  		close(pipefd[1]);
  		
  		for(n = 1; n < _NSIG; n++) signal(n, SIG_DFL);
  		
		set_privs();
		setgid((gid_t)real_gid);
  		setuid((uid_t)real_uid);
  		mc_setenv("LOGIN", scr_user);
  		mc_setenv("PASS", scr_passwd);
  		(void) fastexec(translate(cmdline));
  		exit(1);
  	default: /* Parent */
  		break;
  }
  (void) setcbreak(1); /* Cbreak, no echo */
  enab_sig(1, 0);	       /* But enable SIGINT */
  signal(SIGINT, udcatch);
  close(pipefd[1]);
  
  /* pipe output from "runscript" program to terminal emulator */
  while((n = read(pipefd[0], buf, 80)) > 0) {
  	ptr = buf;
  	while(n--)
  		vt_out(*ptr++);
	timer_update();
  	wflush();
  }
  
  /* Collect status, and clean up. */
  (void) m_wait(&status);
  enab_sig(0, 0);
  signal(SIGINT, SIG_IGN);
  (void) setcbreak(2); /* Raw, no echo */
  close(pipefd[0]);
  scriptname("");
  (void) mcd("");
}
