#
# Makefile	Makefile for Minicom on various systems.
#		You have to pick your system here. Just
#		uncomment it and let it compile.
#
# Version	@(#) Makefile 1.73 (18-Jan-1996) MvS
#
# Bugs		The Linux, Coherent 4.2, Sun and BSD versions
#		have been tested and are known to work.
#		For others, your mileage may vary...
#
#

# Where to install things for Linux sites (FSSTND)
BINDIR	= /home/201/bin
LIBDIR	= $(BINDIR)
MANDIR	= $(BINDIR)

# Where to install things for "normal" sites.
# BINDIR	= /usr/local/bin
# LIBDIR	= /usr/local/lib
# DOCDIR	= /usr/local/lib
# MANDIR	= /usr/local/man/man1

# Take these compilation flags for Linux.
#FLAGS	= -Wall -D_POSIX -D_SYSV -D_SELECT -D_HAVE_MACROS
#PROGS	= minicom runscript
#LFLAGS	= -s
#LIBS	= -ltermcap
#CC	= cc

# Take these compilation flags for Linux with ncurses.
FLAGS	= -Wall -D_POSIX -D_SYSV -D_SELECT -D_HAVE_MACROS # -I/usr/include/ncurses
PROGS	= remote runscript down
LFLAGS	= -s
LIBS	= -lncurses
CC	= gcc

# Take these compilation flags for FreeBSD.
#FLAGS	= -Wall -D_POSIX -D_BSD43 -D_SELECT -D_HAVE_MACROS -D_DCDFLOW
#PROGS	= minicom runscript
#LFLAGS	= -s
#LIBS	= -ltermcap
#CC	= cc

# Take these flags for SCO unix.
#FLAGS	= -D_SYSV -D_SCO -D_POSIX -D_SELECT -D_HAVE_MACROS
#PROGS	= minicom runscript
#LFLAGS	= -s
#LIBS	= -lcurses
#CC	= cc

# These flags are for BSD 4.3 alike systems. Tested on SunOs 4.1.3.
# Used to compile under other flavors of BSD4.3 too..
#LFLAGS	= # -s
#FLAGS	= -D_BSD43 -D_SELECT				# BSD style ioctls
#FLAGS	= -D_POSIX -D_BSD43 -D_SELECT -D_DCDFLOW	# BSD with Posix termios
#FLAGS	= -D_POSIX -D_BSD43 -D_SELECT -DSUN -D_DCDFLOW	# Sun include nightmare
#PROGS	= minicom runscript
#LIBS	= -ltermcap
#CC	= cc

# ========== Everything below this line is not well-tested. ===========

# Take these flags for DG/UX (Data General/UX).
#FLAGS	= -O2 -Wall -D_POSIX -D_DGUX_SOURCE -D_SELECT
#PROGS	= minicom runscript
#LFLAGS	= -s
#LIBS	= -ltermcap
#CC	= gcc

# Take these flags for OSF/1
#FLAGS	= -O2 -D_POSIX -D_SYSV -D_SELECT
#PROGS	= minicom runscript
#LFLAGS	= -s
#LIBS	= -ltermcap
#CC	= cc

# These flags work for Coherent 4.2.
#FLAGS	= -D_POSIX -D_SYSV -D_SELECT -D_COH42 -D_COHERENT -D_NO_TERMIOS
#LFLAGS	= -s
#PROGS	= minicom runscript
#LIBS	= -ltermcap -lmisc
#CC	= cc

# Take these flags for Dynix/ptx (Sequent)
#FLAGS	= -D_POSIX -D_SYSV -D_SELECT -D_SEQUENT -D_NO_TERMIOS
#LFLAGS	= -s
#PROGS	= minicom runscript
#LIBS	= -ltermcap -lsocket
#CC	= cc

