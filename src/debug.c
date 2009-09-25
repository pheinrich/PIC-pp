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

void debug_print_config(deviceConfig_t config) {
	printf("DEBUG: EraseMode=%d\n",config.eraseMode);
	printf("DEBUG: FlashChip=%d\n",config.flashChip);
	printf("DEBUG: PowerSequence=%d\n",config.powerSequence);
	printf("DEBUG: ProgramDelay=%d\n",config.programDelay);
	printf("DEBUG: ProgramTries=%d\n",config.programTries);
	printf("DEBUG: ProgramFlags=%x\n",config.programFlags);
	printf("DEBUG: OverProgram=%d\n",config.overProgram);
	printf("DEBUG: CoreType=%d\n",config.coreType);
	printf("DEBUG: RomSize=%x\n",config.romSize);
	printf("DEBUG: EepromSize=%x\n",config.eepromSize);
	printf("DEBUG: FUSE1 Blank=%x\n",config.fuse1Blank);
	printf("DEBUG: FUSE2 Blank=%x\n",config.fuse2Blank);
	printf("DEBUG: FUSE3 Blank=%x\n",config.fuse3Blank);
	printf("DEBUG: FUSE4 Blank=%x\n",config.fuse4Blank);
	printf("DEBUG: FUSE5 Blank=%x\n",config.fuse5Blank);
	printf("DEBUG: FUSE6 Blank=%x\n",config.fuse6Blank);
	printf("DEBUG: FUSE7 Blank=%x\n",config.fuse7Blank);
	printf("DEBUG: Fuse Count=%d\n",config.fuseCount);
	printf("DEBUG: ChipID=%x\n",config.chipID);
}

void debug_rom_dump(char * romImage, deviceConfig_t devConfig) {
	int ii;

	printf("ROM Dump\n");
	for (ii = 0; ii < devConfig.romSize *2 ; ii++) {
		printf("%02hhX",romImage[ii]);

		if ((ii & 0xF) == 0xF || ii == devConfig.romSize*2 - 1) {
			printf("\n");
		} else if ((ii & 0x1) == 0x1) {
			printf(" ");
		}
	}
}

void debug_eeprom_dump(char * eepromImage, deviceConfig_t devConfig) {
	int ii;

	printf("\nEEPROM Dump\n");
	for (ii = 0; ii < devConfig.eepromSize ; ii++) {
		printf("%02hhX ",eepromImage[ii]);

		if ((ii & 0xF) == 0xF || ii == devConfig.eepromSize - 1) {
			printf("\n");
		}
	}
}

void debug_config_dump(char * fuseImage, deviceConfig_t devConfig) {
   int i;

	puts( "\nConfig Dump");

   printf("ID: 0x" );
   for( i = 0; i < 8; i++ )
      printf("%02hhx", fuseImage[ i ]);

   printf( "\nFuses:" );
   for( i = 0; i < devConfig.fuseCount; i++ )
      printf( " 0x%02hhx%02hhx", fuseImage[ (i << 1) + 9 ], fuseImage[ (i << 1) + 8 ] );
   printf( "\n" );
}
