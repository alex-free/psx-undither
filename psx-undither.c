
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "lib-ps-cd-id/lib-ps-cd-id.h"
#include "error_recalc.h"

FILE *input_file;

unsigned int input_file_size;
unsigned int current_fpos = 0;
unsigned int pattern_match_count = 0;
unsigned int valid_mem_dump_size = 0x200000; // The exact file size generated when dumping RAM in the DuckStation emulator.
unsigned int invalid_mem_dump_size = 0x800000; // The exact file size generated when dumping RAM in the DuckStation emulator with the 8MB RAM option checked.
const unsigned int full_raw_2_sectors_len = 0x1260;

unsigned char sectors_buf[0x1260];
unsigned char *buf;
bool matched_dither;

const unsigned char dither[] = {
    0x00,
    0xE1,
    0x00, // Not actually 00, we don't check this byte for what it is
    0x3C,
    0x00,
    0x02 // changes to zero for no-dither patch
};

#ifdef WIN32
	#include <conio.h> // _getch() for system("pause") replacement
#endif

void do_exit(unsigned char exit_code) // system("pause"); sucks we can do better
{
	#if defined WIN32
		printf("\nPress any key to continue...\n");
		_getch();
	#endif
	exit(exit_code);
}

void bin_patch(const char *file_name) // pass argv1 file_name
{
    /*
        The dither related could possibly start on the end of a sector and end at the beginning of the next sector. Each RAW sector is 0x930 bytes. The first 0x18 bytes are to be ignored as they are just header data. The next 0x800 bytes contains actual data we want to scan through.
        Start at 0. Skip to 0x18. Read the next 0x800 bytes. Skip to a total of 0x930 bytes (one whole raw sector). Skip 0x18 bytes again and then read the next 0x800 bytes. We now have 2 sectors worth of straight up data in a buffer of 0x1000 bytes
        Run search functions on the 0x1000 byte sized buffer.
    */

	eccedc_init();

    const int full_raw_sector_len = 0x930;

    unsigned int sector_count = 0;

    const int single_sector_user_data_len = 0x800;
    const int two_sectors_user_data_len = 0x1000;

    unsigned int patched_lba;
    const int pregap = 150;
    int search_size;
    unsigned char sectors[two_sectors_user_data_len];

    bool last_sector = false;

    unsigned int percentage;
    unsigned int total_number_of_sectors = (input_file_size / full_raw_sector_len);

    printf("Scanning %d sectors, please wait...\n", total_number_of_sectors);
    fseek(input_file, 0, SEEK_SET);

    while(1)
    {
        if(current_fpos > input_file_size)
        {
            break; // even number of sectors, done reading the file.
        }
            
        if((current_fpos + full_raw_sector_len) == input_file_size) // odd number of sectors
        {
            last_sector = true; // This function is reading 2 sectors at a time, so if there is an odd number of sectors we have to change the behavior to only search the last sector. Explicitly break loop when this is set.
        }
        
        percentage = ( ( (sector_count + 1) * 100) / total_number_of_sectors); // + 1 to ensure it gets to 100%
        printf("\rProgress: %d%%", percentage); // last sector so don't add + 1
        fflush(stdout); // clear double buffered input so terminal cursor isn't going nuts

        for(int i=0; i < single_sector_user_data_len; i++)
        {
            sectors[i] = sectors_buf[i + 0x18]; // skip 0x18 header info per sector
        }

        if(!last_sector)
        {
            for(int i=0; i < single_sector_user_data_len; i++)
            {
                sectors[i + single_sector_user_data_len] = sectors_buf[i + 0x18 + full_raw_sector_len]; // skip 0x18 header info then skip exactly 1 sector. Read the next 0x800 bytes. We now have an array's worth of data from 2 sectors which excludes EDC/Header data at the beginning and end of each.
            }

            search_size = two_sectors_user_data_len;
        } else {
            search_size = single_sector_user_data_len;
        }

        for(int s = 0; s < search_size; s++)
        {
            matched_dither = true;
            
            for(int i=0; i < 6; i++)
            {                
                if(dither[i] != sectors[s + i])
                {
                   if(i != 2) // The 3rd byte is not matchable so they are 0x00 in the array and not checked here
                    {
                        matched_dither = false;
                    }
                }   
            }

            if(matched_dither)
            {
                if(s < full_raw_sector_len) // if s is above 0x800 we are in the second sector's user data
                {
                    patched_lba = ((current_fpos / full_raw_sector_len) + pregap);
                } else {
                   patched_lba = ((current_fpos / full_raw_sector_len) + pregap + 1);
                }

                sectors[s + 5] = 0x00; // changes byte 6 to zero removes dither  
                printf("\rGot a dither code match starting in sector %d (LBA: %u)\n", (patched_lba - pregap), patched_lba);
                pattern_match_count++;
            }
        }

        if(!last_sector)
        {
            for(int i=0; i < single_sector_user_data_len; i++) // sector 1
            {
                sectors_buf[i + 0x18] = sectors[i]; // skip 0x18 header info per sector
            }

            for(int i=0; i < single_sector_user_data_len; i++) // sector 2
            {
                sectors_buf[i + 0x18 + full_raw_sector_len] = sectors[single_sector_user_data_len + i]; // skip 0x18 header info then skip exactly 1 sector. Read the next 0x800 bytes. We now have an array's worth of data from 2 sectors which excludes EDC/Header data at the beginning and end of each.
            }
        } else {
            for(int i=0; i < single_sector_user_data_len; i++) // sector 1 (fill it)
            {
                sectors_buf[i + 0x18] = sectors[i]; // skip 0x18 header info per sector
            }

            for(int i=0; i < single_sector_user_data_len; i++) // sector 2 (zero it)
            {
                sectors_buf[i + 0x18 + full_raw_sector_len] = 0; // skip 0x18 header info then skip exactly 1 sector. Read the next 0x800 bytes. We now have an array's worth of data from 2 sectors which excludes EDC/Header data at the beginning and end of each.
            }
        }

        if(patched_lba > 0) // Sector user data needs EDC/ECC update, we now definitely have the modified sector's user data in the buffer
        {
            eccedc_generate(&sectors_buf[0]);
            eccedc_generate(&sectors_buf[0x930]);

            if(!last_sector)
            {
                fseek(input_file, -0x1260, SEEK_CUR);
                fwrite(sectors_buf, 1, full_raw_2_sectors_len, input_file);
            } else {
                fseek(input_file, -0x930, SEEK_CUR);
                fwrite(sectors_buf, 1, full_raw_sector_len, input_file);
            }
            printf("Updated EDC/ECC for dither code modification starting in sector %d (LBA: %u)\n", (patched_lba - pregap), patched_lba);
            patched_lba = 0; // will be set on any future pattern match
        }

        if(!last_sector)
        {
            fseek(input_file, -0x930, SEEK_CUR);
            /* 
            must go back a sector to align with current_fpos, we read the disc like this to catch pattern crossing sector boundaries:
            1) read sector 0 and 1, then
            2) read sector 1 and 2, then
            3) read sector 2 and 3, and so on
            */
            fread(sectors_buf, 1, full_raw_2_sectors_len, input_file); // read 2 sectors
        } else {
            break; // that's it, we wrote to the last sector by this point and have no need to continue...
        }

        sector_count++;
        current_fpos = (current_fpos + full_raw_sector_len); // advance one sector (see above disc image reading pattern explanation).
    }

    if(pattern_match_count != 1) 
    {
        printf("\nImage scan complete, got %d dither code matches\n", pattern_match_count);
    } else {
        printf("\nImage scan complete, got 1 dither code match\n");
    }

    if(pattern_match_count > 0)
    {
        printf("\n%s has been modified successfully!\n", file_name);
    } else {
        printf("\n%s has not been modified, nothing patched!\n", file_name);
    }

    fclose(input_file);
}

