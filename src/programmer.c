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
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "debug.h"
#include "programmer.h"

#define PROTO_VERSION "P018"

int programmer_reset(int port) {
	unsigned char receive;
	unsigned char command = 'Z';

	command = 1;
	write(port,&command,1);

	// Check for a success
	read(port,&receive,1);

	if (receive != 'Q') {
		DEBUG_p("Receieved: %d %c\n",receive,receive);
		return -1;
	}

	return 0;
}
	
int programmer_program_mode(int port) {
	unsigned char receive;
	unsigned char command = 'X';

	command = 'P';
	write(port,&command,1);

	// Check for success
	read(port,&receive,1);
	if (receive != 'P') {
		printf("DEBUG: Bad return: %c\n",receive);
		return -1;
	}

	return 0;
}

int programmer_proto_check(int port) {
	unsigned char command;
	unsigned char receive[4] = "XYZA";

	command = 21;
	write(port,&command,1);
	read(port,receive,4);
	if (strncasecmp(receive,PROTO_VERSION,4) != 0) {
		printf("Version: %4c\n",receive);
		return -2;
	}

	return 0;
}

int programmer_chip_in_socket(int port) {
	unsigned char command;
	unsigned char receive = 'W';

	command = 18;
	write(port,&command,1);
	read(port,&receive,1);
	if (receive != 'A') {
		printf("Got %c\n",receive);
		return -1;
	}

	if (read(port,&receive,1) == 0) {
		return -2;
	}

	return 0;
}
	
int programmer_init(int port, deviceConfig_t devConfig) {
	unsigned char buffer[256] = "ABCDEF";
	unsigned char command;

	// Send initilization values
	command = 3;
	write(port,&command,1);

	buffer[0] = devConfig.romSize >> 8;
	buffer[1] = devConfig.romSize & 0xFF;
	buffer[2] = devConfig.eepromSize >> 8;
	buffer[3] = devConfig.eepromSize & 0xFF;
	buffer[4] = devConfig.coreType & 0xFF;
	buffer[5] = devConfig.programFlags & 0xFF;
	buffer[6] = devConfig.programDelay & 0xFF;
	buffer[7] = devConfig.powerSequence & 0xFF;
	buffer[8] = devConfig.eraseMode & 0xFF;
	buffer[9] = devConfig.programTries & 0xFF;
	buffer[10] = devConfig.programTries & 0xFF;
	if (write(port,buffer,11) != 11) {
		printf("Bad write!");
	}

	// Check for success
	while (read(port,buffer,1) != 1);
	if (buffer[0] != 'I') {
		printf("Bad response: %c\n",buffer[0]);
		return -1;
	}

	return 0;
}

int programmer_voltages_on(int port) {
	unsigned char command;
	unsigned char buffer;

	command = 4;
	write(port,&command,1);

	read(port,&buffer,1);
	if (buffer != 'V') {
		printf("Voltages on: bad response: %d\n",buffer);
		return -1;
	}

	return 0;
}

int programmer_voltages_off(int port) {
	unsigned char command;
	unsigned char buffer;

	command = 5;
	write(port,&command,1);

	read(port,&buffer,1);
	if (buffer != 'v') {
		printf("Voltages off: bad response: %d\n",buffer);
		return -1;
	}

	return 0;
}

int programmer_read_chipId(int port) {
	unsigned char command;
	unsigned char buffer[256];
	int len;

	command = 13;
	write(port,&command,1);

	read(port,buffer,1);
	if (buffer[0] != 'C') {
		DEBUG_p("Read config: bad response: %d\n", buffer[0]);
		return -1;
	}

	len = 0;
	while ((len += read(port,buffer+len,26)) != 26) {
		DEBUG_p("Incomplete read (len = %d)\n",len);
	}

	return ((int)(buffer[1]) << 8 | buffer[0]) & 0xFFE0;
}

int programmer_read_oscal(int port) {
	unsigned char command;
	unsigned char buffer[256];
	int len;

	command = 13;
	write(port,&command,1);

	read(port,buffer,1);
	if (buffer[0] != 'C') {
		DEBUG_p("Read config: bad response: %d\n", buffer[0]);
		return -1;
	}

	len = 0;
	while ((len += read(port,buffer+len,26)) != 26) {
		DEBUG_p("Incomplete read (len = %d)\n",len);
	}

	return ((int)(buffer[25]) << 8 | buffer[24]);
}


int programmer_write_oscal(int port, int oscal, int fuse1) {
	unsigned char command;
	unsigned char buffer[4];

	command = 10;
	write(port,&command,1);

	buffer[0] = (oscal >> 8) & 0xFF;
	buffer[1] = oscal & 0xFF;
	buffer[2] = (fuse1 >> 8) & 0xFF;
	buffer[3] = fuse1 & 0xFF;
	write(port,buffer,4);

	read(port,buffer,1);
	if (buffer[0] == 'C') {
		DEBUG_p("DEBUG: Write Calibration - error writing calibration\n");
		return -1;
	} else if (buffer[0] == 'F') {
		DEBUG_p("DEBUG: Write Calibration - error writing fuse\n");
		return -2;
	} else if (buffer[0] != 'Y') {
		return -3;
	}

	return 0;
}