# Take these flags for a SysV system (sysv/HPUX/ISC)
#FLAGS	= -D_SYSV		 			   # Generic Sysv
#FLAGS	= -D_SYSV -D_POSIX				   # Posix SysV
#FLAGS	= -D_HPUX_SOURCE -D_POSIX -D_SYSV -D_SELECT	   # HPUX
#FLAGS	= -D_SYSV -D_SVR2 -D_POSIX -D_NO_TERMIOS	   # Sysv R2, eg UnixPC
#FLAGS	= -Wall -D_SYSV -DISC -D_SYSV3 -D_POSIX -D_SELECT  # ISC unix
#### Only include the "keyserv" program if you don't have select()
#PROGS	= minicom runscript #keyserv
#LFLAGS	= -s
#LIBS	= -ltermcap
#LIBS	= -ltermcap -linet -lnsl_c -lcposix # ISC unix.
#CC	= cc

# Take these flags for Mark Williams' Coherent version 3.2 (286 version)
#FLAGS	= -D_COHERENT -D_COH3
#LFLAGS	= -s
#PROGS	= minicom runscript keyserv
#LIBS	= -lterm
#CC	= cc

# Take these flags for the PC Minix ACK compiler
#FLAGS	= -D_MINIX -D_POSIX
#LFLAGS	= -i
#PROGS	= minicom runscript keyserv
#LIBS	=
#CC	= cc

# Take these flags for a 68000 Minix
#FLAGS	= -D_MINIX -D_POSIX
#LFLAGS	= -s
#PROGS	= minicom runscript keyserv
#LIBS	=

# Nothing should have to change beneath this line

SRCS	= remote.c vt100.c config.c help.c updown.c \
	  util.c dial.c window.c wkeys.c ipc.c main.c \
	  keyserv.c windiv.c script.c sysdep1.c sysdep2.c \
	  rwconf.c 

HDRS	= remote.h window.h keyboard.h charmap.h config.h \
	  configsym.h patchlevel.h vt100.h port.h sysdep.h

OTHERS	= History Install Makefile Porting Readme Readme.rzsz Todo \
	  install.sh remote.1 remote.users runscript.1 saralogin \
	  scriptdemo unixlogin down

MOBJS	= remote.o vt100.o config.o help.o updown.o \
	  util.o dial.o window.o wkeys.o ipc.o \
	  windiv.o sysdep1.o sysdep2.o rwconf.o main.o

KOBJS	= keyserv.o wkeys.o sysdep2.o

SOBJS	= script.o sysdep1.o

CFLAGS	= $(FLAGS) -DLIBDIR=\"$(LIBDIR)\"

R	= $(ROOTDIR)

all:		$(PROGS)

remote:	$(MOBJS)
		$(CC) $(LFLAGS) -o remote $(MOBJS) $(LIBS)

keyserv:	$(KOBJS)
		$(CC) -o keyserv $(LFLAGS) $(KOBJS) $(LIBS)

runscript:	$(SOBJS)
		$(CC) -o runscript $(LFLAGS) $(SOBJS)

script.o:	script.c

keyserv.o:	keyserv.c $(HDRS)

remote.o:	remote.c $(HDRS)

main.o:		main.c $(HDRS)

windiv.o:	windiv.c $(HDRS)

vt100.o:	vt100.c $(HDRS)
		$(CC) -c $(CFLAGS) -DREMOTE vt100.c

config.o:	config.c $(HDRS)

fastsystem.o:	fastsystem.c $(HDRS)

dial.o:		dial.c $(HDRS)

help.o:		help.c $(HDRS)

updown.o:	updown.c $(HDRS)

window.o:	window.c $(HDRS)

wkeys.o:	wkeys.c $(HDRS)

ipc.o:		ipc.c $(HDRS)

sysdep1.o:	sysdep1.c $(HDRS)

sysdep2.o:	sysdep2.c $(HDRS)

rwconf.o:	rwconf.c $(HDRS)

install:	$(PROGS)
		sh install.sh $(R)$(LIBDIR) $(R)$(BINDIR) $(R)$(MANDIR)

clobber:
		rm -f *.o remote keyserv runscript down

clean:		
		rm -f *.o
		@echo "Type \"make clobber\" to really clean up."

realclean:		
		rm -f *.o remote keyserv runscript *.orig *~
