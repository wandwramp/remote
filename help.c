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

/* Draw a help screen and return the keypress code. */
int help()
{
  WIN *w;
  int c;
  int i;
  int x1, x2;

#if HISTORY
  i = 1;
#else
  i = 0;
#endif
  x1 = (COLS / 2) - 34;
  x2 = (COLS / 2) + 32;
  w = wopen(x1, 3, x2, 19 + i, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);
  
  wlocate(w, 21, 0);
  wputs(w, "Command Summary");
  wlocate(w, 10, 2);

  wprintf(w, "Commands can be called by %s<key>", esc_key());

  wlocate(w, 0, 6);
  wputs(w, " Send files.........S  \263 cOnfigure..........O\n");
  wputs(w, " Quit...............Q  \263");
#ifdef SIGTSTP
  wputs(w, " Suspend............J\n");
#else
  wputs(w, "Jump to a shell....J\n");
#endif
  wputs(w, " Capture on/off.....L  \263 send break.........F\n");

  wputs(w, " lineWrap on/off....W  \263 scroll Back........B");

  wlocate(w, 10, 15 + i);
  wputs(w, "Modified from minicom");
  wlocate(w, 10 , 16 + i);
  wputs(w, "at the University of Waikato 1996");
  wlocate(w, 6, 12 + i);
  wputs(w, "Select function or press Enter for none.");
  wredraw(w, 1);

  c = wxgetch();
  wclose(w, 1);
  return(c);
}
