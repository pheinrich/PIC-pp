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
/**
 * @file Serial.h
 *
 * @author Rick Altherr
 * @date 2004
 *
 * @brief Serial port support functions
 */

#ifndef HTG_SERIAL_H
#define HTG_SERIAL_H

typedef enum {
	PARITY_NONE = 0,
	PARITY_ODD  = 1,
	PARITY_EVEN = 2
} parity_t;

typedef enum {
	SERIAL_BAUDRATE = -1,
	SERIAL_STOPBITS = -2,
	SERIAL_DATABITS = -3,
	SERIAL_CANTOPEN = -4
} serial_errors_t;

/*
 * Opens a serial port with the given parameters
 * Input:  baudrate - speed of the serial port in bits per second
 *         databits - number of bits to represent a character
 *         parity   - type of parity calculation to use
 *         stopbits - number of bits to indicate end of character
 * Output: file descriptor for the given serial port or
 *         a negative number representing the error encountered
 */
#ifdef __cplusplus
extern "C" {
#endif 
int serial_open(const char *port, 
		int baudrate, 
		int databits, 
		parity_t parity, 
		int stopbits);
#ifdef __cplusplus
}
#endif
#endif
