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
 * // fmg 12/20/93 - kludged in color "support" (hey, it works)
 * // fmg 2/15/94 - added 9 x MAC_LEN char macros for F1 to F10 which can be
 *                  save to a specified file so that old defaults file
 *                  works with these patches. TODO: make these alloc
 *                  memory dynamically... it's nice to have a 15K macro
 *                  _WHEN_ it's being (not like now with -DMAC_LEN) :-)
 *
 */
#include "port.h"
#include "remote.h"

#define ADM_CHANGE	1
#define USR_CHANGE	2

#if _HAVE_MACROS
/* fmg macros stuff */
#define MAX_MACS        10       /* fmg - header files? what's that... */
struct macs mmacs[] = {
  { "",       PUBLIC,   "pmac1" },
  { "",       PUBLIC,   "pmac2" },
  { "",       PUBLIC,   "pmac3" },
  { "",       PUBLIC,   "pmac4" },
  { "",       PUBLIC,   "pmac5" },
  { "",       PUBLIC,   "pmac6" },
  { "",       PUBLIC,   "pmac7" },
  { "",       PUBLIC,   "pmac8" },
  { "",       PUBLIC,   "pmac9" },
  { "",       PUBLIC,   "pmac10" },

  /* That's all folks */

  { "",                 0,         (char *)NULL },
};
#endif

struct pars mpars[] = {
  /* Protocols */
  /* Warning: remote assumes the first 12 entries are these proto's ! */
  { "YUNYzmodem",	PUBLIC,   "pname1" },
  { "YUNYymodem",	PUBLIC,   "pname2" },
  { "YUNYxmodem",	PUBLIC,   "pname3" },
  { "NDNYzmodem",	PUBLIC,   "pname4" },
  { "NDNYymodem",	PUBLIC,   "pname5" },
  { "YDNYxmodem",	PUBLIC,   "pname6" },
  { "YUYNkermit",	PUBLIC,   "pname7" },
  { "NDYNkermit",	PUBLIC,   "pname8" },
  { "",			PUBLIC,   "pname9" },
  { "",			PUBLIC,   "pname10" },
  { "",			PUBLIC,   "pname11" },
  { "",			PUBLIC,   "pname12" },
#ifdef __linux__
  { "/usr/bin/sz -vv",	PUBLIC,   "pprog1" },
  { "/usr/bin/sb -vv",	PUBLIC,   "pprog2" },
  { "/usr/bin/sx -vv",	PUBLIC,   "pprog3" },
  { "/usr/bin/rz -vv",	PUBLIC,   "pprog4" },
  { "/usr/bin/rb -vv",	PUBLIC,   "pprog5" },
  { "/usr/bin/rx -vv",	PUBLIC,   "pprog6" },
  { "/usr/bin/kermit -i -l %l -s", PUBLIC, "pprog7" },
  { "/usr/bin/kermit -i -l %l -r", PUBLIC, "pprog8" },
#else
  /* Most sites have this in /usr/local, except Linux. */
  { "/usr/local/bin/sz -vv",	PUBLIC,   "pprog1" },
  { "/usr/local/bin/sb -vv",	PUBLIC,   "pprog2" },
  { "/usr/local/bin/sx -vv",	PUBLIC,   "pprog3" },
  { "/usr/local/bin/rz -vv",	PUBLIC,   "pprog4" },
  { "/usr/local/bin/rb -vv",	PUBLIC,   "pprog5" },
  { "/usr/local/bin/rx -vv",	PUBLIC,   "pprog6" },
  { "/usr/local/bin/kermit -i -l %l -s", PUBLIC, "pprog7" },
  { "/usr/local/bin/kermit -i -l %l -r", PUBLIC, "pprog8" },
#endif
  { "",			PUBLIC,   "pprog9" },
  { "",			PUBLIC,   "pprog10" },
  { "",			PUBLIC,   "pprog11" },
  { "",			PUBLIC,   "pprog12" },
  /* Serial port & friends */
  { DFL_PORT,		PUBLIC,  "port" },
  { CALLIN,		PRIVATE,  "callin" },
  { CALLOUT,		PRIVATE,  "callout" },
  { UUCPLOCK,		PRIVATE,  "lock" },
  { DEF_BAUD,		PRIVATE,   "baudrate" },
  { "8",		PRIVATE,   "bits" },
  { "N",		PRIVATE,   "parity" },
  /* Kermit the frog */
  { KERMIT,		PRIVATE,  "kermit" },
  { "Yes",		PRIVATE,  "kermallow" },
  { "No",		PRIVATE,  "kermreal" },
  { "3",		PUBLIC,   "colusage" },
  /* The script program */
  { "",			PUBLIC,   "scriptprog" },
  /* Modem parameters */
  { "~^M~AT S7=45 S0=0 L1 V1 X4 &c1 E1 Q0^M",   PUBLIC,   "minit" },
  { "^M~ATZ^M~",	PUBLIC,   "mreset" },
  { "ATDT",		PUBLIC,   "mdialpre" },
  { "^M",		PUBLIC,   "mdialsuf" },
  { "ATDP",		PUBLIC,   "mdialpre2" },
  { "^M",		PUBLIC,   "mdialsuf2" },
  { "ATX1DT",		PUBLIC,   "mdialpre3" },
  { ";X4D^M",		PUBLIC,   "mdialsuf3" },
  { "CONNECT",		PUBLIC,   "mconnect" },
  { "NO CARRIER",	PUBLIC,   "mnocon1" },
  { "BUSY",		PUBLIC,   "mnocon2" },
  { "NO DIALTONE",	PUBLIC,   "mnocon3" },
  { "VOICE",		PUBLIC,   "mnocon4" },
  { "~~+++~~ATH^M",	PUBLIC,   "mhangup" },
  { "^M",		PUBLIC,   "mdialcan" },
  { "45",		PUBLIC,   "mdialtime" },
  { "5",		PUBLIC,   "mrdelay" },
  { "10",		PUBLIC,   "mretries" },
  { "No",		PUBLIC,   "mautobaud" },
  { "Yes",		PUBLIC,   "mdropdtr" },
  { "",			PUBLIC,   "updir" },
  { "",			PUBLIC,   "downdir" },
  { "",			PUBLIC,   "scriptdir" },
  { "^A",		PUBLIC,   "escape-key" },
  { "DEL",		PUBLIC,   "backspace" },
  { "enabled",		PUBLIC,   "statusline" },
  { "Yes",		PUBLIC,   "hasdcd" },
  { "Yes",		PUBLIC,   "rtscts" },
  { "No",		PUBLIC,   "xonxoff" },
  { "D",		PUBLIC,   "zauto" },

