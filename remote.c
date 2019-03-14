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
 * fmg 1/11/94 colors
 *
 */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define EXTERN
#include "port.h"
#include "remote.h"
#include "patchlevel.h"

#define RESET 1
#define NORESET 2

#ifdef _SVR2
extern struct passwd *getpwuid();
#endif

#ifdef DEBUG
/* Show signals when debug is on. */
static void signore(sig)
int sig;
{
  if (stdwin != NIL_WIN)
	werror("Got signal %d", sig);
  else
	printf("Got signal %d\r\n", sig);
}
#endif

/*
 * Sub - menu's.
 */

static char *c1[] = { "   Yes  ", "   No   ", CNULL };
static char *c2[] = { "  Close ", " Pause  ", "  Exit  ", CNULL };
static char *c3[] = { "  Close ", " Unpause", "  Exit  ", CNULL };
static char *c7[] = { "   Yes  ", "   No   ", CNULL };
char * LIBDIRREAL[100];

/* Initialize modem port. */
void port_init()
{
  /* -- not if its a socket */
  if( isSocket )
    return;
  
  m_setparms(portfd, P_BAUDRATE, P_PARITY, P_BITS, P_HASRTS[0] == 'Y',
	P_HASXON[0] == 'Y');
}

static void do_hang(askit)
int askit;
{
  int c = 0;

  if( isSocket ) /*  not if its a socket */
    return ;
 
  if (askit) c = ask("Hang-up line?", c7);
  if (c == 0) hangup();
}

/*
 * We've got the hangup or term signal.
 */
static void hangsig(sig)
int sig;
{
  if (stdwin != NIL_WIN)
	werror("Killed by signal %d !\n", sig);
  if (capfp != (FILE *)0) fclose(capfp);
  keyboard(KUNINSTALL, 0);

  if( !isSocket ) /* only hangup and reset if its a modem */
    {
      hangup();
      modemreset();
    }
  leave("\n");
}

/*
 * Jump to a shell
 */
#ifdef SIGTSTP
/*ARGSUSED*/
static void shjump(sig)
int sig;
{
  extern int use_status;

  wleave();
  signal(SIGTSTP, SIG_DFL);
  printf("Suspended. Type \"fg\" to resume.\n");
  kill(getpid(), SIGTSTP);
  signal(SIGTSTP, shjump);
  wreturn();
  if (use_status) show_status();
}
#else
/*ARGSUSED*/
static void shjump(dummy)
int dummy;
{
  extern int use_status;

  char *sh;
  int pid;
  int status;
  int f;

  sh = getenv("SHELL");
  if (sh == CNULL) {
  	werror("SHELL variable not set");
  	return;
  }
  if ((pid = fork()) == -1) {
  	werror("Out of memory: could not fork()");
  	return;
  }
  if (pid != 0) wleave();
  if (pid == 0) {
	for(f = 1; f < _NSIG; f++) signal(f, SIG_DFL);
  	for(f = 3; f < 20; f++) close(f);
	set_privs();
	setgid((gid_t)real_gid);
  	setuid((uid_t)real_uid);
	printf("Shelled out. Type \"exit\" to return.\n");
  	execl(sh, sh, CNULL);
  	exit(1);
  }
  (void) m_wait(&status);
  wreturn();
  if (use_status) show_status();
}
#endif 

#if HISTORY
/* Get a line from either window or scroll back buffer. */
static ELM *getline_elm(w, no)
WIN *w;
int no;
{
  int i;

  if (no < us->histlines) {
	/* Get a line from the history buffer. */
	i = no + us->histline /*- 1*/;
	if (i >= us->histlines) i -= us->histlines;
	if (i < 0) i = us->histlines - 1;
	return(us->histbuf + (i * us->xs));
  }

  /* Get a line from the "us" window. */
  no -= us->histlines;
  if (no >= w->ys) no = w->ys - 1;
  return(w->map + (no * us->xs));
}

