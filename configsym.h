/*
 * configsym.h	- Offsets into the mpars structure
 *		  When the mpars structure is changed,
 *		  change these define's too.
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1995 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * fmg 1/11/94 colors
 * fmg 2/15/94 macros
 *
 */

struct pars {
  /* value is first, so that (char *)mpars[0] == mpars[0].value */
  /* Try doing this in PASCAL !! :-) */
  char value[64];
  int flags;
  char *desc;
};
extern struct pars mpars[];

/* fmg 2/20/94 macros - Length of Macros */

#ifndef MAC_LEN
#define MAC_LEN 257
#endif

struct macs {
  char value[MAC_LEN];
  int flags;
  char *desc;
};
extern struct macs mmacs[];


#define CHANGED	3
#define PRIVATE	4
#define PUBLIC	8

#define PROTO_BASE	0
#define MAXPROTO	12
#define PROG_BASE	12

#define P_PNN(n)	(mpars[PROTO_BASE + n].value[0])
#define P_PUD(n)	(mpars[PROTO_BASE + n].value[1])
#define P_PFULL(n)	(mpars[PROTO_BASE + n].value[2])
#define P_PIORED(n)	(mpars[PROTO_BASE + n].value[3])
#define P_PNAME(n)	(&mpars[PROTO_BASE + n].value[4])
#define P_PPROG(n)	mpars[PROG_BASE + n].value

#define P_PORT		mpars[24].value
#define P_CALLIN	mpars[25].value
#define P_CALLOUT	mpars[26].value
#define P_LOCK		mpars[27].value
#define P_BAUDRATE	mpars[28].value
#define P_BITS		mpars[29].value
#define P_PARITY	mpars[30].value
#define P_KERMIT	mpars[31].value
#define P_KERMALLOW	mpars[32].value
#define P_KERMREAL	mpars[33].value
#define P_COLUSAGE	mpars[34].value
#define P_SCRIPTPROG	mpars[35].value
/* The next entries must be kept in order */
#define P_MINIT		mpars[36].value
#define P_MRESET	mpars[37].value
#define P_MDIALPRE	mpars[38].value
#define P_MDIALSUF	mpars[39].value
#define P_MDIALPRE2	mpars[40].value
#define P_MDIALSUF2	mpars[41].value
#define P_MDIALPRE3	mpars[42].value
#define P_MDIALSUF3	mpars[43].value
#define P_MCONNECT	mpars[44].value
#define P_MNOCON1	mpars[45].value
#define P_MNOCON2	mpars[46].value
#define P_MNOCON3	mpars[47].value
#define P_MNOCON4	mpars[48].value
#define P_MHANGUP	mpars[49].value
#define P_MDIALCAN	mpars[50].value
#define P_MDIALTIME	mpars[51].value
#define P_MRDELAY	mpars[52].value
#define P_MRETRIES	mpars[53].value
/* Yup, until here. */
#define P_MAUTOBAUD	mpars[54].value
#define P_MDROPDTR	mpars[55].value
#define P_UPDIR		mpars[56].value
#define P_DOWNDIR	mpars[57].value
#define P_SCRIPTDIR	mpars[58].value
#define P_ESCAPE	mpars[59].value
#define P_BACKSPACE	mpars[60].value
#define P_STATLINE	mpars[61].value
#define P_HASDCD	mpars[62].value
#define P_HASRTS	mpars[63].value
#define P_HASXON	mpars[64].value
#define P_PAUTO		mpars[65].value

/* fmg colors - these are used in signaling when values have changed
                so that the preferences saving function knows what to save */

#define P_MFG           mpars[66].value
#define P_MBG           mpars[67].value
#define P_TFG           mpars[68].value
#define P_TBG           mpars[69].value
#define P_SFG           mpars[70].value
#define P_SBG           mpars[71].value

/* fmg  macros file name & entry used to signal when macros need to be saved */

#define P_MACROS        mpars[72].value  /* macros save filename */
#define P_MACCHG        mpars[73].value  /* macros changed flag */
#define P_MACENAB	mpars[74].value	 /* macros enabled flag */

#define P_SOUND		mpars[75].value
#define P_HISTSIZE      mpars[76].value  /* History buffer size */
 
/* fmg - macros struct */

#define P_MAC1          mmacs[0].value
#define P_MAC2          mmacs[1].value
#define P_MAC3          mmacs[2].value
#define P_MAC4          mmacs[3].value
#define P_MAC5          mmacs[4].value
#define P_MAC6          mmacs[5].value
#define P_MAC7          mmacs[6].value
#define P_MAC8          mmacs[7].value
#define P_MAC9          mmacs[8].value
#define P_MAC10          mmacs[9].value

