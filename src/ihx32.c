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
#include "debug.h"
#include "ihx32.h"

char ascii2hexNybble( char ch )
{
   return( '9' < ch ? ('a' > ch ? ch - 'A' : ch - 'a') + 10 : ch - '0' );
}

int ihx32_read(FILE * hexFile, unsigned char * romImage, unsigned char *eepromImage, unsigned char *fuseImage, deviceConfig_t config ) {
	int lba = 0;
	char lineBuffer[256];
	const char* cp;
	char partBuffer[5];
	char *destImage;
	int numBytes;
	int address;
	int recType;
	int value;
	int ii;
	int done = 0;

	DEBUG_p("Preparing to read from hex file\n");

	while (done == 0) {
		fgets(lineBuffer,255,hexFile);
      cp = lineBuffer;

		// First character is always ':'
      if( ':' != *cp++ )
	continue;  // skip comment lines (anything not starting with ':')

		// Next come the number of bytes
      numBytes = (ascii2hexNybble( *cp++ ) << 4) + ascii2hexNybble( *cp++ );

		// Then the lower 16-bits of the address
      for( ii = 0, address = 0; ii < 4; ii++ )
      {
         address <<= 4;
         address += ascii2hexNybble( *cp++ );
      }

		// And then the record type
      recType = (ascii2hexNybble( *cp++ ) << 4) + ascii2hexNybble( *cp++ );

		switch(recType) {
			case 0: // Data Record
				address += lba;
            if( address >= config.configStart )
            {
               if( address >= config.eepromStart )
                  destImage = eepromImage + address - config.eepromStart;
               else
                  destImage = fuseImage + 8 + address - config.configStart;

               for( ii = 0; ii < numBytes; ii++ )
                  *destImage++ = (ascii2hexNybble( *cp++ ) << 4) + ascii2hexNybble( *cp++ );
            }
            else
            {
               destImage = romImage + address;
               for( ii = 0; ii < numBytes; ii += 2, destImage += 2 )
               {
                  destImage[1] = (ascii2hexNybble( *cp++ ) << 4) + ascii2hexNybble( *cp++ );
                  destImage[0] = (ascii2hexNybble( *cp++ ) << 4) + ascii2hexNybble( *cp++ );
               }
            }
				break;
			case 1: // End of File record
				done = 1;
				break;
			case 2: // TODO : segment address
				break;
			case 4: // Upper 16-bits of address
            for( ii = 0, lba = 0; ii < 4; ii++ )
            {
               lba <<= 4;
               lba += ascii2hexNybble( *cp++ );
            }
            lba <<= 16;
				break;
		}
	}

	return 0;
}