/* Redraw the window. */
static void drawhist(w, y, r)
WIN *w;
int y;
int r;
{
  int f;

  w->direct = 0;
  for(f = 0; f < w->ys; f++)
	wdrawelm(w, f, getline_elm(w, y++));
  if (r) wredraw(w, 1);
  w->direct = 1;
}

/* Scroll back */
static void scrollback()
{
  int y;
  WIN *b_us, *b_st;
  int c;
  char hline[128];

  /* Find out how big a window we must open. */
  y = us->y2;
  if (st == (WIN *)0 || (st && tempst)) y--;

  /* Open a window. */ 
  b_us = wopen(0, 0, us->x2, y, 0, us->attr, COLFG(us->color),
		COLBG(us->color), 0, 0, 0);
  wcursor(b_us, CNONE);

  /* Open a help line window. */
  b_st = wopen(0, y+1, us->x2, y+1, 0, st_attr, sfcolor, sbcolor, 0, 0, 1);
  b_st->doscroll = 0;
  b_st->wrap = 0;

  /* Make sure help line is as wide as window. */
  strcpy(hline, "   SCROLL MODE    U=up D=down F=page-forward B=page-backward ESC=exit                    ");
  if (b_st->xs < 127) hline[b_st->xs] = 0;
  wprintf(b_st, hline);
  wredraw(b_st, 1);
  wflush();

  /* And do the job. */
  y = us->histlines;

  drawhist(b_us, y, 0);

  while((c = wxgetch()) != K_ESC) {
	switch(c) {
	  case 'u':
	  case K_UP:
		if (y <= 0) break;
		y--;
		wscroll(b_us, S_DOWN);
		wdrawelm(b_us, 0, getline_elm(b_us, y));
		wflush();
		break;
	  case 'd':
	  case K_DN:
		if (y >= us->histlines) break;
		y++;
		wscroll(b_us, S_UP);
		wdrawelm(b_us, b_us->ys - 1, getline_elm(b_us, y + b_us->ys - 1));
		wflush();
		break;
	  case 'b':
	  case K_PGUP:
		if (y <= 0) break;
		y -= b_us->ys;
		if (y < 0) y = 0;
		drawhist(b_us, y, 1);
		break;
	  case 'f':
	  case K_PGDN:
		if (y >= us->histlines) break;
		y += b_us->ys;
		if (y > us->histlines) y = us->histlines;
		drawhist(b_us, y, 1);
		break;
	}
  }
  /* Cleanup. */
  wclose(b_us, y == us->histlines ? 0 : 1);
  wclose(b_st, 1);
  wlocate(us, us->curx, us->cury);
  wflush();
}
#endif

#ifdef SIGWINCH
/* The window size has changed. Re-initialize. */
static void change_size(sig)
int sig;
{
  (void)sig;
  size_changed = 1;
  signal(SIGWINCH, change_size);
}
#endif

/*
 * Read a word from strings 's' and advance pointer.
 */
static char *getword(s)
char **s;
{
  char *begin;

  /* Skip space */
  while(**s == ' ' || **s == '\t') (*s)++;
  /* End of line? */
  if (**s == '\0' || **s == '\n') return((char *)0);
  
  begin = *s;
  /* Skip word */
  while(**s != ' ' && **s != '\t' && **s != '\n' && **s) (*s)++;
  /* End word with '\0' */
  if (**s) {
  	**s = 0;
  	(*s)++;
  }
  return(begin);
}

static char *bletch = 
  "Usage: remote [-somlz] [-c on] [-a on] [-t TERM] [-d entry] [-p ttyp] [configuration]\n";

static void usage(env_args, optind, mc)
int env_args, optind;
char *mc;
{
  if (env_args >= optind && mc)
  	fprintf(stderr, "Wrong option in environment REMOTE=%s\n", mc);
  fprintf(stderr, bletch);
  fprintf(stderr, "Type \"remote -h\" for help.\n");
  exit(1);
}

