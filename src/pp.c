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
#include <unistd.h>
#include <sysexits.h>
#include <termios.h>
#include "serial.h"
#include "devConfig.h"
#include "debug.h"
#include "programmer.h"
#include "ihx32.h"

void usage(char * const argv[], int full_usage) {
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [-c <file>] -d <device> [-p <file>] -r <file>\n",argv[0]);
	fprintf(stderr,"\t%s [-c <file>] -d <device> [-p <file>] [-fC] -w <file>\n",argv[0]);
	fprintf(stderr,"\t%s [-c <file>] -d <device> [-p <file>] -v <file>\n",argv[0]);
	fprintf(stderr,"\t%s [-c <file>] -d <device> [-p <file>] -e\n",argv[0]);
	if (full_usage) {
		fprintf(stderr,"\nOptions:\n");
		fprintf(stderr,"\t-C\tWrite calibration word stored in the file (default: preserve\n\t\tcalibration word stored on device\n\n");
		fprintf(stderr,"\t-c <file>\n\t\tRead configuration from <file> (default: "SYSCONFDIR"/pp.conf)\n\n");
		fprintf(stderr,"\t-d <device>\n\t\tSpecifies <device> as the PIC being operated on (ex: 16F777)\n\n");
		fprintf(stderr,"\t-e\tErase the device (ROM, EEPROM, FUSEs)\n\n");
		fprintf(stderr,"\t-f\tForce writing to the device\n\n");
		fprintf(stderr,"\t-p <file>\n\t\tUse <file> as the serial port on which the programmer\n\t\tis attached\n\n");
		fprintf(stderr,"\t-r <file>\n\t\tRead from the device into <file>\n\n");
		fprintf(stderr,"\t-w <file>\n\t\tWrite from <file> to the device\n\n");
		fprintf(stderr,"\t-v <file>\n\t\tVerify the contents of the device match those of <file>\n\n");
	}
}

