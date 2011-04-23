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
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef MINGW
#include<sys/param.h>
#endif

#include "brnflip.h"

uint32_t traverseMegahalTree(uint8_t* brain, long start, long bound);
long getMegahalDictStart(uint8_t* brain, long brainLen);
void flipMegahalBrain(uint8_t* brain, long brainLen, long dictStart);

void flip16InPlace(uint8_t* x);
void flip32InPlace(uint8_t* x);

int convertMegahalBrain(uint8_t* brain, long brainLen, megahal_filetype target)
{
    #if !BYTE_ORDER == BIG_ENDIAN && !BYTE_ORDER == LITTLE_ENDIAN
    return MEGAHAL_UNSUPPORTED_ENDIANESS;
    #endif

    if(brain[9] != 5){
        return MEGAHAL_UNKNOWN_INPUT;
    }

    char cookie[10];
    memset(cookie, 0, 10);
    memcpy(cookie, brain, 9);
    if(strcmp(cookie, "MegaHALv8") != 0){
        return MEGAHAL_UNKNOWN_INPUT;
    }

    megahal_filetype type = MEGAHAL_UNKNOWN_FILETYPE;

    long dictStart = getMegahalDictStart(brain, brainLen);

    if(dictStart <= 0){
        return MEGAHAL_UNKNOWN_INPUT;
    }

    /* Determine endianess of the input */
    uint32_t dictLength;
    memcpy(&dictLength, brain + dictStart, 4);
    uint32_t flippedDictLength = dictLength;
    flip32InPlace((uint8_t*) &flippedDictLength);

    if(dictLength != flippedDictLength){
        long i = dictStart + 4;
        uint32_t numWords = 0;
        uint8_t wordLen = 0;
        while(i < brainLen){
            wordLen = brain[i];
            ++numWords;
            i += wordLen + 1;
        }

        if(numWords == dictLength){
            #if BYTE_ORDER == BIG_ENDIAN
            type = MEGAHAL_BIG_ENDIAN;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            type = MEGAHAL_LITTLE_ENDIAN;
            #endif
        }else if(numWords == flippedDictLength){
            #if BYTE_ORDER == BIG_ENDIAN
            type = MEGAHAL_LITTLE_ENDIAN;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            type = MEGAHAL_BIG_ENDIAN;
            #endif
        }
    }else{
        /* This is a rare occurance, so go out of your way to test it. */
        long position = traverseMegahalTree(brain, 10, dictStart);
        if(position >= 0){
            position = traverseMegahalTree(brain, position, dictStart);
            if(position >= 0){
                if(position == dictStart){
                    /* Not a 100% guarantee, but very unlikely to be
                        * incorrect in the real world. */
                    #if BYTE_ORDER == BIG_ENDIAN
                    type = MEGAHAL_BIG_ENDIAN;
                    #elif BYTE_ORDER == LITTLE_ENDIAN
                    type = MEGAHAL_LITTLE_ENDIAN;
                    #endif
                }else{
                    #if BYTE_ORDER == BIG_ENDIAN
                    type = MEGAHAL_LITTLE_ENDIAN;
                    #elif BYTE_ORDER == LITTLE_ENDIAN
                    type = MEGAHAL_BIG_ENDIAN;
                    #endif
                }
            }
        }

        if(position < 0){
            #if BYTE_ORDER == BIG_ENDIAN
            type = MEGAHAL_LITTLE_ENDIAN;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            type = MEGAHAL_BIG_ENDIAN;
            #endif
        }
    }

    if(type == target){
        return 0;
    }else if(type == MEGAHAL_UNKNOWN_FILETYPE){
        return MEGAHAL_UNKNOWN_INPUT;
    }else{
        flipMegahalBrain(brain, brainLen, dictStart);
    }

    return 0;
}

int forceMegahalConversion(uint8_t* brain, long brainLen)
{
    long dictStart = getMegahalDictStart(brain, brainLen);

    if(dictStart <= 0){
        return MEGAHAL_UNKNOWN_INPUT;
    }

    flipMegahalBrain(brain, brainLen, dictStart);

    return 0;
}

/* This function attempts to traverse a MegaHALv8 tree. If the tree extends past
 * bound, the function returns -1, otherwise, it returns the position in the
 * brain of the end of the tree. MegaHALv8 brains have two trees. If the second
 * ends at the start of the dictionary, than it is extremely likely that the
 * brain is of the same endianess as the machine executing this function.
 */

uint32_t traverseMegahalTree(uint8_t* brain, long start, long bound)
{
    if(start + 10 > bound)
    {
        return -1;
    }

    uint16_t numBranches;
    memcpy(&numBranches, brain + start + 8, 2);
    long newStart = start + 10;

    uint32_t i;
    for(i = 0; i < numBranches; ++i){
        newStart = traverseMegahalTree(brain, newStart, bound);
        if(newStart < 0){
            break;
        }
    }

    return newStart;
}

/* This function finds the start of the MegaHALv8 dictionary. It takes advantage
 * of the fact that the dictionary starts with "<ERROR>".
 */

long getMegahalDictStart(uint8_t* brain, long brainLen)
{
    long dictStart = brainLen;
    char c = brain[dictStart];
    int foundStart = 0;
    while(foundStart == 0 && dictStart >= 0){
        /* Search for the start of the dictionary */
        if(c == 7){
            char test[8];
            memset(test, 0, 8);
            memcpy(test, &brain[dictStart + 1], 7);
            if(strcmp(test, "<ERROR>") == 0){
                foundStart = 1;
            }
        }
        if(foundStart == 0){
            --dictStart;
            c = brain[dictStart];
        }
    }

    return dictStart - 4;
}

/* This function performs the flipping of the MegaHALv8 brain. Since the
 * dictionary must not be flipped, this function requires the start position
 * of the dictonary.
 */
void flipMegahalBrain(uint8_t* brain, long brainLen, long dictStart)
{
    long position = 10;
    while(position < dictStart){
        flip16InPlace(brain + position);
        flip32InPlace(brain + position + 2);
        flip16InPlace(brain + position + 6);
        flip16InPlace(brain + position + 8);
        position += 10;
    }
    flip32InPlace(brain + position);
}

void flip16InPlace(uint8_t* x)
{
    uint8_t temp = x[0];
    x[0] = x[1];
    x[1] = temp;
}

void flip32InPlace(uint8_t* x){
    char temp[4];
    memcpy(&temp, x, 4);
    x[0] = temp[3];
    x[1] = temp[2];
    x[2] = temp[1];
    x[3] = temp[0];
}
