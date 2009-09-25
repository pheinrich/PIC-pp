/*
Copyright (C) 2005 Richard Altherr <kc8apf@kc8apf.net>

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation; either 
version 2 of the License, or (at your option) any later 
version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#if HAVE_CONFIG_H
# include <config.h>
#endif
#include "serial.h"
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>

int serial_open(const char *port, int baudrate, int databits, 
		parity_t parity, int stopbits) 
{
	int fd;
	struct termios oldtio, newtio;
	int termbaud, termdatabits, termstopbits, termparity;

	// Determine baud rate setting
	switch (baudrate) {
		case 50:
			termbaud = B50;
			break;
		case 75:
			termbaud = B75;
			break;
		case 110:
			termbaud = B110;
			break;
		case 134:
			termbaud = B134;
			break;
		case 150:
			termbaud = B150;
			break;
		case 200:
			termbaud = B200;
			break;
		case 300:
			termbaud = B300;
			break;
		case 600:
			termbaud = B600;
			break;
		case 1200:
			termbaud = B1200;
			break;
		case 1800:
			termbaud = B1800;
			break;
		case 2400:
			termbaud = B2400;
			break;
		case 4800:
			termbaud = B4800;
			break;
		case 9600:
			termbaud = B9600;
			break;
		case 19200:
			termbaud = B19200;
			break;
		case 38400:
			termbaud = B38400;
			break;
		case 57600:
			termbaud = B57600;
			break;
		case 115200:
			termbaud = B115200;
			break;
		default:
			return -1;
	}

	// Two stop bits or one?
	if (stopbits == 2) {
		termstopbits = CSTOPB;
	} else if (stopbits == 1) {
		termstopbits = 0;
	} else {
		return -2;
	}

	// Parity?
	if (parity == PARITY_EVEN) {
		termparity = PARENB;
	} else if (parity == PARITY_ODD) {
		termparity = PARENB | PARODD;
	} else {
		termparity = 0;
	}

	// How many data bits?
	switch (databits) {
		case 5:
			termdatabits = CS5;
			break;
		case 6:
			termdatabits = CS6;
			break;
		case 7:
			termdatabits = CS7;
			break;
		case 8:
			termdatabits = CS8;
			break;
		default:
			return -3;
	}

	// Open the device file
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0)
	{
		// Can't open the device
		return -4;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);

	// Retrieve the current terminal IO settings
	if(-1 == tcgetattr(fd, &oldtio))
	  {
	    close(fd);
	    return -4;
	  }

	oldtio.c_oflag = 0;
	oldtio.c_lflag = 0;
	oldtio.c_iflag &= (IXON | IXOFF | IXANY);
	oldtio.c_cflag &= ~HUPCL;
	oldtio.c_cflag |= termdatabits | termstopbits | termparity | CREAD | CLOCAL;

	// Reads complete if 2 characters are read or
	// .5 seconds elapses
	oldtio.c_cc[VTIME] = 5;
	oldtio.c_cc[VMIN] = 0;

	// Set the new IO settings
	if (cfsetspeed(&oldtio,termbaud) != 0) {
		perror("Ouch");
	};
	if (tcsetattr(fd,TCSANOW,&oldtio) != 0) {
		perror("OOps");
	}

	tcgetattr(fd,&newtio);
	if (newtio.c_cflag != oldtio.c_cflag) printf("cflag\n");
	if (newtio.c_iflag != oldtio.c_iflag) printf("iflag\n");
	if (newtio.c_oflag != oldtio.c_oflag) printf("oflag\n");
	if (newtio.c_lflag != oldtio.c_lflag) printf("lflag\n");
	

	return fd;
}
