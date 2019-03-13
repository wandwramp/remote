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
#ifdef ISC
#  include <sys/bsdtypes.h>
#endif
#include <sys/types.h>
#if defined (_POSIX) || defined(_BSD43)
#  include <unistd.h>
#  include <stdlib.h>
#else
   char *getenv();
#endif
#if defined (__linux__)
#  include <termcap.h>
#else
  char *tgetstr(), *tgoto();
  int tgetent(), tputs(), tgetnum(), tgetflag();
#endif
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>
#include <sys/stat.h>
#ifdef _MINIX
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <errno.h>
#if defined(_BSD43) || defined(_SYSV)
#  define NOSTREAMS
#  include <sys/file.h>
#endif
#if defined(_COH42) || defined(_SEQUENT)
#  include <sys/select.h>
#endif

/* This one's for Coherent 3. What about Coherent 4 ? */
#ifdef _COH3
#  include <access.h>
#  define W_OK AWRITE
#  define R_OK AREAD
#endif

/* And for ancient systems like SVR2 */
#ifndef W_OK
#  define W_OK 2
#  define R_OK 4
#endif
#ifndef SEEK_SET
#  define SEEK_SET 0
#  define SEEK_END 2
#endif
#ifndef _NSIG
#  ifndef NSIG
#    define _NSIG 31
#  else
#    define _NSIG NSIG
#  endif
#endif

/* A general macro for prototyping. */
#ifndef _PROTO
#  if __STDC__
#    define _PROTO(fun, args) fun args
#  else
#  define _PROTO(fun, args) fun()
#  endif
#endif

#ifdef _COH42
#  define time_t long
#endif

/* Enable music routines. Could we use defined(i386) here? */
#if defined(__linux__) || defined(_COH42) || defined(_SCO)
#  ifdef _SELECT
#    define VC_MUSIC 1
#  endif
#endif

/* Availability of setreuid(uid_t, uid_t) */
#if defined(__linux__) || defined(_BSD43)
#  define HAS_REUID
#endif

/* Availability of fchown(int, uid_t, gid_t) */
#if defined (_SYSV) || defined (_BSD43) || defined(_DGUX_SOURCE)
#  if !defined(_SVR2) && !defined(_SYSV3) && !defined(_COH42)
#    define HAS_FCHOWN
#  endif
#endif