int programmer_read_rom(int port, unsigned char * romImage, deviceConfig_t devConfig) {
	unsigned char command;
	int size;
	int len;

	command = 11;
	write(port,&command,1);

	len = 0;
	size = devConfig.romSize * 2;
	while ((len += read(port,romImage+len,256)) != size) {
		DEBUG_p("DEBUG: rom read %d of %d bytes\n",len,size);
	}

	return 0;
}

int programmer_read_eeprom(int port, unsigned char * eepromImage, deviceConfig_t devConfig) {
	unsigned char command;
	int size;
	int len;

	command = 12;
	write(port,&command,1);

	len = 0;
	size = devConfig.eepromSize;
	while ((len += read(port,eepromImage+len,256)) != size) {
		DEBUG_p("DEBUG: eeprom read %d of %d bytes\n",len,size);
	}

	return 0;
}

int programmer_read_config(int port, char * fuseImage, deviceConfig_t devConfig) {
	unsigned char command;
	unsigned char buffer[256];
	int size;
	int len;
	int ii;

	command = 13;
	write(port,&command,1);

	read(port,buffer,1);
	if (buffer[0] != 'C') {
		DEBUG_p("DEBUG: Read config: bad response: %d\n", buffer[0]);
		return -1;
	}

	len = 0;
	while ((len += read(port,buffer+len,26)) != 26) {
		DEBUG_p("DEBUG: config read %d of 26\n",len);
	}

	// IDs first
	for (ii = 0; ii < 8; ii++) {
		fuseImage[ii] = buffer[ii+2];
	}

	// Followed by Fuses in MSB
	for (ii = 0; ii < devConfig.fuseCount*2; ii += 2) {
		fuseImage[8+ii] = buffer[10+ii+1];
		fuseImage[8+ii+1] = buffer[10+ii];
	}

	return 0;
}

int programmer_erase_chip(int port) {
	unsigned char command;
	unsigned char buffer;

	command = 14;
	write(port,&command,1);

	read(port,&buffer,1);
	if (buffer != 'Y') {
		DEBUG_p("DEBUG: erase: bad response: %c\n", buffer);
		return -1;
	}

	return 0;
}

// Returns 0 if rom is erased, 1 if not, and negative on error
int programmer_erase_rom_check(int port, deviceConfig_t devConfig ) {
	unsigned char command;
	unsigned char buffer;

	command = 15;
	write(port, &command, 1);

	// Set the high byte of the rom padding
	buffer = (devConfig.romPad >> 8) & 0xFF;
	DEBUG_p("DEBUG: erase rom check: sending %02x as high byte\n",buffer);
	write(port, &buffer, 1);

	buffer = '\0';
	read(port, &buffer, 1);
	while (buffer == 'B') {
		// More bytes being checked
		DEBUG_p("DEBUG: erase rom check: read 256 bytes\n");
		read(port, &buffer, 1);
	}

	if (buffer == 'Y' || (devConfig.calWord == 1 && buffer == 'C')) {
		return 0;
	} else if (buffer == 'N') {
		return 1;
	} else {
		DEBUG_p("DEBUG: erase rom check: bad response: %c\n",buffer);
		return -1;
	}

}

// Returns 0 if eeprom is erased, 1 if not, and negative on error
int programmer_erase_eeprom_check(int port) {
	unsigned char command;
	unsigned char buffer;

	command = 16;
	write(port, &command, 1);

	read(port, &buffer, 1);
	if (buffer == 'Y') {
		return 0;
	} else if (buffer == 'N') {
		return 1;
	} else {
		DEBUG_p("DEBUG: erase eeprom check: bad response: %c\n",buffer);
		return -1;
	}

}

