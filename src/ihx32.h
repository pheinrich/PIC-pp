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
#ifndef IHX32_H
#define IHX32_H
#include "devConfig.h"

int ihx32_read(FILE * hexFile, unsigned char * romImage, unsigned char * eepromImage, unsigned char * fuseImage, deviceConfig_t config);
int ihx32_write(FILE * hexFile, unsigned char * romImage, unsigned char * eepromImage, unsigned char * fuseImage, deviceConfig_t config);
#endif
