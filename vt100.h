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

/* Keypad and cursor key modes. */
#define NORMAL	1
#define APPL	2

/* Don't change - hardcoded in remote's dial.c */
#define VT100	1
#define ANSI	3

/* Prototypes from vt100.c */
_PROTO( void vt_install, (void(*)(), void (*)(), WIN *));
_PROTO( void vt_init, (int, int, int, int, int));
_PROTO( void vt_pinit, (WIN *, int, int));
_PROTO( void vt_set, (int, int, FILE *, int, int, int, int));
_PROTO( void vt_out, (int ));
_PROTO( void vt_send, (int ch));