  /* fmg 1/11/94 colors */
  /* MARK updated 02/17/95 to be more like TELIX. After all its configurable */

  { "YELLOW",           PUBLIC,   "mfcolor" },
  { "BLUE",             PUBLIC,   "mbcolor" },
  { "WHITE",            PUBLIC,   "tfcolor" },
  { "BLACK",            PUBLIC,   "tbcolor" },
  { "WHITE",            PUBLIC,   "sfcolor" },
  { "RED",              PUBLIC,   "sbcolor" },

  /* fmg 2/20/94 macros */

  { ".macros",          PUBLIC,   "macros" },
  { "",                 PUBLIC,   "changed" },
  { "Yes",		PUBLIC,   "macenab" },

  /* Continue here with new stuff. */
  { "Yes",		PUBLIC,   "sound"  },

  /* MARK updated 02/17/95 - History buffer size */
  { "256",             PUBLIC,    "histlines" },

  /* That's all folks */
  { "",                 0,         (char *)NULL },
};
 
#if _HAVE_MACROS
/*
 * fmg - Write the macros to a file.
 */
int writemacs(fp, all)
FILE *fp;
int all;
{
  struct macs *m;

  for(m = mmacs; m->desc; m++)
        if ((all && (m->flags & ADM_CHANGE))
        || ((m->flags & PUBLIC) && (m->flags & USR_CHANGE)))
                fprintf(fp, "%s %-16.16s %s\n", m->flags &
                        PUBLIC ? "pu" : "pr", m->desc, m->value);
  return(0);
}
#endif

/*
 * Write the parameters to a file.
 */
int writepars(fp, all)
FILE *fp;
int all;
{
  struct pars *p;

  if (all)
	fprintf(fp, "# Machine-generated file - use \"remote -s\" to change parameters.\n");
  else
	fprintf(fp, "# Machine-generated file - use setup menu in remote to change parameters.\n");

  for(p = mpars; p->desc; p++)
  	if ((all && (p->flags & ADM_CHANGE)) ||
  	   ((p->flags & PUBLIC) && (p->flags & USR_CHANGE)))
  		fprintf(fp, "%s %-16.16s %s\n",
  			p->flags & PUBLIC ? "pu" : "pr", p->desc, p->value);
  return(0);
}