int main(int argc, char * const argv[]) {
	char optChar;
	char errmsg[255];
	char hexFile[255];
	char serialPort[255];
	char device[255];
	char configFile[255];
	char buffer[255];
	unsigned char *romImage = NULL;
	unsigned char *eepromImage = NULL;
	unsigned char fuseImage[22];
	unsigned char *verifyRomImage = NULL;
	unsigned char *verifyEepromImage = NULL;
	unsigned char verifyFuseImage[22];
	unsigned int calWord;
	int serialFd;
	FILE * hexFd;
	int op_read = 0, op_write = 0, op_verify = 0, op_erase = 0;
	int op_write_force = 0, op_write_cal = 0;
	int ii;
	int ret;
	int devChipId;
	deviceConfig_t devConfig;

	// Init default values
	strcpy(serialPort,"/dev/ttyS0");
	strcpy(configFile,SYSCONFDIR"/pp.conf");

	// Parse command line options
	while ((optChar = getopt(argc, argv, "Cc:d:efhp:r:w:v:")) != -1) {
		switch (optChar) {
			case 'C': // Calibration word
				op_write_cal = 1;
				break;
			case 'c': // Config file
				strncpy(configFile,optarg,255);
				break;
			case 'd': // Device
				strncpy(device,optarg,255);
				break;
			case 'e': // Erase chip
				op_erase = 1;
				break;
			case 'f': // Force write
				op_write_force = 1;
				break;
			case 'h': // Help
				usage(argv,1);
				return EX_USAGE;
				break;
			case 'p': // Port
				strncpy(serialPort,optarg,255);
				break;
			case 'r': // Read
				op_read = 1;
				strncpy(hexFile,optarg,255);
				break;
			case 'w': // Write
				op_write = 1;
				strncpy(hexFile,optarg,255);
				break;
			case 'v': // Verify
				op_verify = 1;
				strncpy(hexFile,optarg,255);
				break;
			case '?':
				usage(argv,0);
				return EX_USAGE;
				break;
		}
	}

	// If no op was specified, give them usage
	if (!(op_read || op_write || op_verify || op_erase)) {
		usage(argv,0);
		return EX_USAGE;
	}

	// Get the config for the device specified
	if ((ret = config_parse(configFile, device, &devConfig)) < 0) {
		switch (ret) {
			case CONFIG_ERR_CANTOPEN:
				sprintf(errmsg,"Error opening %s",configFile);
				perror(errmsg);
				break;
			case CONFIG_ERR_POWERSEQ:
				fprintf(stderr,"Error: Invalid power sequence for device\n");
				break;
			case CONFIG_ERR_CORETYPE:
				fprintf(stderr,"Error: Invalid core type for device\n");
				break;
			case CONFIG_ERR_NOTFOUND:
				fprintf(stderr,"Error: No config for device in config file\n");
				break;
		}

		return EX_DATAERR;
	}


#if defined(DEBUG)
	debug_print_config(devConfig);
#endif

	// Initialize space for ROM, EEPROM, and config images
	// Note: romSize and eepromSize are in words
	//       (1 word = 2 bytes)
	romImage = (unsigned char *)malloc(sizeof(unsigned char) * devConfig.romSize * 2);
	for (ii = devConfig.romSize*2 - 2; ii >= 0; ii -= 2) {
		romImage[ii] = devConfig.romPad >> 8;
		romImage[ii+1] = devConfig.romPad & 0xFF;
	}

	if (devConfig.eepromSize > 0) {
		eepromImage = (unsigned char *)malloc(sizeof(unsigned char) * devConfig.eepromSize);
		memset(eepromImage, devConfig.eepromPad, sizeof(unsigned char) * devConfig.eepromSize);
	}

	// fuseImage contains up to 8 FUSE words plus the 8 ID bytes
	fuseImage[0] = ' ';
	fuseImage[1] = ' ';
	fuseImage[2] = ' ';
	fuseImage[3] = ' ';
	fuseImage[4] = ' ';
	fuseImage[5] = ' ';
	fuseImage[6] = ' ';
	fuseImage[7] = ' ';
	fuseImage[8] = devConfig.fuse1Blank >> 8;
	fuseImage[9] = devConfig.fuse1Blank & 0xFF;
	if (devConfig.fuseCount == 2 || devConfig.fuseCount == 7) {
		fuseImage[10] = devConfig.fuse2Blank & 0xFF;
		fuseImage[11] = devConfig.fuse2Blank >> 8;
	}
	if (devConfig.fuseCount == 7) {
		fuseImage[12] = devConfig.fuse3Blank & 0xFF;
		fuseImage[13] = devConfig.fuse3Blank >> 8;
		fuseImage[14] = devConfig.fuse4Blank & 0xFF;
		fuseImage[15] = devConfig.fuse4Blank >> 8;
		fuseImage[16] = devConfig.fuse5Blank & 0xFF;
		fuseImage[17] = devConfig.fuse5Blank >> 8;
		fuseImage[18] = devConfig.fuse6Blank & 0xFF;
		fuseImage[19] = devConfig.fuse6Blank >> 8;
		fuseImage[20] = devConfig.fuse7Blank & 0xFF;
		fuseImage[21] = devConfig.fuse7Blank >> 8;
	}

	DEBUG_p("DEBUG: Attempting to open serial port %s\n",serialPort);

	// Open the serial port
	if ((serialFd = serial_open(serialPort,19200,8,PARITY_NONE,1)) < 0) {
		switch(serialFd) {
			case SERIAL_BAUDRATE:
				fprintf(stderr,"Error: Invalid baud rate");
				break;
			case SERIAL_STOPBITS:
				fprintf(stderr,"Error: Invalid number of stop bits");
				break;
			case SERIAL_DATABITS:
				fprintf(stderr,"Error: Invalid number of data bits");
				break;
			case SERIAL_CANTOPEN:
				sprintf(errmsg,"Error opening %s",serialPort);
				perror(errmsg);
				break;
		}
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: Opened serial port %s\n",serialPort);

	// For some reason there is always a left-over byte
	read(serialFd,buffer,255);

	// Attempt to send a "reset"
	if (programmer_reset(serialFd) != 0) {
		printf("Error: failed to reset board\n");
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: Device reset\n");

	// Switch to program mode
	if (programmer_program_mode(serialFd) != 0) {
		printf("Error: failed to enter program mode\n");
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: Entered command mode\n");

	// Check protocol number
	if (programmer_proto_check(serialFd) != 0) {
		printf("Error: wrong protocol version\n");
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG Correct protocol\n");

	// Check if chip is in socket
	if (programmer_chip_in_socket(serialFd) != 0) {
		printf("Error: no chip connected\n");
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: Chip is in socket\n");

	// Switch to program mode
	if (programmer_program_mode(serialFd) != 0) {
		printf("Error: failed to enter program mode\n");
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: Entered program mode\n");

	// Attempt to initialize the programmer
	if (programmer_init(serialFd,devConfig) != 0) {
		printf("Error: failed to initialize programmer\n");
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: Programmer initialized\n");

	// Check to make sure that the chip in the programmer
	// is the device specified
	programmer_voltages_on(serialFd);
	devChipId = programmer_read_chipId(serialFd);
	programmer_voltages_off(serialFd);

	if (devChipId < 0) {
		printf("Error: failed to read configuration data\n");
		return EX_SOFTWARE;
	}

	if (devChipId != devConfig.chipID) {
		printf("Error: chip in programmer is not a %s\n", device);
		return EX_SOFTWARE;
	}

	DEBUG_p("DEBUG: ChipID matched specified device\n");

	if (op_read) {
		if ((hexFd = fopen(hexFile, "w")) < 0) {
			sprintf(errmsg, "Error opening %s:", hexFile);
			perror(errmsg);
			return EX_NOINPUT;
		}

		printf("Reading from a %s into %s...\n",device,hexFile);

		programmer_voltages_on(serialFd);
		programmer_read_rom(serialFd, romImage, devConfig);
		if (devConfig.eepromSize > 0) {
			programmer_read_eeprom(serialFd, eepromImage, devConfig);
		}
		programmer_read_config(serialFd, fuseImage, devConfig);
		programmer_voltages_off(serialFd);

		//#if defined(DEBUG)
		debug_rom_dump(romImage, devConfig);
		debug_eeprom_dump(eepromImage, devConfig);
		debug_config_dump(fuseImage, devConfig);
		//#endif

		ihx32_write(hexFd,romImage,eepromImage,fuseImage,devConfig);

	} else if (op_write) {
		if ((hexFd = fopen(hexFile, "r")) < 0) {
			sprintf(errmsg, "Error opening %s", hexFile);
			perror(errmsg);
			return EX_NOINPUT;
		}

		printf("Writting to a %s from %s...\n",device,hexFile);

		if (ihx32_read(hexFd,romImage,eepromImage,fuseImage,devConfig) != 0) {
			printf("ihx32_read aborted\n");
		}

		fclose(hexFd);

		if (op_write_force == 0) {
			// Check for a blank ROM
			programmer_voltages_on(serialFd);
			ret = programmer_erase_rom_check(serialFd, devConfig);

			if (ret < 0) {
				printf("Error: failed to check for erased ROM\n");
				return EX_SOFTWARE;
			} else if (ret == 1) {
				printf("Error: ROM is not erased, write will not be performed.  Override with -f.\n");
				return EX_SOFTWARE;
			}

         ret = programmer_echo(serialFd, 'V');
         if( 0 > ret ) {
            printf("Error: failed echo\n");
            return EX_SOFTWARE;
         }
   
			// Check for a blank EEPROM
			ret = programmer_erase_eeprom_check(serialFd);

			if (ret < 0) {
				printf("Error: failed to check for erased EEPROM\n");
				return EX_SOFTWARE;
			} else if (ret == 1) {
				printf("Error: EEPROM is not erased, write will not be performed.  Override with -f.\n");
				return EX_SOFTWARE;
			}

         ret = programmer_echo(serialFd, 'v');
         if( 0 > ret ) {
            printf("Error: failed echo\n");
            return EX_SOFTWARE;
         }
         ret = programmer_echo(serialFd, 'V');
         if( 0 > ret ) {
            printf("Error: failed echo\n");
            return EX_SOFTWARE;
         }
         programmer_voltages_off(serialFd);
		}


		if (devConfig.calWord == 1 && op_write_cal == 0) {
			// Grab the calibration data
			programmer_voltages_on(serialFd);
			calWord = programmer_read_oscal(serialFd);
			programmer_voltages_off(serialFd);

			if (calWord < 0) {
				printf("Error: failed to read calibration data\n");
			} else {
				DEBUG_p("DEBUG: calWord=%04X\n",calWord);
				romImage[devConfig.romSize*2-2] = (calWord >> 8) & 0xFF;
				romImage[devConfig.romSize*2-1] = calWord & 0xFF;
			}
			
		}

#if defined(DEBUG)
		debug_rom_dump(romImage, devConfig);
		debug_eeprom_dump(eepromImage, devConfig);
		debug_config_dump(fuseImage, devConfig);
#endif

		programmer_voltages_on(serialFd);
		ret = programmer_write_rom(serialFd,romImage,devConfig);
		if (ret < 0) {
			printf("Error: failed to write ROM\n");
			return EX_SOFTWARE;
		}

      ret = programmer_echo(serialFd, 'V');
      if( 0 > ret ) {
         printf("Error: failed echo\n");
         return EX_SOFTWARE;
      }

		if (devConfig.eepromSize > 0) {
			ret = programmer_write_eeprom(serialFd,eepromImage,devConfig);
			if (ret < 0) {
				printf("Error: failed to write EEPROM\n");
			}
		}

      ret = programmer_echo(serialFd, 'V');
      if( 0 > ret ) {
         printf("Error: failed echo\n");
         return EX_SOFTWARE;
      }

		ret = programmer_write_fuses(serialFd,fuseImage,devConfig);
		if (ret < 0) {
			printf("Error: failed to write config\n");
         return EX_SOFTWARE;
		}
      
      ret = programmer_echo(serialFd, 'V');
      if( 0 > ret ) {
         printf("Error: failed echo\n");
         return EX_SOFTWARE;
      }
      
      ret = programmer_write_18Fxxxxx(serialFd,fuseImage,devConfig);
		programmer_voltages_off(serialFd);
		if (ret < 0) {
			printf("Error: failed to write FUSEs\n");
         return EX_SOFTWARE;
		}

	} else if (op_verify) {
		printf("Verifying the %s contains %s...\n",device,hexFile);

		// Initialize space for ROM, EEPROM, and config images
		// Note: romSize and eepromSize are in words
		//       (1 word = 2 bytes)
		verifyRomImage = (unsigned char *)malloc(sizeof(unsigned char) * devConfig.romSize * 2);
		for (ii = devConfig.romSize*2 - 2; ii >= 0; ii -= 2) {
			verifyRomImage[ii] = 0x3F;
			verifyRomImage[ii+1] = 0xFF;
		}

		if (devConfig.eepromSize > 0) {
			verifyEepromImage = (unsigned char *)malloc(sizeof(unsigned char) * devConfig.eepromSize);
			memset(verifyEepromImage, devConfig.eepromPad, sizeof(unsigned char) * devConfig.eepromSize);
		}

		// fuseImage contains up to 8 FUSE words plus the 8 ID bytes
		verifyFuseImage[0] = ' ';
		verifyFuseImage[1] = ' ';
		verifyFuseImage[2] = ' ';
		verifyFuseImage[3] = ' ';
		verifyFuseImage[4] = ' ';
		verifyFuseImage[5] = ' ';
		verifyFuseImage[6] = ' ';
		verifyFuseImage[7] = ' ';
		verifyFuseImage[8] = devConfig.fuse1Blank >> 8;
		verifyFuseImage[9] = devConfig.fuse1Blank & 0xFF;
		if (devConfig.fuseCount == 2 || devConfig.fuseCount == 7) {
			verifyFuseImage[10] = devConfig.fuse2Blank >> 8;
			verifyFuseImage[11] = devConfig.fuse2Blank & 0xFF;
		}
		if (devConfig.fuseCount == 7) {
			verifyFuseImage[12] = devConfig.fuse3Blank >> 8;
			verifyFuseImage[13] = devConfig.fuse3Blank & 0xFF;
			verifyFuseImage[14] = devConfig.fuse4Blank >> 8;
			verifyFuseImage[15] = devConfig.fuse4Blank & 0xFF;
			verifyFuseImage[16] = devConfig.fuse5Blank >> 8;
			verifyFuseImage[17] = devConfig.fuse5Blank & 0xFF;
			verifyFuseImage[18] = devConfig.fuse6Blank >> 8;
			verifyFuseImage[19] = devConfig.fuse6Blank & 0xFF;
			verifyFuseImage[20] = devConfig.fuse7Blank >> 8;
			verifyFuseImage[21] = devConfig.fuse7Blank & 0xFF;
		}

		// Open the file
		if ((hexFd = fopen(hexFile, "r")) < 0) {
			sprintf(errmsg, "Error opening %s:", hexFile);
			perror(errmsg);
			return EX_NOINPUT;
		}

		// Read the contents
		if (ihx32_read(hexFd,verifyRomImage,verifyEepromImage,verifyFuseImage,devConfig) != 0) {
			printf("ihx32_read aborted\n");
		}

		fclose(hexFd);

		// Read the device contents
		programmer_voltages_on(serialFd);
		programmer_read_rom(serialFd, romImage, devConfig);
		if (devConfig.eepromSize > 0) {
			programmer_read_eeprom(serialFd, eepromImage, devConfig);
		}
		programmer_read_config(serialFd, fuseImage, devConfig);
		programmer_voltages_off(serialFd);

#if defined(DEBUG)
		debug_rom_dump(romImage, devConfig);
		debug_eeprom_dump(eepromImage, devConfig);
		debug_config_dump(fuseImage, devConfig);
#endif

		// Compare the ROMs
		for (ii = devConfig.romSize*2 - 2; ii >= 0; ii -= 2) {
			if (romImage[ii] != verifyRomImage[ii] || romImage[ii+1] != verifyRomImage[ii+1]) {
				fprintf(stderr,"Difference found in ROM\n");
				fprintf(stderr,"Device word 0x%x: 0x%02x%02x\n",ii,romImage[ii], romImage[ii+1]);
				fprintf(stderr,"File word 0x%x: 0x%02x%02x\n",ii,verifyRomImage[ii], verifyRomImage[ii+1]);
				return 1;
			}
		}

		// Compare the EEPROMs
		if (devConfig.eepromSize > 0) {
			for (ii = devConfig.eepromSize - 1; ii >= 0; ii--) {
				if (eepromImage[ii] != verifyEepromImage[ii]) {
					fprintf(stderr,"EEPROM difference found:\n");
					fprintf(stderr,"Device byte 0x%x: 0x%02x\n",ii,eepromImage[ii]);
					fprintf(stderr,"File byte 0x%x: 0x%02x\n",ii,verifyEepromImage[ii]);
					return 2;
				}
			}
		}

		// Compare FUSEs
		for (ii = 0; ii < devConfig.fuseCount; ii++) {
			if (fuseImage[8 + ii] != verifyFuseImage[8 + ii] ||
			    fuseImage[8 + ii + 1] != verifyFuseImage[8 + ii + 1]) {
				fprintf(stderr,"FUSE difference found:\n");
				fprintf(stderr,"Device FUSE %d: 0x%02x%02x\n",ii,fuseImage[8 + ii], fuseImage[9 + ii]);
				fprintf(stderr,"File FUSE %d: 0x%02x%02x\n",ii,verifyFuseImage[8 + ii], verifyFuseImage[9 + ii]);
				return 3;
			}
		}
	
	} else if (op_erase) {
		if (devConfig.flashChip != 1) {
			printf("Error: Device selected is not a FLASH device\n");
			printf("Error: It cannot be erased by the programmer\n");
			return EX_NOINPUT;
		}

		printf("Erasing device...\n");

		if (devConfig.calWord == 1) {
			// Grab the calibration data
			programmer_voltages_on(serialFd);
			calWord = programmer_read_oscal(serialFd);
			programmer_voltages_off(serialFd);

			if (calWord < 0) {
				printf("Error: failed to read calibration data\n");
			} else {
				DEBUG_p("DEBUG: calWord=%04X\n",calWord);
				romImage[devConfig.romSize*2-2] = (calWord >> 8) & 0xFF;
				romImage[devConfig.romSize*2-1] = calWord & 0xFF;
			}
			
		}

		programmer_voltages_on(serialFd);
		ret = programmer_erase_chip(serialFd);
		programmer_voltages_off(serialFd);


		if (ret != 0) {
			printf("Error: unable to erase chip\n");
		}

		if(devConfig.calWord == 1) {
			// Write the calibration word
			programmer_voltages_on(serialFd);
			ret = programmer_write_oscal(serialFd,calWord,devConfig.fuse1Blank);
			programmer_voltages_off(serialFd);

			if (ret < 0) {
				printf("Error: failed to write calibration data\n");
			}
		}
	};

	printf("Success\n");
	return EX_OK;
}