int ihx32_write(FILE * hexFile, unsigned char * romImage, unsigned char * eepromImage, unsigned char * fuseImage, deviceConfig_t config) {
	char buffer[256];
	char work[256];
	int ii;
	int numBytes;
	int runStart;
	int lba;
	char lbachk;
	int lbakick;
	unsigned char checksum;
	// These are in words, convert to bytes
	int romSize = config.romSize*2;
	int eepromStart = config.eepromStart*2;
	int eepromSize = config.eepromSize;
	int configStart = config.configStart*2;
	int configSize = config.fuseCount*2;

	numBytes = 0;
	checksum = 0;
	runStart = -1;
	lba = 0;
	buffer[0] = ':';

	// Write the ROM
	for (ii = 0; ii <= romSize; ii+=2) {
		if (ii == romSize) {
			ii = ii;
		}

		if ((ii % 0x10000) == 0 && ii < romSize) {
			// LBA boundary
			lba = ii & 0xFFFF0000;
			lba >>= 16;

			lbachk = 2 + 4;
			lbachk += (lba & 0xFF00) >> 8;
			lbachk += (lba & 0xFF);
			lbachk = (0 - lbachk) & 0xFF;
			sprintf(buffer,":02000004%04X%02hhX\n",lba,lbachk);
			fputs(buffer,hexFile);
		}
		
		if (ii % 0x10 == 0x0 || ii == romSize || (romImage[ii] == (config.romPad >> 8) && romImage[ii+1] == (config.romPad & 0xFF)) ) {
			// Do we have something to output?
			if (numBytes != 0) {
				// Fill in number of bytes
				sprintf(work,"%02hhX",numBytes);
				buffer[1] = work[0];
				buffer[2] = work[1];
				checksum += numBytes;

				// Then do address
				sprintf(work,"%04X",runStart & 0xFFFF);
				buffer[3] = work[0];
				buffer[4] = work[1];
				buffer[5] = work[2];
				buffer[6] = work[3];
				checksum += (runStart & 0xFF00) >> 8;
				checksum += (runStart & 0xFF);

				// Then the record type
				buffer[7] = '0';
				buffer[8] = '0';

				// Negate the checksum per the rules
				checksum = (0 - checksum);

				sprintf(work,"%02X",checksum);
				buffer[9+(numBytes*2)] = work[0];
				buffer[9+(numBytes*2)+1] = work[1];
				buffer[9+(numBytes*2)+2] = '\n';
				buffer[9+(numBytes*2)+2+1] = '\0';

				fputs(buffer, hexFile);

				numBytes = 0;
				checksum = 0;
				runStart = ii;
			} else {
				runStart = -1;
			}

			if (ii % 0x10 == 0x0 && ii < romSize && !(romImage[ii] == (config.romPad >> 8) && romImage[ii+1] == (config.romPad & 0xFF)) ) {
				if (runStart == -1) runStart = ii;

				sprintf(work,"%02X",romImage[ii+1]);
				buffer[9+(numBytes*2)] = work[0];
				buffer[9+(numBytes*2)+1] = work[1];
				checksum += romImage[ii+1];

				sprintf(work,"%02X",romImage[ii]);
				buffer[9+(numBytes*2)+2] = work[0];
				buffer[9+(numBytes*2)+3] = work[1];
				checksum += romImage[ii];

				numBytes += 2;
			}
		} else {
			if (runStart == -1) runStart = ii;

			sprintf(work,"%02X",romImage[ii+1]);
			buffer[9+(numBytes*2)] = work[0];
			buffer[9+(numBytes*2)+1] = work[1];
			checksum += romImage[ii+1];

			sprintf(work,"%02X",romImage[ii]);
			buffer[9+(numBytes*2)+2] = work[0];
			buffer[9+(numBytes*2)+3] = work[1];
			checksum += romImage[ii];

			numBytes += 2;
		}

	}

	if ((configStart & 0xFFFF0000) >> 16 != lba) {
		lbakick = 1;
	}

	runStart = -1;
	// Write the config
	for (ii = 0; ii <= configSize; ii+=2) {
		if ((((configStart+ii) % 0x10000) == 0 || lbakick == 1) && ii < configSize ) {
			// LBA boundary
			lba = (configStart+ii) & 0xFFFF0000;
			lba >>= 16;

			lbachk = 2 + 4;
			lbachk += (lba & 0xFF00) >> 8;
			lbachk += (lba & 0xFF);
			lbachk = (0 - lbachk) & 0xFF;
			sprintf(buffer,":02000004%04X%02hhX\n",lba,lbachk);
			fputs(buffer,hexFile);
		}
		
		if ((configStart+ii) % 0x10 == 0x0 || ii == configSize) {
			// Do we have something to output?
			if (numBytes != 0) {
				// Fill in number of bytes
				sprintf(work,"%02hhX",numBytes);
				buffer[1] = work[0];
				buffer[2] = work[1];
				checksum += numBytes;

				// Then do address
				sprintf(work,"%04X",(configStart+runStart) & 0xFFFF);
				buffer[3] = work[0];
				buffer[4] = work[1];
				buffer[5] = work[2];
				buffer[6] = work[3];
				checksum += ((configStart+runStart) & 0xFF00) >> 8;
				checksum += ((configStart+runStart) & 0xFF);

				// Then the record type
				buffer[7] = '0';
				buffer[8] = '0';

				// Negate the checksum per the rules
				checksum = (0 - checksum);

				sprintf(work,"%02X",checksum);
				buffer[9+(numBytes*2)] = work[0];
				buffer[9+(numBytes*2)+1] = work[1];
				buffer[9+(numBytes*2)+2] = '\n';
				buffer[9+(numBytes*2)+2+1] = '\0';

				fputs(buffer, hexFile);

				numBytes = 0;
				checksum = 0;
				runStart = ii;
			} else {
				runStart = -1;
			}

			if ((configStart+ii) % 0x10 == 0x0 && ii < configSize) {
				if (runStart == -1) runStart = ii;

				sprintf(work,"%02X",fuseImage[8+ii+1]);
				buffer[9+(numBytes*2)] = work[0];
				buffer[9+(numBytes*2)+1] = work[1];
				checksum += fuseImage[8+ii+1];

				sprintf(work,"%02X",fuseImage[8+ii]);
				buffer[9+(numBytes*2)+2] = work[0];
				buffer[9+(numBytes*2)+3] = work[1];
				checksum += fuseImage[8+ii];

				numBytes += 2;
			}
		} else {
			if (runStart == -1) runStart = ii;

			sprintf(work,"%02X",fuseImage[8+ii+1]);
			buffer[9+(numBytes*2)] = work[0];
			buffer[9+(numBytes*2)+1] = work[1];
			checksum += fuseImage[8+ii+1];

			sprintf(work,"%02X",fuseImage[8+ii]);
			buffer[9+(numBytes*2)+2] = work[0];
			buffer[9+(numBytes*2)+3] = work[1];
			checksum += fuseImage[8+ii];

			numBytes += 2;
		}

	}

	if ((eepromStart & 0xFFFF0000) >> 16 != lba) {
		lbakick = 1;
	}

	// Write the EEPROM
	if (eepromSize > 0) {
		runStart = -1;
		for (ii = 0; ii <= eepromSize; ii+=2) {
			if ((ii % 0x10000) == 0 && ii < eepromSize) {
				// LBA boundary
				lba = (eepromStart+ii) & 0xFFFF0000;
				lba >>= 16;

				lbachk = 2 + 4;
				lbachk += (lba & 0xFF00) >> 8;
				lbachk += (lba & 0xFF);
				lbachk = (0 - lbachk) & 0xFF;
				sprintf(buffer,":02000004%04X%02hhX\n",lba,lbachk);
				fputs(buffer,hexFile);
			}
			
			if ((eepromStart+ii) % 0x10 == 0x0 || ii == eepromSize || (eepromImage[ii] == (config.eepromPad) && eepromImage[ii+1] == (config.eepromPad & 0xFF)) ) {
				// Do we have something to output?
				if (numBytes != 0) {
					// Fill in number of bytes
					sprintf(work,"%02hhX",numBytes);
					buffer[1] = work[0];
					buffer[2] = work[1];
					checksum += numBytes;

					// Then do address
					sprintf(work,"%04X",(eepromStart+runStart) & 0xFFFF);
					buffer[3] = work[0];
					buffer[4] = work[1];
					buffer[5] = work[2];
					buffer[6] = work[3];
					checksum += ((eepromStart+runStart) & 0xFF00) >> 8;
					checksum += ((eepromStart+runStart) & 0xFF);

					// Then the record type
					buffer[7] = '0';
					buffer[8] = '0';

					// Negate the checksum per the rules
					checksum = (0 - checksum);

					sprintf(work,"%02X",checksum);
					buffer[9+(numBytes*2)] = work[0];
					buffer[9+(numBytes*2)+1] = work[1];
					buffer[9+(numBytes*2)+2] = '\n';
					buffer[9+(numBytes*2)+2+1] = '\0';

					fputs(buffer, hexFile);

					numBytes = 0;
					checksum = 0;
					runStart = ii;
				} else {
					runStart = -1;
				}

				if ((eepromStart+ii) % 0x10 == 0x0 && ii < eepromSize && !(eepromImage[ii] == (config.eepromPad) && eepromImage[ii+1] == (config.eepromPad & 0xFF)) ) {
					if (runStart == -1) runStart = ii;

					sprintf(work,"%02X",eepromImage[ii+1]);
					buffer[9+(numBytes*2)] = work[0];
					buffer[9+(numBytes*2)+1] = work[1];
					checksum += eepromImage[ii+1];

					sprintf(work,"%02X",eepromImage[ii]);
					buffer[9+(numBytes*2)+2] = work[0];
					buffer[9+(numBytes*2)+3] = work[1];
					checksum += eepromImage[ii];

					numBytes += 2;
				}
			} else {
				if (runStart == -1) runStart = ii;

				sprintf(work,"%02X",eepromImage[ii+1]);
				buffer[9+(numBytes*2)] = work[0];
				buffer[9+(numBytes*2)+1] = work[1];
				checksum += eepromImage[ii+1];

				sprintf(work,"%02X",eepromImage[ii]);
				buffer[9+(numBytes*2)+2] = work[0];
				buffer[9+(numBytes*2)+3] = work[1];
				checksum += eepromImage[ii];

				numBytes += 2;
			}

		}
	}

	// Write the completion
	strcpy(buffer,":00000001FF\n");
	fputs(buffer,hexFile);

	return 0;
}
