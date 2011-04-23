/*
 *  Copyright 2007 Michael Buckley
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the Gnu Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#ifdef MINGW
#include<sys/param.h>
#endif

#include "brnflip.h"

#if defined(WIN32) || defined(DOS)
#define strcasecmp stricmp
#endif

#ifndef EOVERFLOW
#define EOVERFLOW E2BIG
#endif

void printUsage(char* programName);

int main(int argc, char* argv[])
{
    char* input = NULL;
    char* output = NULL;
    megahal_filetype target = MEGAHAL_UNKNOWN_FILETYPE;
    int force = 0;

    {
        int i;
        for(i = 1; i < argc ;++i){
            if(strcmp(argv[i], "-o") == 0){
                if(i + 1 >= argc || output != NULL){
                    printUsage(argv[0]);
                    return 0;
                }else{
                    ++i;
                    output = argv[i];
                }
            }else if(strcmp(argv[i], "--target") == 0){
                if(i + 1 >= argc || target != MEGAHAL_UNKNOWN_FILETYPE){
                    printUsage(argv[0]);
                    return 0;
                }else{
                    ++i;
                    if(strcasecmp(argv[i], "big") == 0){;
                        target = MEGAHAL_BIG_ENDIAN;
                    }else if(strcasecmp(argv[i], "little") == 0){
                        target = MEGAHAL_LITTLE_ENDIAN;
                    }else if(strcasecmp(argv[i], "this") == 0){
                        #if BYTE_ORDER == LITTLE_ENDIAN
                        target = MEGAHAL_LITTLE_ENDIAN;
                        #elif BYTE_ORDER == BIG_ENDIAN
                        target = MEGAHAL_BIG_ENDIAN;
                        #endif
                    }else if(strcasecmp(argv[i], "other") == 0){
                         #if BYTE_ORDER == LITTLE_ENDIAN
                        target = MEGAHAL_BIG_ENDIAN;
                        #elif BYTE_ORDER == BIG_ENDIAN
                        target = MEGAHAL_LITTLE_ENDIAN;
                        #endif
                    }else{
                        printUsage(argv[0]);
                        return 0;
                    }
                }
            }else if(strcmp(argv[i], "--force") == 0){
                force = 1;
            }else{
                if(input != NULL){
                    printUsage(argv[0]);
                    return 0;
                }else{
                    input = argv[i];
                }
            }
        }
    }

    if(input == NULL){
        input = "megahal.brn";
    }

    if(output == NULL){
        output = "megahal.brn";
    }

    if(target == MEGAHAL_UNKNOWN_FILETYPE){
        #if BYTE_ORDER == LITTLE_ENDIAN
        target = MEGAHAL_LITTLE_ENDIAN;
        #elif BYTE_ORDER == BIG_ENDIAN
        target = MEGAHAL_BIG_ENDIAN;
        #endif
    }

    FILE* f = fopen(input, "rb");
    if(f == NULL){
        printf("Unable to open input file: %s\n", input);
        #ifdef WIN32
        puts("Press enter to quit.");
        getc(stdin);
        #endif
        return 0;
    }

    if(fseek(f, 0, SEEK_END) < 0){
        if(errno == EOVERFLOW){
            printf("The input file is too large: %s\n", input);
        }else if(errno == ESPIPE){
            printf("The input file is not a regular file: %s\n", input);
        }else{
            printf("Unknown error opening input file: %s\n", input);
        }

        puts("Contact me for assistance. http://angrymen.org/contact/");
        #ifdef WIN32
        puts("Press enter to quit.");
        getc(stdin);
        #endif
    }

    long brainLen = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buffer = (uint8_t*) malloc(brainLen);
    fread(buffer, brainLen, 1, f);
    fclose(f);

    int error;

    if(force == 1){
        error = forceMegahalConversion(buffer, brainLen);
    }else{
        #if !BYTE_ORDER == BIG_ENDIAN && !BYTE_ORDER == LITTLE_ENDIAN
        puts("Your machine appears to be neither big nor little-endian.");
        puts("This program will only run on big or little-endian machines.");
        puts("You can use the --force option to force a conversion.");
        puts("This will only convert between big and little-endian.");
        puts("I would be glad develop a converter that works on your machine.");
        puts("Please contact me at http://angrymen.org/contact/");
        return 0;
        #else
        error = convertMegahalBrain(buffer, brainLen, target);
        #endif
    }

    if(error != 0){
        if(error == MEGAHAL_UNKNOWN_INPUT){
            printf("Input file does not appear to be a brain: %s\n", input);
        }else{
            puts("Unknown error.");
        }
        puts("Contact me for assistance. http://angrymen.org/contact/");
        #ifdef WIN32
        puts("Press enter to quit.");
        getc(stdin);
        #endif
        return 0;
    }

    f = fopen(output, "w");
    if(f == NULL){
        printf("Unable to open output file: %s\n", output);
        #ifdef WIN32
        puts("Press enter to quit.");
        getc(stdin);
        #endif
        return 0;
    }

    fwrite(buffer, brainLen, 1, f);
    fclose(f);

    puts("Conversion completed successfully.");
    #ifdef WIN32
    puts("Press enter to quit.");
    getc(stdin);
    #endif
    return 0;
}

void printUsage(char* programName)
{
    printf("Usage: %s [input] [-o output] [--target target]\n", programName);
    puts("Each parameter may only be specified once.");
    puts("Input and output are the filenames of the input and output files.");
    puts("Target is the target endianess. It defaults to your machine's.");
    puts("Supported targets are:");
    puts("\tbig\tbig-endian");
    puts("\tlittle\tlittle-endian");
    puts("\tthis\tyour machine's endianess");
    puts("\tother\tthe opposite of your machine's endianess");
    #ifdef WIN32
    puts("Press enter to quit.");
    getc(stdin);
    #endif
}