/* Give some help information */
static void helpthem()
{
  char *mc = getenv("REMOTE");

  printf("\n%s", Version);
#ifdef __DATE__
  printf(" (compiled %s)", __DATE__);
#endif
  printf("(c) Miquel van Smoorenburg\n\n%s\n", bletch);
  printf("  -s             : enter setup mode (only as user remote)\n");
  printf("  -o             : do not initialize lockfiles at startup\n");
  printf("  -m             : use meta or alt key for commands\n");
  printf("  -l             : literal ; assume screen uses the IBM-PC character set\n");
  /* printf("  -L             : Ditto, but assume screen uses ISO8859\n"); */
  printf("  -z             : try to use terminal's status line\n");
  printf("  -c [on | off]  : ANSI style color usage on or off\n");
  printf("  -a [on | off]  : use reverse or highlight attributes on or off\n");
  printf("  -t term        : override TERM environment variable\n");
  printf("  -p ttyp..      : connect to pseudo terminal\n");
  printf("  configuration  : configuration file to use\n\n");
  printf("These options can also be specified in the REMOTE environment variable.\n");
  printf("This variable is currently %s%s.\n", mc ? "set to " : "unset",
	mc ? mc : "");
  printf("The LIBDIRREAL to find the configuration files and the\n");
  printf("access file remote.users is compiled as %s.\n\n", LIBDIRREAL);

#if 0  /* More than 24 lines.. */
  printf("\
This program is free software; you can redistribute it and/or\n\
modify it under the terms of the GNU General Public License\n\
as published by the Free Software Foundation; either version\n\
2 of the License, or (at your option) any later version.\n\n");
#endif

}

