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
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#define CONFIG_ERR_CANTOPEN	-1
#define CONFIG_ERR_POWERSEQ	-2
#define CONFIG_ERR_CORETYPE	-3
#define CONFIG_ERR_NOTFOUND	-4

typedef struct {
	int bits;
	int eraseMode;
	int flashChip;
	int calWord;
	int bandGap;
	int powerSequence;
	int programDelay;
	int programTries;
	int programFlags;
	int overProgram;
	int coreType;
	int romSize;
	int romPad;
	int eepromStart;
	int eepromSize;
	int eepromPad;
	int configStart;
	int fuse1Blank;
	int fuse2Blank;
	int fuse3Blank;
	int fuse4Blank;
	int fuse5Blank;
	int fuse6Blank;
	int fuse7Blank;
	int fuseCount;
	int chipID;
} deviceConfig_t;

int config_parse(char * configFile, char * chipName, deviceConfig_t * devConfig);
#endif