void gameshark_gen(const char *file_name) // pass argv1 file_name)
{
    unsigned int gameshark_write_byte_address;

    unsigned char *bytes;

    while(1)
    {
        if(current_fpos > input_file_size)
        {
            break;
        }

        matched_dither = true;
        
        for(int i=0; i < 6; i++)
        {                
            if(dither[i] != buf[current_fpos + i])
            {
                if(i != 2) // This byte is not matchable so they are 0x00 in the array and not checked here
                {
                    matched_dither = false;
                }
            }
        }

        if(matched_dither)
        {
            printf("\nFound dither code starting at offset: 0x%08X\n", current_fpos);
            gameshark_write_byte_address = (current_fpos + 4); // offset from start of pattern to end of pattern - 1 (must be even address for gameshark codes)

            bytes=(unsigned char *)&gameshark_write_byte_address;
            bytes[3] = 0xD0;
            printf("\n%08X 0200\n", gameshark_write_byte_address); // Look for 0x00 0x02 (reversed for gs codes compared to cd image)
            bytes=(unsigned char *)&gameshark_write_byte_address;
            bytes[3] = 0x80;        
            gameshark_write_byte_address = bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24); // NOT BIG-ENDIAN SAFE    
            printf("%08X 0000\n", gameshark_write_byte_address); // Replace 2 bytes with 0x00 0x00
            
            current_fpos = (current_fpos + 6); // Skip the next 6 bytes since we know these were part of the previous pattern match
            pattern_match_count++;
        } else {
            current_fpos++; // Move to next byte to start checks all over again until EOF
        }
    }

    if(pattern_match_count == 1)
    {
        printf("\n1 GameShark code has been generated successfully from %s\n", file_name);
    } else if(pattern_match_count > 1) {
        printf("\n%d GameShark codes have been generated successfully from %s!\n", pattern_match_count, file_name);
    } else {
        printf("\n0 GameShark codes were generated, no dither code was found in %s\n", file_name);
    }
}

