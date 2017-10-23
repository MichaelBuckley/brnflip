/*
 *  Copyright 2007-2017 Michael Buckley
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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

void print_usage(char* program_name);

int main(int argc, char* argv[]) {
    char* input = NULL;
    char* output = NULL;
    megahal_filetype target = unknown_filetype;
    int force = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc || output != NULL) {
                print_usage(argv[0]);
                return 0;
            } else {
                ++i;
                output = argv[i];
            }
        }else if(strcmp(argv[i], "--target") == 0){
            if(i + 1 >= argc || target != unknown_filetype){
                print_usage(argv[0]);
                return 0;
            } else {
                ++i;
                if (strcasecmp(argv[i], "big") == 0) {;
                    target = big_endian;
                } else if(strcasecmp(argv[i], "little") == 0) {
                    target = little_endian;
                } else if (strcasecmp(argv[i], "this") == 0) {
                    #if BYTE_ORDER == LITTLE_ENDIAN
                    target = little_endian;
                    #elif BYTE_ORDER == BIG_ENDIAN
                    target = bit_endian;
                    #endif
                } else if (strcasecmp(argv[i], "other") == 0) {
                     #if BYTE_ORDER == LITTLE_ENDIAN
                    target = big_endian;
                    #elif BYTE_ORDER == BIG_ENDIAN
                    target = little_endian;
                    #endif
                } else {
                    print_usage(argv[0]);
                    return 0;
                }
            }
        } else if(strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            if (input != NULL) {
                print_usage(argv[0]);
                return 0;
            } else {
                input = argv[i];
            }
        }
    }

    if (input == NULL) {
        input = "megahal.brn";
    }

    if (output == NULL) {
        output = "megahal.brn";
    }

    if(target == unknown_filetype) {
        #if BYTE_ORDER == LITTLE_ENDIAN
        target = little_endian;
        #elif BYTE_ORDER == BIG_ENDIAN
        target = BIG_ENDIAN;
        #endif
    }

    FILE* f = fopen(input, "rb");

    if (f == NULL) {
        fprintf(stderr, "Unable to open input file: %s\n", input);
        return 0;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        if (errno == EOVERFLOW) {
            fprintf(stderr, "The input file is too large: %s\n", input);
        } else if (errno == ESPIPE) {
            fprintf(stderr, "The input file is not seekable: %s\n", input);
        } else {
            fprintf(stderr, "Unknown error opening input file: %s\n", input);
        }
    }

    long brainLen = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*) malloc(brainLen);
    fread(buffer, brainLen, 1, f);
    fclose(f);

    brnflip_error error;

    if (force == 1) {
        error = brnflip_flip_buffer(buffer, brainLen);
    }else{
        #if !BYTE_ORDER == BIG_ENDIAN && !BYTE_ORDER == LITTLE_ENDIAN
        perror("Your machine appears to be neither big nor little-endian.\n");
        perror("This program will only run on big or little-endian machines.\n");
        perror("You can use the --force option to force a conversion.\n");
        perror("This will only convert between big and little-endian.\n");
        return 0;
        #else
        megahal_filetype endianess;
        error = brnflip_detect_endianess(buffer, brainLen, &endianess);
        if (endianess != target) {
            error = brnflip_flip_buffer(buffer, brainLen);
        }
        #endif
    }

    switch (error) {
        case no_error:
            break;

        case invalid_file:
            fprintf(stderr, "Input file does not appear to be a brain: %s\n", input);
            return 0;
    }

    f = fopen(output, "wb");

    if (f == NULL) {
        fprintf(stderr, "Unable to open output file: %s\n", output);
        return 0;
    }

    fwrite(buffer, brainLen, 1, f);
    fclose(f);

    perror("Conversion completed successfully.\n");
    return 0;
}

void print_usage(char* program_name) {
    printf("Usage: %s [input] [-o output] [--target target] [--force]\n", program_name);

    puts("Each parameter may only be specified once.");
    puts("Input and output are the filenames of the input and output files.");
    puts("Target is the target endianess. It defaults to your machine's.");
    puts("Supported targets are:");
    puts("\tbig\tbig-endian");
    puts("\tlittle\tlittle-endian");
    puts("\tthis\tyour machine's endianess");
    puts("\tother\tthe opposite of your machine's endianess");
}
