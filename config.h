/*
 * config.h	Default configuration.
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1996 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

/*
 * Definitions below are not hard-coded - you can change them from
 * the setup menu in minicom, or you can start minicom with the
 * "-s" flag.
 * Recommended setting for some systems are commented. Uncomment
 * and adjust them to your system.
 */

/* Operating system INdependant parameters. (Usually the same everywhere) */
#define KERMIT "/usr/bin/kermit -l %l -b %b"	/* How to call kermit */
#define UUCPLOCK	"/var/spool/uucp"	/* Lock file directory */
#define LOGFILE		"remote.log"		/* Not defined = not used */

#define MIN_LINES 2

/* Operating system dependant parameters, per OS. A few samples are given. */
#if defined(__linux__)
#  define DFL_PORT "/dev/cua1"		/* Which tty to use */
#  define DEF_BAUD "38400"		/* Default baud rate */
#  define CALLOUT  ""			/* Gets run to get into dial out mode */
#  define CALLIN   ""			/* Gets run to get into dial in mode */
#  undef  UUCPLOCK
#  define UUCPLOCK "/tmp"               /* /var/lock */	/* FSSTND 1.2 */
#endif

#if defined (_COHERENT)
#  define DFL_PORT "/dev/modem"
#  define DEF_BAUD "9600"
#  define CALLOUT  "" /* "/etc/disable com1r" */
#  define CALLIN   "" /* "/etc/enable com1r"  */
#endif

#ifdef _HPUX_SOURCE
#  define DFL_PORT "/dev/cua2p0"
#  define DEF_BAUD "19200"
#  define CALLOUT  ""
#  define CALLIN   ""
#endif

#ifdef ISC
#  define DFL_PORT "/dev/tty01"
#  define DEF_BAUD "9600"
#  define CALLOUT  ""
#  define CALLIN   ""
#endif

#ifdef __FreeBSD__
#  define DFL_PORT "/dev/modem"
#  define DEF_BAUD "19200"
#  define CALLOUT  ""
#  define CALLIN   ""
#endif

/* Some reasonable defaults if not defined */
#ifndef DFL_PORT
#  define DFL_PORT "/dev/tty8"
#  define DEF_BAUD "2400"
#  define CALLIN   ""
#  define CALLOUT  ""
#endif

/*
 * The next definitions are permanent ones - you can't edit the
 * configuration from within minicom to change them
 * (unless you use a binary editor, like a real hacker :-)
 */

/* Some reasonable defaults if not defined */
#ifndef DFL_PORT
#  define DFL_PORT "/dev/tty8"
#  define DEF_BAUD "2400"
#  define CALLIN   ""
#  define CALLOUT  ""
#endif

/*
 * The next definitions are permanent ones - you can't edit the
 * configuration from within minicom to change them
 * (unless you use a binary editor, like a real hacker :-)
 */

/* This defines a special mode in the wwxgetch() routine. The
 * basic idea behind this probably works on the consoles of
 * most PC-based unices, but it's only implemented for Linux.
 */
#if defined (__linux__)
#  define KEY_KLUDGE 1
#endif

/* Define this if you want a history buffer. */
#define HISTORY 1

/* And this for the translation tables (vt100 -> ASCII) */
#if __STDC__
#  define TRANSLATE 1
#  define CONST const
#else
#  define TRANSLATE 0
#  define CONST
#endif
