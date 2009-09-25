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

int programmer_echo(int port, char ch) {
	unsigned char command;

   command = 2;
   write(port,&command,1);
   write(port,&ch,1);
   read(port,&command,1);
   if( command != ch ) {
      printf("DEBUG: Bad echo: %c != %c\n", command, ch);
      return -1;
   }
   
   return 0;
}

int programmer_proto_check(int port) {
	unsigned char command;
	unsigned char receive[5] = "XYZA\0";

	command = 21;
	write(port,&command,1);
	read(port,receive,4);
	if (strncasecmp(receive,PROTO_VERSION,4) != 0) {
	  printf("Version: %4s\n", receive);
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
   printf("0x%05x: ", len);
	while ((len += read(port,romImage+len,1)) != size) {
      printf(" %02hhx", romImage[len-1]);
      if( 0 == (len % 16))
         printf("\n0x%05x: ",len);
	}
   puts("");

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
   printf("\n0x%05x: ",len+devConfig.eepromStart);
	while ((len += read(port,eepromImage+len,1)) != size) {
      printf(" %02hhx", eepromImage[len-1]);
      if( 0 == (len % 16))
         printf("\n0x%05x: ",len+devConfig.eepromStart);
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
	// Followed by Fuses in MSB
   memcpy(fuseImage, buffer + 2, 22);

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

int programmer_write_rom(int port, char *romImage, deviceConfig_t devConfig)
{
   unsigned short* sp = (unsigned short*)romImage;
   unsigned short* ep = sp + devConfig.romSize;
   unsigned short pad = (unsigned short)devConfig.romPad;

   while( ep > sp && pad == *--ep )
      /* loop to first non-pad word */;

   int in, totalWords = ++ep - sp;
   unsigned char buffer[ 5 ];

   *buffer = 7;
   write( port, buffer, 1 );

   buffer[ 0 ] = totalWords >> 8;
   buffer[ 1 ] = totalWords & 0xff;
   write( port, buffer, 2 );

   read( port, buffer, 1);
   if( 'Y' != *buffer )
   {
      DEBUG_p( "DEBUG: write rom: bad response: %c\n", *buffer );
      return -1;
   }

   while( 1 )
   {
      unsigned short address = sp - (unsigned short*)romImage;

      write( port, sp, 32 );
      /*
      {
	int a;
	printf("0x%04x:", (unsigned int)(((char*)sp) - romImage));
	for(a=0; a<32; a++) {
	  printf(" %02x", ((unsigned char*)sp)[a]);
	}
	puts("");
      }
      */
      printf( "Writing ROM: %d%%\r", 50 * ((char*)sp - romImage) / totalWords );
      fflush(stdout);
      sp += (32 / sizeof( *sp ));

      in = read( port, buffer, 5 );
      switch( *buffer )
      {
         case 'N':
	    printf( "\nFailed to write ROM (current address 0x%0x)\n", (unsigned int)(((char*)sp) - romImage) );
            return -3;

         case 'P':
	    puts("Writing ROM: complete");
	    return 0;

         case 'Y':
            break;

         default:
	    printf( "\nBad write response, '%c' (current address 0x%0x)\n", *buffer, (unsigned int)(((char*)sp) - romImage) );
            break;
      }
   }
}

int programmer_write_eeprom(int port, char *eepromImage, deviceConfig_t devConfig) {
	unsigned char command;
	unsigned char buffer[32];
	int ii, offset, size;

	command = 8;
	write(port,&command,1);
	// Send eepromSize
	size = devConfig.eepromSize;
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
		printf( "Writing EEPROM: %d%%\r", 100 * offset / size );
		fflush(stdout);

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
	puts( "Writing EEPROM: complete" );
	return 0;
}

int programmer_write_fuses(int port, char * fuseImage, deviceConfig_t devConfig) {
	unsigned char command;
	unsigned char buffer[24];

	command = 9;
	write(port,&command,1);

	printf("Writing fuses... ");
	fflush(stdout);

	buffer[0] = '0';
	buffer[1] = '0';

	if (devConfig.bits == 14) {
      memcpy(buffer + 2, fuseImage, 4);
      memset(buffer + 6, 'F', 4);
      memcpy(buffer + 10, fuseImage + 8, 2);
      memset(buffer + 12, 0xff, 12);
	} else if (devConfig.bits == 16) {
      memcpy(buffer + 2, fuseImage, 22);
	} else {
		return -1;
	}

	write(port,buffer,24);
	puts( "complete");
	read(port,buffer,1);
	if (buffer[0] != 'Y') {
		DEBUG_p("DEBUG: write config: bad response %c\n",buffer[0]);
		return -2;
	}

	return 0;	
}

int programmer_write_18Fxxxxx(int port, char* fuseImage, deviceConfig_t devConfig)
{
   unsigned char command;
   unsigned char buffer[24];

   command = 17;
   write(port,&command,1);
   printf( "Writing 18Fxxxx fuses... " );   
   fflush(stdout);

   memset(buffer,0,2);
   memcpy(buffer + 2, fuseImage, 22);

   write(port,buffer,24);
   puts("complete");
	read(port,buffer,1);
	if (buffer[0] != 'Y') {
		DEBUG_p("DEBUG: write 18Fxxxxx: bad response %c\n",buffer[0]);
		return -2;
	}
   
	return 0;	
}
