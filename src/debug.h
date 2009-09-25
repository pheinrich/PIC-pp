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
#ifndef DEBUG_H
#define DEBUG_H

#include "devConfig.h"

#ifdef DEBUG
#define DEBUG_p(x...) printf(x)
#else
#define DEBUG_p(x...)
#endif

void debug_print_config(deviceConfig_t dev);

void debug_rom_dump(char * romImage, deviceConfig_t devConfig);
void debug_eeprom_dump(char * eepromImage, deviceConfig_t devConfig);
void debug_config_dump(char * fuseImage, deviceConfig_t devConfig);

#endif
