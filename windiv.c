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

/*
 * Popup a window and put a text in it.
 */  
/*VARARGS1*/
WIN *tell(s, a1, a2, a3, a4)
char *s, *a1, *a2, *a3, *a4;
{
  WIN *w;
  char buf[128];

  if (stdwin == NIL_WIN) return(NULL);

  sprintf(buf, s, a1, a2, a3, a4);

  w = wopen((COLS / 2) - 2 - strlen(buf) / 2, 8,
	    (COLS / 2) + 2 + strlen(buf) / 2, 10,
  	     BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  wcursor(w, CNONE);	
  wlocate(w, 2, 1);
  wputs(w, buf);
  wredraw(w, 1);
  return(w);
}

/*
 * Show an error message.
 */
/*VARARGS1*/
void werror(s, a1, a2, a3, a4)
char *s, *a1, *a2, *a3, *a4;
{
  WIN *tellwin;
  
  tellwin = tell(s, a1, a2, a3, a4);
  sleep(2);
  wclose(tellwin, 1);
}

/*
 * Vertical "wselect" function.
 */
int ask(what, s)
char *what;
char *s[];
{
  int num = 0;
  int cur = 0, ocur = 0;
  int f, c;
  WIN *w;
  int size, offs;

  for(f = 0; s[f]; f++) num++;

  size = 5 * num;
  offs = 0;
  if (strlen(what) > 2 * size + 4) {
	size = strlen(what) / 2 + 2;
	offs = size - 5*num;
  }
  w = wopen((COLS / 2) - size , 8, (COLS / 2) + 1 + size, 9,
		BSINGLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
	
  dirflush = 0;

  wcursor(w, CNONE);
  wlocate(w, 1 + size - (strlen(what) / 2), 0);
  wputs(w, what);

  for(f = 1; f < num; f++) {
  	wlocate(w, 2 + offs + 10*f, 1);
  	wputs(w, s[f]);
  }
  wredraw(w, 1);

  while(1) {
  	wlocate(w, 2 + offs + 10*cur, 1);
	if (!useattr)
		wprintf(w, ">%s", s[cur] + 1);
	else {
	  	wsetattr(w, XA_REVERSE | stdattr);
  		wputs(w, s[cur]);
	}
  	ocur = cur;
  	wflush();
  	switch(c = wxgetch()) {
  		case ' ':
  		case 27:
  		case 3:
  			dirflush = 1;
  			wclose(w, 1);
  			return(-1);
  		case '\r':
  		case '\n':
  			dirflush = 1;
  			wclose(w, 1);
  			return(cur);
  		case K_LT:
  		case 'h':
  			cur--;
  			if (cur < 0) cur = num - 1;
  			break;
  		default:
  			cur = (cur + 1) % num;
  			break;
  	}
  	wlocate(w, 2 + offs + 10*ocur, 1);
  	wsetattr(w, stdattr);
	if (!useattr)
		wputs(w, " ");
	else
  		wputs(w, s[ocur]);
  }
}

extern int editline();

/*
 * Popup a window and ask for input.
 */
char *input(s, buf)
char *s;
char *buf;
{
  WIN *w;

  w = wopen((COLS / 2) - 20, 11, (COLS / 2) + 20, 12,
		BDOUBLE, stdattr, mfcolor, mbcolor, 1, 0, 1);
  wputs(w, s);
  wlocate(w, 0, 1);
  wprintf(w, "> %-38.38s", buf);
  wlocate(w, 2, 1);
  if (wgets(w, buf, 38, 128) < 0) buf = CNULL;
  wclose(w, 1);
  return(buf);
}

#if 0
/* Here I mean to work on a file selection window. */
/* Maybe remote 1.70 - MvS. */

struct file {
  char *name;
  char isdir;
};
static struct filelist **list;
static int nrlist;

/* Compare two directory entries. */
static int cmpdir(d1, d2)
struct file *d1;
struct file *d2;
{
  if (strcmp(d1->name, "..")) return(-1);
  return(strcmp(d1->name, d2->name));
}

/* Sort the directory list. */
static int sortdir()
{
  qsort(list, nrlist, sizeof(struct file *), cmpdir);
}

/* Create file list of directory. */
static int makelist()
{
  DIR *dir;
  struct dirent *d;
  struct file **new;
  int left = 0;
  int f;

  if ((dir = opendir(".")) == NULL) {
	wbell();
	return(-1);
  }

  /* Free the old list if nessecary. */
  if (list) {
	for(f = 0; f < nrlist; f++) free(list[f]);
	free(list);
	list = NULL;
  }

  while(d = readdir(dir)) {

	/* Skip "." entry. */
	if (strcmp(d->d_name, ".") == 0) continue;

	/* Add this to our list. */
	if (left == 0) {
		/* Re-malloc. */
		if ((new = malloc(nrents + 10 * sizeof(void *))) == NULL) {
			closedir(dir);
			return(-1);
		}
		if (list) {
			memcpy(new, list, nrlist * sizeof(struct file **));
			free(list);
		}
		list = new;
		left = 10;
	}

	/* Stat this file. */
#ifdef S_IFLNK
	(void) lstat(d->d_name, &st);
#else
	(void) stat(d->d_name, &st);
#endif
	list[nrlist]->isdir = S_ISDIR(st.st_mode);
	f = 0;
	if (S_ISDIR(st.st_mode)) f = '/';
#ifdef S_ISLNK
	if (S_ISLNK(st.st_mode)) f = '@';
#endif
#ifdef S_ISFIFO
	if (S_ISFIFO(st.st_mode)) f = '|';
#endif
#ifdef S_ISSOCK
	if (S_ISSOCK(st.st_mode)) f = '=';
#endif
	if (S_ISREG(st.st_mode) && (st.st_mode & 0x111)) f = '*';

	/* Fill in name. */
	if ((list[nrlist]->name = malloc(strlen(d->d_name + 2))) == NULL) {
		closedir(dir);
		return(-1);
	}
	sprintf(list[nrlist]->name, "%s%c", d->d_name, f);
	nrlist++;
	left--;
  }
  closedir(dir);
  return(0);
}

/* Select a file. */
char *wfilesel()
{
  WIN *w;
  char cwd[64];

  /* Open one window. */
  w = wopen((COLS / 2) - 20, 5, (COLS / 2) + 20, 20,
		BDOUBLE, stdattr, mfcolor, mbcolor, 1, 0, 1);
  getcwd(cwd, 64);

  while(1) {
	makelist();
	sortdir();


#endif /* DEVEL */