/*
 * Read the parameters from a file.
 */
int readpars(fp, init)
FILE *fp;
int init;
{
  struct pars *p;
  char line[300];
  char *s;
  int public;
  int dosleep = 0;

  if (init) strcpy(P_SCRIPTPROG, "runscript");

  while(fgets(line, 300, fp) != (char *)0) {

  	s = strtok(line, "\n\t ");
	if (s == NULL) continue;

  	/* Here we have pr for private and pu for public */
  	public = -1;
  	if (strcmp(s, "pr") == 0) {
  		public = 0;
  		s = strtok((char *)NULL, "\n\t ");
  	}
  	if (strcmp(line, "pu") == 0) {
  		public = 1;
  		s = strtok((char *)NULL, "\n\t ");
  	}
	if (s == NULL || public < 0) 
           {
           continue;
           }

  	/* Don't read private entries if prohibited */
  	if (!init && public == 0) 
           {
           continue;
           }


  	for(p = mpars; p->desc != (char *)0; p++) {
  		if (strcmp(p->desc, s) == 0) {

			/* See if this makes sense. */
			if (((p->flags & PRIVATE) && public) ||
			    ((p->flags & PUBLIC)  && !public)) {
				fprintf(stderr,
			"** Parameter %s is %s, but is marked %s in %s config file\n",
					s,
					(p->flags & PRIVATE) ? "private" : "public",
					public ? "public" : "private",
					init ? "global" : "personal");
				dosleep = 1;
			}
			/* See if user is allowed to change this flag. */
			if ((p->flags & PRIVATE) && public == 1) {
				fprintf(stderr,
			"== Attempt to change private parameter %s - denied.\n",
					p->desc);
				dosleep = 1;
				continue;
			}

  			/* Set value */
  			if ((s = strtok((char *)NULL, "\n")) == (char *)0)
  				s = "";
  			while(*s && (*s == '\t' || *s == ' ')) s++;

  			/* If the same as default, don't mark as changed */
  			if (strcmp(p->value, s) == 0) {
  				p->flags = 0; 
  			} else {
				if (init)
					p->flags |= ADM_CHANGE;
				else
					p->flags |= USR_CHANGE;
  				strcpy(p->value, s);
  			}
#if 0 /* Ehm. This makes no sense, Mike. */
  			/* Set flags */
  			p->flags |= (public ? PUBLIC : PRIVATE);
  			p->flags &= ~(public ? PRIVATE : PUBLIC);
#endif
  			break;
  		}
  	}
  }
  /* To get over a bug in remote 1.60 :( */
  if (strcmp(P_BAUDRATE, "Curr") == 0) strcpy(P_BAUDRATE, "38400");

  if (dosleep) sleep(3);

  return(0);
}

#if _HAVE_MACROS
/*
 * fmg - Read the macros from a file.
 */
int readmacs(fp, init)
FILE *fp;
int init;
{
  struct        macs *m;
  char          line[MAC_LEN];
  int           public, max_macs=MAX_MACS+1;
  char          *s;

  while(fgets(line, MAC_LEN, fp) != (char *)0 && max_macs--) {
        s = strtok(line, "\n\t ");
        /* Here we have pr for private and pu for public */
        public = 0;
        if (strcmp(s, "pr") == 0) {
                public = 0;
                s = strtok((char *)NULL, "\n\t ");
        }
        if (strcmp(line, "pu") == 0) {
                public = 1;
                s = strtok((char *)NULL, "\n\t ");
        }
        /* Don't read private entries if prohibited */
        if (!init && public == 0) continue;

        for(m = mmacs; m->desc != (char *)0; m++) {
                if (strcmp(m->desc, s) == 0) {
                                ;
                        /* Set value */
                        if ((s = strtok((char *)NULL, "\n")) == (char *)0)
                                s = "";
                        while(*s && (*s == '\t' || *s == ' ')) s++;

                        /* If the same as default, don't mark as changed */
                        if (strcmp(m->value, s) == 0) {
                                m->flags = 0;
                        } else {
                                if (init)
                                        m->flags |= ADM_CHANGE;
                                else
                                        m->flags |= USR_CHANGE;
                                strcpy(m->value, s);
                        }
#if 0
                        /* Set flags */
                        m->flags |= (public ? PUBLIC : PRIVATE);
#endif
                        break;
                }
        }
  }
  return(0);
}
#endif
