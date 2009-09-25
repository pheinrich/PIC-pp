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
#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include "devConfig.h"

int programmer_reset(int port);
int programmer_program_mode(int port);
int programmer_echo(int port, char ch);
int programmer_proto_check(int port);
int programmer_chip_in_socket(int port);
int programmer_init(int port, deviceConfig_t devConfig);
int programmer_voltages_on(int port);
int programmer_voltages_off(int port);
int programmer_read_chipId(int port);
int programmer_read_oscal(int port);
int programmer_write_oscal(int port, int oscal, int fuse1);
int programmer_read_rom(int port, unsigned char * romImage, deviceConfig_t devConfig);
int programmer_read_eeprom(int port, unsigned char * eepromImage, deviceConfig_t devConfig);
int programmer_read_config(int port, char * fuseImage, deviceConfig_t devConfig);
int programmer_erase_chip(int port);
int programmer_erase_rom_check(int port, deviceConfig_t devConfig );
int programmer_erase_eeprom_check(int port);
int programmer_write_rom(int port, char *romImage, deviceConfig_t devConfig);
int programmer_write_eeprom(int port, char *eepromImage, deviceConfig_t devConfig);
int programmer_write_fuses(int port, char * fuseImage, deviceConfig_t devConfig);
int programmer_write_18Fxxxxx(int port, char * fuseImage, deviceConfig_t devConfig);
#endif
