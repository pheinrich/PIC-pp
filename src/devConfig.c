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
#include <unistd.h>
#include <string.h>
#include "devConfig.h"

#define BUFFER_SIZE 255

int config_parse(char * configFile, char * chipName, deviceConfig_t * devConfig) {
	FILE * configFd;
	char *token;
	char key[BUFFER_SIZE];
	char value[BUFFER_SIZE];
	char buffer[BUFFER_SIZE];
	int foundChip = 0;

	// Blank the structure
	memset(devConfig,0,sizeof(deviceConfig_t));

	// Open the config file
	if ((configFd = fopen(configFile, "r")) == NULL) {
		return -1;
	}

	// Read until we hit the end of the file
	while (fgets(buffer,BUFFER_SIZE,configFd) != NULL) {
		// Blank line
		if (buffer[0] == '\n') {
			if (foundChip != 0) {
				break;
			} else {
				continue;
			}
		}

		// Parse the key and value
		sscanf(buffer,"%[0-9a-zA-Z]=%s",key,value);

		// Look for the CHIPname
		// If we find the right CHIPname value, read everything else
		if (strcasecmp(key,"CHIPname") == 0) {
			if (strcasecmp(value,chipName) == 0) {
				foundChip = 1;
			}
		} else if (foundChip != 0) {
			if (strcasecmp(key,"EraseMode") == 0) {
				devConfig->eraseMode = strtol(value,NULL,10);
			} else if (strcasecmp(key,"FlashChip") == 0) {
				if (strcasecmp(value,"Y") == 0) {
					devConfig->flashChip = 1;
				} else {
					devConfig->flashChip = 0;
				}
			} else if (strcasecmp(key,"PowerSequence") == 0) {
				if (strcasecmp(value,"Vcc") == 0) {
					devConfig->powerSequence = 0;
					devConfig->programFlags |= 1 << 3;
				} else if (strcasecmp(value,"VccVpp1") == 0) {
					devConfig->powerSequence = 1;
					devConfig->programFlags |= 1 << 3;
				} else if (strcasecmp(value,"VccVpp2") == 0) {
					devConfig->powerSequence = 2;
					devConfig->programFlags |= 1 << 3;
				} else if (strcasecmp(value,"Vpp1Vcc") == 0) {
					devConfig->powerSequence = 3;
					devConfig->programFlags |= 1 << 3;
				} else if (strcasecmp(value,"Vpp2Vcc") == 0) {
					devConfig->powerSequence = 4;
					devConfig->programFlags |= 1 << 3;
				} else if (strcasecmp(value,"VccFastVPP1") == 0) {
					devConfig->powerSequence = 1;
					devConfig->programFlags &= 0xFB;
				} else if (strcasecmp(value,"VccFastVPP2") == 0) {
					devConfig->powerSequence = 2;
					devConfig->programFlags &= 0xFB;
				} else {
					return -2;
				}
			} else if (strcasecmp(key,"ProgramDelay") == 0) {
				devConfig->programDelay = strtol(value,NULL,10);
			} else if (strcasecmp(key,"ProgramTries") == 0) {
				devConfig->programTries = strtol(value,NULL,10);
			} else if (strcasecmp(key,"OverProgram") == 0) {
				devConfig->overProgram = strtol(value,NULL,10);
			} else if (strcasecmp(key,"CoreType") == 0) {
				if (strcasecmp(value,"bit12_A") == 0) {
					devConfig->coreType = 4;
					devConfig->bits = 12;
				} else if (strcasecmp(value,"bit12_B") == 0) {
					devConfig->coreType = 11;
					devConfig->bits = 12;
				} else if (strcasecmp(value,"bit14_A") == 0) {
					devConfig->coreType = 5;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_B") == 0) {
					devConfig->coreType = 6;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_C") == 0) {
					devConfig->coreType = 7;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_D") == 0) {
					devConfig->coreType = 8;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_E") == 0) {
					devConfig->coreType = 9;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_F") == 0) {
					devConfig->coreType = 10;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_G") == 0) {
					devConfig->coreType = 3;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit14_H") == 0) {
					devConfig->coreType = 12;
					devConfig->bits = 14;
				} else if (strcasecmp(value,"bit16_A") == 0) {
					devConfig->coreType = 1;
					devConfig->bits = 16;
				} else if (strcasecmp(value,"bit16_B") == 0) {
					devConfig->coreType = 2;
					devConfig->bits = 16;
				} else if (strcasecmp(value,"bit16_C") == 0) {
					devConfig->coreType = 0;
					devConfig->bits = 16;
				} else {
					return -3;
				}

				if (devConfig->bits == 12) {
					devConfig->romPad = 0xFFF;
					devConfig->eepromStart = 0;
					devConfig->configStart = 0xFFF;
				} else if (devConfig->bits == 14) {
					devConfig->romPad = 0x3FFF;
					devConfig->eepromStart = 0x2100;
					devConfig->eepromPad = 0xFF;
					devConfig->configStart = 0x2007;
				} else if (devConfig->bits == 16) {
					devConfig->romPad = 0xFFFF;
					devConfig->eepromStart = 0xF00000;
					devConfig->eepromPad = 0xFF;
					devConfig->configStart = 0x300000;
				}
			} else if (strcasecmp(key,"ROMsize") == 0) {
				devConfig->romSize = strtol(value,NULL,16);
			} else if (strcasecmp(key,"EEPROMsize") == 0) {
				devConfig->eepromSize = strtol(value,NULL,16);
			} else if (strcasecmp(key,"FUSEblank") == 0) {
				devConfig->fuse1Blank = 0;
				devConfig->fuse2Blank = 0;
				devConfig->fuse3Blank = 0;
				devConfig->fuse4Blank = 0;
				devConfig->fuse5Blank = 0;
				devConfig->fuse6Blank = 0;
				devConfig->fuse7Blank = 0;
				devConfig->fuseCount = sscanf(value,"%x %x %x %x %x %x %x\n",
					&(devConfig->fuse1Blank),
					&(devConfig->fuse2Blank),
					&(devConfig->fuse3Blank),
					&(devConfig->fuse4Blank),
					&(devConfig->fuse5Blank),
					&(devConfig->fuse6Blank),
					&(devConfig->fuse7Blank));
			} else if (strcasecmp(key,"CALword") == 0) {
				if (strcasecmp(value,"Y") == 0) {
					devConfig->programFlags |= 1;
					devConfig->calWord = 1;
				} else {
					devConfig->programFlags &= 0xFE;
					devConfig->calWord = 0;
				}
			} else if (strcasecmp(key,"BandGap") == 0) {
				if (strcasecmp(value,"Y") == 0) {
					devConfig->programFlags |= (1<<1);
					devConfig->bandGap = 1;
				} else {
					devConfig->programFlags &= 0xFD;
					devConfig->bandGap = 0;
				}
			} else if (strcasecmp(key,"ChipID") == 0) {
				devConfig->chipID = strtol(value,NULL,16);
			}
		}
	}

	if (foundChip) {
		return 0;
	} else {
		return -4;
	}
}
