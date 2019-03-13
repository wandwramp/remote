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

static int not_suid = -1;

/*
 * A modified version of the getargs routine.
 */
static int getargs(s, arps, maxargs)
register char *s;
char *arps[];
int maxargs;
{
	register int i;
	register char *sp;
	register char qchar;
	int literal = 0;

	i = 0;
	while (i < maxargs) {
		while (*s == ' ' || *s == '\t')
			++s;
		if (*s == '\n' || *s == '\0')
			break;
		arps[i++] = sp = s;
		qchar = 0;
		while(*s != '\0'  &&  *s != '\n') {
			if (literal) {
				literal = 0;
				*sp++ = *s++;
				continue;
			}
			literal = 0;
			if (qchar == 0 && (*s == ' ' || *s == '\t')) {
				++s;
				break;
			}
			switch(*s) {
			default:
				*sp++ = *s++;
				break;
			case '\\':
				literal = 1;
				s++;
				break;	
			case '"':
			case '\'':
				if(qchar == *s) {
					qchar = 0;
					++s;
					break;
				}
				if(qchar)
					*sp++ = *s++;
				else
					qchar = *s++;
				break;
			}
		}
		*sp++ = 0;
	}
	if (i >= maxargs)
		return -1;
	arps[i] = (char *)0;
	return i;
}

/*
 * Is a character from s2 in s1?
 */
static int anys(s1, s2)
char *s1, *s2;
{
  while(*s2)
  	if (strchr(s1, *s2++) != (char *)NULL) return(1);
  return(0);
}

/*
 * If there is a shell-metacharacter in "cmd",
 * call a shell to do the dirty work.
 */
int fastexec(cmd)
char *cmd;
{
  char *words[128];
  char *p;

  if (anys(cmd, "~`$&*()=|{};?><"))
  	return(execl("/bin/sh", "sh", "-c", cmd, (char *)0));

  /* Delete escape-characters ment for the shell */
  p = cmd;
  while((p = strchr(p, '\\')) != (char *)NULL)
  	strcpy(p, p + 1);

  /* Split line into words */
  if (getargs(cmd, words, 127) < 0) {
  	return(-1);
  }
  return (execvp(words[0], words));
}

/*
 * Fork, then redirect I/O if neccesary.
 * in    : new stdin
 * out   : new stdout
 * err   : new stderr
 * Returns exit status of "cmd" on success, -1 on error.
 */
int fastsystem(cmd, in, out, err)
char *cmd, *in, *out, *err;
{
  int pid;
  int st;
  int async = 0;
  char *p;

  /* If the command line ends with '&', don't wait for child. */
  p = strrchr(cmd, '&');
  if (p != (char *)0 && !p[1]) {
  	*p = 0;
  	async = 1;
  }
  
  /* Fork. */
  if ((pid = fork()) == 0) { /* child */
  	if (in != (char *)NULL) {
  		close(0);
  		if (open(in, O_RDONLY) < 0) exit(-1);
  	}
  	if (out != (char *)NULL) {
  		close(1);
  		if (open(out, O_WRONLY) < 0) exit(-1);
  	}
  	if (err != (char *)NULL) {
  		close(2);
  		if (open(err, O_RDWR) < 0) exit(-1);
  	}
  	exit(fastexec(cmd));
  } else if (pid > 0) { /* parent */
  	if (async) return(0);
  	pid = m_wait(&st);
  	if (pid < 0) return(-1);
  	return(st);
  }
  return(-1);
}

/* Drop all priviliges (irreversable). */
void drop_all_privs()
{
#ifdef HAS_REUID
  /* Regain privs needed to drop privs :) */
  setregid(real_gid, eff_gid);
  setreuid(real_uid, eff_uid);
#endif

  /* Drop it. */
  setgid(real_gid);
  setuid(real_uid);
  eff_uid = real_uid;
  eff_gid = real_gid;
  not_suid = 1;
}

/* Drop priviliges (swap uid's) */
void drop_privs()
{
#ifdef HAS_REUID
  setregid(eff_gid, real_gid);
  if (setreuid(eff_uid, real_uid) < 0)
	fprintf(stderr, "remote: cannot setreuid(%d, %d)\n", eff_uid, real_uid);
  not_suid = 1;
#endif
}

/* Set priviliges (swap uid's) */
void set_privs()
{
#ifdef HAS_REUID
  setregid(real_gid, eff_gid);
  if (setreuid(real_uid, eff_uid) < 0)
	fprintf(stderr, "remote: cannot setreuid(%d, %d)\n", real_uid, eff_uid);
  not_suid = 0;
#endif
}

/* Safe fopen for suid programs. */
FILE *sfopen(file, mode)
char *file;
char *mode;
{
#ifdef HAS_REUID
  int saved_errno;
#else
  char *p, buf[128];
  struct stat stt;
  int do_chown = 0;
#endif
  FILE *fp;

  /* Initialize. */
  if (not_suid < 0) not_suid = (real_uid == eff_uid);

  /* If we're not running set-uid at the moment just open the file. */
  if (not_suid) return (fopen(file, mode));

#ifdef HAS_REUID
  /* Just drop priviliges before opening the file. */
  drop_privs();
  fp = fopen(file, mode);
  saved_errno = errno;
  set_privs();
  errno = saved_errno;
  return(fp);
#else

  /* Read access? */
  if (strcmp(mode, "r") == 0) {
	if (access(file, R_OK) < 0) return(NULL);
	return(fopen(file, mode));
  }

  /* Write access test. */
  if (stat(file, &stt) == 0) {
	if (access(file, W_OK) < 0) return(NULL);
  } else {
	/* Test if directory is writable. */
	strcpy(buf, file);
	if((p = strrchr(buf, '/')) == (char *)NULL)
  		strcpy(buf, ".");
	else
		*p = '\0';
	if (access(buf, W_OK) < 0) return(NULL);
	do_chown = 1;
  }

  /* Now open/create the file. */
  if ((fp = fopen(file, mode)) == NULL) return(fp);
  if (!do_chown) return(fp);

#ifndef HAS_FCHOWN
  /* There is a security hole / race condition here. */
  (void) chown(file, (uid_t)real_uid, (gid_t)real_gid);
#else
  /* But this is safe. */
  (void) fchown(fileno(fp), (uid_t)real_uid, (gid_t)real_gid);
#endif /* HAS_FCHOWN */
  return(fp);
#endif /* HAS_REUID */
}

