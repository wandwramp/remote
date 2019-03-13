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
 * Send a string to the modem.
 */
void m_puts(s)
char *s;
{
  char c;

  while(*s) {
  	if (*s == '^' && (*(s + 1))) {
  		s++;
  		if (*s == '^')
  			c = *s;
  		else
  			c = (*s) & 31;
  	} else
  		c = *s;
  	if (c == '~')
  		sleep(1);
  	else	
  		write(portfd, &c, 1);
  	s++;
  }
}

/* Hangup. */
void m_hangup()
{
  /* -- not if socket */
  if( isSocket )
    return;

  if (P_MDROPDTR[0] == 'Y') {
  	m_dtrtoggle(portfd);
  } else {
  	m_puts(P_MHANGUP);
  	sleep(1);
  }
  /* If we don't have DCD support fake DCD dropped */
  bogus_dcd = 0;
}