int main(argc, argv)
int argc;
char **argv;
{
  struct sockaddr_un server;    /* for -x parameter */
  int c;			/* Command character */
  int quit = 0;			/* 'q' or 'x' pressed */
  char *s, *bufp;		/* Scratch pointers */
  int dosetup = 0, doinit = 1;	/* -o and -s options */
  char buf[80];			/* Keyboard input buffer */
  char capname[128];		/* Name of capture file */
  struct passwd *pwd;		/* To look up user name */
  int userok = 0;		/* Scratch variables */
  FILE *fp;			/* Scratch file pointer */
  char userfile[256];		/* Locate user file */
  char *use_port;		/* Name of initialization file */
  char *args[20];		/* New argv pointer */
  int argk = 1;			/* New argc */
  extern int getopt(), optind;
  extern char *optarg;		/* From getopt (3) package */
  extern int use_status;	/* Use status line. */
  char *mc;			/* For 'REMOTE' env. variable */
  int env_args;			/* Number of args in env. variable */
  char *cmd_dial;		/* Entry from the command line. */
  int alt_code;			/* Type of alt key */
  char pseudo[64];
  
  
  readlink("/proc/self/exe", LIBDIRREAL, 100); //everything runs from the build dir
  char * lastSlash = strrchr(LIBDIRREAL, '/');
  *(lastSlash + 1) = 0;

  /* Initialize global variables */
  portfd = -1;
  isSocket = 0; /* -- */
  parfile[0] = '\0';
  capfp = (FILE *)NULL;
  docap = 0;
  online = -1;
  stdattr = XA_NORMAL;
  us = NIL_WIN;
  addlf = 0;
#ifdef _SELECT
  local_echo = 0;
#endif
  strcpy(capname, "remote.cap");
  lockfile[0] = 0;
  tempst = 0;
  st = NIL_WIN;
  us = NIL_WIN;
  bogus_dcd = 0;
  usecolor = 0;
  screen_ibmpc = 0;
  screen_iso = 0;
  useattr = 1;
  strcpy(termtype, getenv("TERM") ? getenv("TERM") : "dumb");
  stdattr = XA_NORMAL;
  use_port = "dfl";
  alt_override = 0;
  scr_name[0] = 0;
  scr_user[0] = 0;
  scr_passwd[0] = 0;
  dial_name = (char *)NULL;
  dial_number = (char *)NULL;
  size_changed = 0;
  escape = 1;
  cmd_dial = NULL;
  real_uid = getuid();
  real_gid = getgid();
  eff_uid  = geteuid();
  eff_gid  = getegid();
#ifdef __CYGWIN__
  remote_uid = 0;
#else
  if (getpwnam("remote") == NULL)
    remote_uid = 0;
  else 
    remote_uid = getpwnam("remote")->pw_uid;
#endif

/* fmg 1/11/94 colors (set defaults) */
/* MARK updated 02/17/95 to be more similiar to TELIX */
  mfcolor = YELLOW;
  mbcolor = BLUE;
  tfcolor = WHITE;
  tbcolor = BLACK;
  sfcolor = WHITE;
  sbcolor = RED;
  st_attr = XA_NORMAL;
  
/* MARK updated 02/17/95 default history buffer size */
  num_hist_lines = 256;

/* fmg - but we reset these to F=WHITE, B=BLACK if -b flag found */

  /* Before processing the options, first add options
   * from the environment variable 'REMOTE'.
   */
  args[0] = "remote";
  if ((mc = getenv("REMOTE")) != CNULL) {
 	strncpy(buf, mc, 80);
 	bufp = buf;
 	buf[79] = 0;
 	while(isspace(*bufp)) bufp++;
 	while(*bufp) {
 		for(s = bufp; !isspace(*bufp) && *bufp; bufp++)
 			;
 		args[argk++] = s;
 		while(isspace(*bufp)) *bufp++ = 0;
 	}
  }
  env_args = argk;

  /* Add command - line options */
  for(c = 1; c < argc; c++) args[argk++] = argv[c];
  args[argk] = CNULL;

  do {
	/* Process options with getopt */
	while((c = getopt(argk, args, "zhlsomMbc:a:t:d:p:g:w:x:")) != EOF) switch(c) {
  		case 's': /* setup */
  			if (real_uid != remote_uid && real_uid != eff_uid) {
		fprintf(stderr, "remote: -s switch needs user remote privilige\n");
				exit(1);
			}
  			dosetup = 1;
  			break;
		case 'h':
			helpthem();
			exit(1);
			break;
		case 'p': /* Pseudo terminal to use. */
			if (strncmp(optarg, "/dev/", 5) == 0)
				optarg += 5;
			if (strncmp(optarg, "tty", 3) != 0 ||
			    !strchr("pqrstuvwxyz", optarg[3])) {
				fprintf(stderr, "remote: argument to -p must be a pty\n");
				exit(1);
			}
			sprintf(pseudo, "/dev/%s", optarg);
			dial_tty = pseudo;

			/* Drop priviliges. */
			drop_all_privs();
			break;
  		case 'm': /* ESC prefix metakey */
  			alt_override++;
			alt_code = 27;
			break;
		case 'M': /* 8th bit metakey. */
  			alt_override++;
			alt_code = 128;
  			break;
		case 'l': /* Literal ANSI chars */
			screen_ibmpc++;
			break;
		case 'L': /* Literal ISO8859 chars */
			screen_iso++;
			break;
		case 't': /* Terminal type */
			strcpy(termtype, optarg);
#ifdef __linux__
			/* Bug in older libc's (< 4.5.26 I think) */
			if ((s = getenv("TERMCAP")) != NULL && *s != '/')
				unsetenv("TERMCAP");
#endif
			break;
  		case 'o': /* DON'T initialize */
 	 		doinit = 0;
  			break;
  		case 'c': /* Color on/off */
  			if (strcmp("on", optarg) == 0) {
  				usecolor = 1;
  				stdattr = XA_BOLD;
  				break;
  			}
  			if (strcmp("off", optarg) == 0) {
  				usecolor = 0;
  				stdattr = XA_NORMAL;
  				break;
  			}
  			usage(env_args, optind - 1, mc);
  			break;
  		case 'a': /* Attributes on/off */
  			if (strcmp("on", optarg) == 0) {
  				useattr = 1;
  				break;
  			}
  			if (strcmp("off", optarg) == 0) {
  				useattr = 0;
  				break;
  			}
  			usage(env_args, optind - 1, mc);
  			break;
		case 'd': /* Dial from the command line. */
			cmd_dial = optarg;
			break;
		case 'z': /* Enable status line. */
			use_status = 1;
			break;

			/* -- */
	case 'g': /* -- allows a different parfile to be specified */
	  sprintf( parfile, "%s", optarg );
	  break;

	case 'w': /* -- gives us a socket id for use with wrampsim */
	  portfd = atoi( optarg );
	  isSocket = 1;
	  break;

	case 'x':
	  
	  portfd = socket(AF_UNIX, SOCK_STREAM, 0);
	  server.sun_family = AF_UNIX;
	  sprintf( server.sun_path, "%s", optarg );
	  
	  if (connect(portfd, (struct sockaddr *) &server, 
		      sizeof(struct sockaddr_un))    < 0) 
	    {
	      close( portfd );
	      fprintf( stderr, "Error: connecting stream socket, path: %s\n",
		       server.sun_path );
	      exit(-1);
	    }

	  isSocket = 1;
	  break;

  		default:
  			usage(env_args, optind, mc);
  			break;
  	}

  	/* Now, get portname if mentioned. Stop at end or '-'. */
 	while(optind < argk && args[optind][0] != '-')
  		use_port = args[optind++];

    /* Loop again if more options */
  } while(optind < argk);

  if( isSocket )
    {
      if( read( portfd, NULL, 0 ) < 0 )
	{
	  fprintf( stderr, "error: portfd is giving an error.\n"
		   "errno = %d | 0x%x\n", errno, errno );
	  sleep( 2 );
	  exit( -4 );
	}
    }

  if (real_uid == remote_uid && dosetup == 0) {
	fprintf(stderr, "%s%s%s",
  "remote: WARNING: please don't run remote as user remote when not maintaining\n",
  "                  it (with the -s switch) since all changes to the\n",
  "                  configuration will be GLOBAL !.\n");
	sleep(5);
  }

  /* set the parfile if it is not already set */
  if( parfile[0] == '\0' )
    {
      /* Avoid fraude ! */	
      for(s = use_port; *s; s++) if (*s == '/') *s = '_';
      sprintf(parfile, "%sremoterc.%s", LIBDIRREAL, use_port);

	  if( access( parfile, F_OK ) != -1 ) {
		fprintf(stderr, "File exists %s\n", parfile);
	  }
	  else {
		  fprintf(stderr, "File does not exist '%s'\n", parfile);
		  sprintf(parfile, "/etc/remoterc.%s", use_port);

		  fprintf(stderr, "trying '%s'\n", parfile );
		   if( access( parfile, F_OK ) != -1 ) {
			   fprintf(stderr, "File exists\n");	
		   }

	  }
    }
  /* Get password file information of this user. */
  if ((pwd = getpwuid(real_uid)) == (struct passwd *)0) {
  	fprintf(stderr, "You don't exist. Go away.\n");
  	exit(1);
  }

  /* Remember home directory and username. */
  if ((s = getenv("HOME")) == CNULL)
	strcpy(homedir, pwd->pw_dir);
  else
	strcpy(homedir, s);
  strcpy(username, pwd->pw_name);

  /* Get personal parameter file */
  sprintf(pparfile, "%s/.remoterc.%s", homedir, use_port);

  /* Check this user in the USERFILE */
  if (real_uid != remote_uid && real_uid != eff_uid) {
  	sprintf(userfile, "%s/remote.users", LIBDIRREAL);
	if ((fp = fopen(userfile, "r")) != (FILE *)0) {
		while(fgets(buf, 70, fp) != CNULL && !userok) {
			/* Read first word */
			bufp = buf;
			s = getword(&bufp);
			/* See if the "use_port" matches */
			if (s && (!strcmp(pwd->pw_name, s) ||
				strcmp("ALL", s) == 0)) {
				if ((s = getword(&bufp)) == CNULL)
					userok = 1;
				else do {
					if (!strcmp(s, use_port)) {
						userok = 1;
						break;
					}
				} while((s = getword(&bufp)) != CNULL);
			}
		}
		fclose(fp);
		if (!userok) {
			fprintf(stderr,
   "Sorry %s. You are not allowed to use configuration %s.\n",
				pwd->pw_name, use_port);
			fprintf(stderr, "Ask your sysadm to add your name to %s\n",
				userfile);
			exit(1);
		}
	}
  }
  buf[0] = 0;

  read_parms();
  if (dial_tty == NULL) dial_tty = P_PORT;

  stdwin = NIL_WIN; /* It better be! */

  /* Reset colors if we don't use 'em. */
  if (!usecolor) {
  	mfcolor = tfcolor = sfcolor = WHITE;
  	mbcolor = tbcolor = sbcolor = BLACK;
	st_attr = XA_REVERSE;
  }
  if (!dosetup && open_term(doinit) < 0) exit(1);

  mc_setenv("TERM", termtype);

  if (win_init(tfcolor, tbcolor, XA_NORMAL) < 0) leave("");

  if (COLS < 40 || LINES < 10) {
  	leave("Sorry. Your screen is too small.\n");
  }

  if (dosetup) {
  	if (config(1)) {
  		wclose(stdwin, 1);
  		exit(0);
  	}
  	if (open_term(doinit) < 0) exit(1);
  }

  /* Signal handling */
  for(c = 1; c <= _NSIG; c++) {
#ifdef SIGCLD /* Better not mess with those */
	if (c == SIGCLD) continue;
#endif
#ifdef SIGCHLD
	if (c == SIGCHLD) continue;
#endif
#ifdef SIGCONT
	if (c == SIGCONT) continue;
#endif
	signal(c, hangsig);
  }

#if 0 /* Is this OK? */
  signal(SIGHUP, SIG_IGN);
#endif
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

#ifdef SIGTSTP
  signal(SIGTSTP, shjump);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTINT
	signal(SIGTINT, SIG_IGN);
#endif
#ifdef SIGWINCH
  signal(SIGWINCH, change_size);
#endif

#if DEBUG
  for(c = 1; c < _NSIG; c++) {
	if (c == SIGTERM) continue; /* Saviour when hung */
	signal(c, signore);
  }
#endif

  keyboard(KINSTALL, 0);

  if (strcmp(P_BACKSPACE, "BS") != 0) 
	keyboard(KSETBS, P_BACKSPACE[0] == 'B' ? 8 : 127);
  if (alt_override)
	keyboard(KSETESC, alt_code);
  else if (strcmp(P_ESCAPE, "^A") != 0) {
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
  }

  st = NIL_WIN;
  us = NIL_WIN;

  init_emul(VT100, 1);
 
  wputs(us, CR_VERSION);
  wprintf(us, "\nPress %sZ for help on special keys\r\n\n", esc_key());

#if 0
  /* Now that all initialization is done, drop our priviliges. */
  drop_privs();
#endif

  readdialdir();

  if (cmd_dial) dialone(cmd_dial);

  /* The main loop calls do_terminal and gets a function key back. */
  while(!quit) {
	c = do_terminal();
dirty_goto:	
	switch(c + 32 *(c >= 'A' && c <= 'Z')) {
		case 'a': /* Add line feed */
			addlf = !addlf;
			vt_set(addlf, -1, NULL, -1, -1, -1, -1);
			s = addlf ? "Add linefeed ON" : "Add linefeed OFF";
			werror(s);
			break;
#if _SELECT
		case 'e': /* Local echo on/off. */
			local_echo = !local_echo;
			vt_set(-1, -1, NULL, -1, -1, local_echo, -1);
			s = local_echo ? "Local echo ON" : "Local echo OFF";
			werror(s);
			break;
#endif
		case 'z': /* Help */
			c = help();
			if (c != 'z') goto dirty_goto;
			break;
		case 'c': /* Clear screen */
			winclr(us);
			break;	
		case 'f': /* Send break */
			sendbreak();
			break;
#if HISTORY
		case 'b': /* Scroll back */
			scrollback();
			break;
#endif
#if 0
		case 'm': /* Initialize modem */
			modeminit();
			break;	
#endif
		case 'x': /* Exit Remote */
		case 'q': /* Exit Remote */
			c = ask("Leave Remote?", c1);
			if (c == 0) {
				quit = RESET;
			}
#if _HAVE_MACROS
			if (!strcmp(P_MACCHG,"CHANGED")) {
				c = ask ("Save macros?",c1);
				if (c == 0)
					if (dodflsave() < 0) /* fmg - error */
					{
						c = 'O'; /* hehe */
						quit = 0;
						goto dirty_goto;
					}
			}
#endif
			break;
		case 'l': /* Capture file */
			if (capfp == (FILE *)0 && !docap) {
				s = input("Capture to which file? ", capname);
				if (s == CNULL || *s == 0) break;
				if ((capfp = sfopen(s, "a")) == (FILE *)NULL) {
					werror("Cannot open capture file");
					break;
				}
				docap = 1;
			} else if (capfp != (FILE *)0 && !docap) {
				c = ask("Capture file", c3);
				if (c == 0) {
					fclose(capfp);
					capfp = (FILE *)NULL;
					docap = 0;
				}
				if (c == 1) docap = 1;
			} else if (capfp != (FILE *)0 && docap) {
				c = ask("Capture file", c2);
				if (c == 0) {
					fclose(capfp);
					capfp = (FILE *)NULL;
					docap = 0;
				}
				if (c == 1) docap = 0;
			}
			vt_set(addlf, -1, capfp, docap, -1, -1, -1);
			break;
		case 'p': /* Set parameters */
			get_bbp(P_BAUDRATE, P_BITS, P_PARITY, 0);
			port_init();
			if (st != NIL_WIN) mode_status();
			quit = 0;
			break;
		case 'k': /* Run kermit */
			kermit();
			break;
		case 'h': /* Hang up */
			do_hang(1);
			break;
		case 'd': /* Dial */
			dialdir();
			break;		
		case 't': /* Terminal emulation */
			c = dotermmenu();
			if (c > 0) init_emul(c, 1);
			break;
		case 'w': /* Line wrap on-off */	
			c = (!us->wrap);
			vt_set(addlf, c, capfp, docap, -1, -1, -1);
			s = c ? "Linewrap ON" : "Linewrap OFF";
			werror(s);
			break;
		case 'o': /* Configure Remote */
			(void) config(0);
			break;
		case 's': /* Upload */
			updown('U', 0);
			break;
		case 'r': /* Download */
			updown('D', 0);
			break;	
		case 'j': /* Jump to a shell */
			shjump(0);
			break;
		case 'g': /* Run script */	
			runscript(1, "", "", "");
			break;
		case 'i': /* Re-init, re-open portfd. */
			cursormode = (cursormode == NORMAL) ? APPL : NORMAL;
			keyboard(cursormode == NORMAL ? KCURST : KCURAPP, 0);
			break;
		default:
			break;
	}
  };

  /* -- not if its a socket */
  if( !isSocket )
    {
      /* Reset parameters */
      if (quit != NORESET)
	m_restorestate(portfd);
      else
	m_hupcl(portfd, 0);
    }

  if (capfp != (FILE *)0) fclose(capfp);
  wclose(us, 0);
  wclose(st, 0);
  wclose(stdwin, 1);
  set_privs();
  keyboard(KUNINSTALL, 0);
  if (lockfile[0]) unlink(lockfile);
  close(portfd);
  /* Please - if your system doesn't have uid_t and/or gid_t, define 'em
   * conditionally in "port.h".
   */
  chown(dial_tty, (uid_t)portuid, (gid_t)portgid);
  if (quit != NORESET && P_CALLIN[0])
	(void) fastsystem(P_CALLIN, CNULL, CNULL, CNULL);
  return(0);
}