int main (int argc, const char * argv[]) 
{
    printf("PSX Undither %s By Alex Free (C)2024 (GNU GPLv2)\nhttps://alex-free.github.io/psx-undither\n\n", VERSION);

    if(argc != 2)
    {
        printf("Error: psxund requires a valid argument.\n" 
        "Usage:\n\n"
        "psxund <input file>\n\n<input file>      Can be either the data track bin file of a PlayStation 1 disc image (the sole .bin file or the .bin file named something like track 01), or a DuckStation RAM dump file.\n\n");
        do_exit(1);
    }

    if((input_file = fopen(argv[1], "rb+")) == NULL) // may need to write binary for cd image bin patching
    {
        printf("Error: Cannot open the input file: %s\n", argv[1]);
        do_exit(1);
    }

    fseek(input_file, 0, SEEK_END);
    input_file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    if(input_file_size == invalid_mem_dump_size)
    {
        printf("Mode: GameShark code generation\nError: incorrect memory dump size detected\nDo you have the 'Enable 8MB RAM' option in DuckStation enabled? Uncheck that option if so and make a new RAM dump.\n");
        do_exit(1);
    } else if(input_file_size == valid_mem_dump_size) {
        printf("Mode: GameShark code generation\n");
        buf = (unsigned char *)malloc(input_file_size);
            
        if(buf == NULL) 
        {
            printf("Error allocating memory for DuckStation RAM dump file: %s", argv[1]);
            fclose(input_file);
            do_exit(1);
        }

        if(fread(buf, 1, input_file_size, input_file) != input_file_size)
        {
            printf("Error loading DuckStation RAM dump file: %s\n", argv[1]);
            fclose(input_file);
            do_exit(1);
        }
            
        fclose(input_file);
        printf("Loaded DuckStation RAM dump file: %s successfully\n", argv[1]);
        gameshark_gen(argv[1]);
        free(buf);
    } else {
        int valid = is_ps_cd(input_file);

        if(!valid)
        {
            printf("\n%s does not appears to be a PlayStation 1 CD image or a memory dump from duckstation\n", argv[1]);
            do_exit(1);
        } else if(valid) {
            printf("Mode: CD image BIN patcher\n");

            if(fread(sectors_buf, 1, full_raw_2_sectors_len, input_file) != full_raw_2_sectors_len)
            {
                printf("Error loading CD image bin file: %s\n", argv[1]);
                fclose(input_file);
                do_exit(1);
            }

            bin_patch(argv[1]);
        }
    }

    do_exit(0);
}