int programmer_write_rom(int port, char *romImage, deviceConfig_t devConfig) {
	unsigned char command;
	unsigned char buffer[32];
	int ii, offset, endOffset, keepGoing = 1;

	for (ii = devConfig.romSize*2 - 2; ii >= 0; ii -= 2) {
		if (romImage[ii] != (devConfig.romPad >> 8) && romImage[ii+1] != (devConfig.romPad & 0xFF)) {
			endOffset = ii;
			break;
		}
	}
	endOffset += 2;
	endOffset /= 2;
	DEBUG_p("DEBUG: Going to write from 0x0000 - 0x%04X\n",endOffset);

	command = 7;
	write(port,&command,1);

	// Send romSize
	buffer[0] = endOffset >> 8;
	buffer[1] = endOffset & 0xFF;
	write(port,buffer,2);

	read(port,buffer,1);
	if (buffer[0] != 'Y') {
		DEBUG_p("DEBUG: write rom: bad response: %c\n",buffer[0]);
		return -1;
	}

	// Fill Buffer A
	for (ii = 0; ii < 32; ii++) {
		buffer[ii] = romImage[ii];
	}
	offset = ii;
	write(port,buffer,32);

	read(port,buffer,1);
	if (buffer[0] != 'Y') {
		DEBUG_p("DEBUG: write rom: buffer_A bad response: %c\n",buffer[0]);
		return -2;
	}

	// Fill Buffer B
	for (ii = 0; ii < 32; ii++) {
		buffer[ii] = romImage[offset+ii];
	}
	offset += ii;
	write(port,buffer,32);

	while (1) {
		// Blank buffer
		buffer[0] = '\0';
		buffer[1] = '\0';
		buffer[2] = '\0';

		// Read completion codes
		read(port,buffer,1);

		// Possible results are
		// 'N' = Programming error followed by current addres MSB first
		// 'YP' = Programming finished
		// 'Y' = Read for next 32 bytes
		if (buffer[0] == 'N') {
			read(port,buffer,2);
			DEBUG_p("DEBUG: write rom: error writing %x\n",(unsigned int)(buffer[0] << 8) | buffer[1]);
			return -3;
		} else if (buffer[0] == 'Y') {
			if (read(port,buffer,1) == 1 && buffer[0] == 'P') {
				// All done
				break;
			}
		}

		// Write the next 32 bytes
		for (ii = 0; ii < 32; ii++) {
			buffer[ii] = romImage[offset+ii];
		}
		offset += ii;
		write(port, buffer, 32);
	}

	return 0;
}

int programmer_write_eeprom(int port, char *eepromImage, deviceConfig_t devConfig) {
	unsigned char command;
	unsigned char buffer[32];
	int ii, offset, size;

	command = 8;
	write(port,&command,1);

	// Send eepromSize
	size = devConfig.eepromSize + 2;
	buffer[0] = size >> 8;
	buffer[1] = size & 0xFF;
	write(port,buffer,2);

	read(port,buffer,1);
	if (buffer[0] != 'Y') {
		DEBUG_p("DEBUG: write eeprom: bad response: %c\n",buffer[0]);
		return -1;
	}

	offset = 0;

	while (1) {
		buffer[0] = eepromImage[offset];
		buffer[1] = eepromImage[offset+1];
		write(port,buffer,2);

		// Read completion codes
		read(port,buffer,1);

		// Possible results are
		// 'P' = Programming finished
		// 'Y' = Read for next 2 bytes
		if (buffer[0] == 'P') {
			// All done
			break;
		}

		offset += 2;
	}

	return 0;
}

int programmer_write_fuses(int port, char * fuseImage, deviceConfig_t devConfig) {
	unsigned char command;
	unsigned char buffer[24];

	command = 9;
	write(port,&command,1);

	buffer[0] = '0';
	buffer[1] = '0';

	if (devConfig.bits == 14) {
		buffer[2] = fuseImage[0];
		buffer[3] = fuseImage[1];
		buffer[4] = fuseImage[2];
		buffer[5] = fuseImage[3];
		buffer[6] = 'F';
		buffer[7] = 'F';
		buffer[8] = 'F';
		buffer[9] = 'F';
		buffer[10] = fuseImage[9];
		buffer[11] = fuseImage[8];
		buffer[12] = 0xFF;
		buffer[13] = 0xFF;
		buffer[14] = 0xFF;
		buffer[15] = 0xFF;
		buffer[16] = 0xFF;
		buffer[17] = 0xFF;
		buffer[18] = 0xFF;
		buffer[19] = 0xFF;
		buffer[20] = 0xFF;
		buffer[21] = 0xFF;
		buffer[22] = 0xFF;
		buffer[23] = 0xFF;
	} else if (devConfig.bits == 16) {
		buffer[2] = fuseImage[0];
		buffer[3] = fuseImage[1];
		buffer[4] = fuseImage[2];
		buffer[5] = fuseImage[3];
		buffer[6] = fuseImage[4];
		buffer[7] = fuseImage[5];
		buffer[8] = fuseImage[6];
		buffer[9] = fuseImage[7];
		buffer[10] = fuseImage[9];
		buffer[11] = fuseImage[8];
		buffer[12] = fuseImage[11];
		buffer[13] = fuseImage[10];
		buffer[14] = fuseImage[13];
		buffer[15] = fuseImage[12];
		buffer[16] = fuseImage[15];
		buffer[17] = fuseImage[14];
		buffer[18] = fuseImage[17];
		buffer[19] = fuseImage[16];
		buffer[20] = fuseImage[19];
		buffer[21] = fuseImage[18];
		buffer[22] = fuseImage[21];
		buffer[23] = fuseImage[20];
	} else {
		return -1;
	}

	write(port,buffer,24);

	read(port,buffer,1);
	if (buffer[0] != 'Y') {
		DEBUG_p("DEBUG: write config: bad response %c\n",buffer[0]);
		return -2;
	}

	return 0;	
}
