/*
 *  Copyright 2007-2011 Michael Buckley
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

/* Megahal Brain Format
 *
 * Megahal brain files are comprised of a header, two trees and a dictionary
 * (as in a list of words, not an associative array). The two trees have the
 * same format, and represent the model of the brain.
 *
 * Header
 *
 * Type     | Name        |  Description
 *----------+-------------+----------------------------------------------------
 * char[9]  | cookie      | "Megahalv8", not null terminated
 * char     | model order | This is always 5.
 *
 * Tree Nodes
 *
 * Type     | Name        |  Description
 *----------+-------------+----------------------------------------------------
 * uint16_t | symbol      | The index in the dictionary of the word for this
 *          |             | node
 * uint32_t | usage       | Node data
 * uint16_t | count       | Node data
 * uint16_t | branch      | The number of child nodes of this node. The child
 *          |             | nodes are written immediately following this node.
 *
 * Dictionary
 *
 * The dictioanry consists of a single uint32_t count of the number of words in
 * the dictionary, followed by the words themselves. This list always begins
 * with "<ERROR>". These words are not stored as NULL-terminated strings, but
 * as pascal strings. The first byte is the length of the string, and is
 * followed by that many characters.
 */

#define MEGAHAL_HEADER_LENGTH 10

#define MEGAHAL_TREE_NODE_LENGTH sizeof(uint16_t) * 3 + sizeof(uint32_t)
#define MEGAHAL_MIN_TREE_LENGTH  MEGAHAL_TREE_NODE_LENGTH

#define MEGAHAL_FIRST_DICTIONARY_WORD        "<ERROR>"
#define MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH strlen("<ERROR>")
#define MEGAHAL_MIN_DICTIONARY_LENGTH        sizeof(uint32_t) * 2 + \
    MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH

#define MEGAHAL_MIN_BRAIN_LENGTH MEGAHAL_HEADER_LENGTH + \
    MEGAHAL_MIN_TREE_LENGTH * 2 + \
    MEGAHAL_MIN_DICTIONARY_LENGTH

#define MEGAHAL_COOKIE_LENGTH 9

void brnflip_flip_16_in_place(char* x);
void brnflip_flip_32_in_place(char* x);

brnflip_error brnflip_verify_header(char* brain, size_t brainLength);

brnflip_error brnflip_find_dictionary_offset(
    char*  brain,
    size_t brainLength,
    off_t* dictionaryOffset
);

brnflip_error brnflip_count_words_in_dictionary(
    char*    brain,
    size_t   brainLength,
    off_t    dictionaryOffset,
    int32_t* numWordsInDictionary
);

brnflip_error brnflip_traverse_tree(
    char*  brain,
    size_t brainLength,
    off_t* position,
    int    assumeFlipped
);

brnflip_error brnflip_detect_endianess(
    char*             brain,
    size_t            brainLength,
    megahal_filetype* outFiletype
)
{
    off_t    position                = MEGAHAL_HEADER_LENGTH;
    off_t    dictionaryOffset        = 0;
    uint32_t dictionaryLength        = 0;
    uint32_t flippedDictionaryLength = 0;
    int32_t  numWordsInDictionary    = 0;
    int      assumeFlipped           = 0;

    *outFiletype = MEGAHAL_UNKNOWN_FILETYPE;

    brnflip_error returnCode = brnflip_verify_header(
        brain,
        brainLength
    );

    returnCode = returnCode || brnflip_find_dictionary_offset(
        brain,
        brainLength,
        &dictionaryOffset
    );

    if (returnCode == BRNFLIP_NO_ERROR) {
        memcpy(&dictionaryLength, brain + dictionaryOffset, sizeof(uint32_t));
        flippedDictionaryLength = dictionaryLength;
        brnflip_flip_32_in_place((char*) &flippedDictionaryLength);
    }

    returnCode = returnCode || brnflip_count_words_in_dictionary(
        brain,
        brainLength,
        dictionaryOffset,
        &numWordsInDictionary
    );

    if (flippedDictionaryLength != numWordsInDictionary) {
        assumeFlipped = 1;
    } else if (dictionaryLength != numWordsInDictionary) {
        returnCode = MEGAHAL_INVALID_FILE;
    }


    if (returnCode == BRNFLIP_NO_ERROR) {
        returnCode = returnCode || brnflip_traverse_tree(
            brain,
            brainLength,
            &position,
            assumeFlipped
        );

        returnCode = returnCode || brnflip_traverse_tree(
            brain,
            brainLength,
            &position,
            assumeFlipped
        );


        if (returnCode != BRNFLIP_NO_ERROR || position != dictionaryOffset) {
            // If dictionaryLength is the same number when its endianess is
            // flipped, we could have gotten assumeFlipped wrong, so try again.
            if (dictionaryLength == flippedDictionaryLength) {
                position      = MEGAHAL_HEADER_LENGTH;
                assumeFlipped = !assumeFlipped;
                returnCode    = BRNFLIP_NO_ERROR;

                returnCode = returnCode || brnflip_traverse_tree(
                    brain,
                    brainLength,
                    &position,
                    assumeFlipped
                );

                returnCode = returnCode || brnflip_traverse_tree(
                    brain,
                    brainLength,
                    &position,
                    assumeFlipped
                );
            } else {
                returnCode = MEGAHAL_INVALID_FILE;
            }
        }
    }

    if (returnCode == BRNFLIP_NO_ERROR) {
        // We can now be sure that assumeFlipped is a correct assumption.
        if (assumeFlipped) {
            #if BYTE_ORDER == BIG_ENDIAN
            *outFiletype = MEGAHAL_LITTLE_ENDIAN;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            *outFiletype = MEGAHAL_BIG_ENDIAN;
            #endif
        } else {
            #if BYTE_ORDER == BIG_ENDIAN
            *outFiletype = MEGAHAL_BIG_ENDIAN;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            *outFiletype = MEGAHAL_LITTLE_ENDIAN;
            #endif
        }
    }

    return returnCode;
}

/* This function performs the flipping of the MegaHALv8 brain. Since the
 * dictionary must not be flipped, this function requires the start position
 * of the dictonary.
 */
brnflip_error brnflip_flip_buffer(
    char*  brain,
    size_t brainLength
)
{
    off_t position         = MEGAHAL_HEADER_LENGTH;
    off_t dictionaryOffset = 0;

    brnflip_error returnCode = brnflip_verify_header(
        brain,
        brainLength
    );

    returnCode = returnCode || brnflip_find_dictionary_offset(
        brain,
        brainLength,
        &dictionaryOffset
    );

    if (returnCode == BRNFLIP_NO_ERROR) {
        while (position < dictionaryOffset) {
            brnflip_flip_16_in_place(brain + position);
            position += sizeof(uint16_t);

            brnflip_flip_32_in_place(brain + position);
            position += sizeof(uint32_t);

            brnflip_flip_16_in_place(brain + position);
            position += sizeof(uint16_t);

            brnflip_flip_16_in_place(brain + position);
            position += sizeof(uint16_t);
        }

        brnflip_flip_32_in_place(brain + position);
        position += sizeof(uint32_t);
    }

    return returnCode;
}

/* This function verifies the header of a brain file, returning BRNFLIP_NO_ERROR
 * if the header is valid, and BRNFLIP_INVALID_INPUT otherwise.
 */
brnflip_error brnflip_verify_header(char* brain, size_t brainLength)
{
    char cookie[MEGAHAL_COOKIE_LENGTH + 1];

    if (brainLength < MEGAHAL_MIN_BRAIN_LENGTH || brain[9] != 5) {
        return MEGAHAL_INVALID_FILE;
    }

    memset(cookie, 0, MEGAHAL_COOKIE_LENGTH + 1);
    memcpy(cookie, brain, MEGAHAL_COOKIE_LENGTH);

    if (strcmp(cookie, "MegaHALv8") != 0) {
        return MEGAHAL_INVALID_FILE;
    }

    return BRNFLIP_NO_ERROR;
}

/* This function finds the start of the MegaHALv8 dictionary. It takes advantage
 * of the fact that the dictionary starts with "<ERROR>". If the start is found,
 * it returns BRNFLIP_NO_ERROR, but if not, returns BRNFLIP_INVALID_FILE.
 */

brnflip_error brnflip_find_dictionary_offset(
    char*  brain,
    size_t brainLength,
    off_t* dictionaryOffset
)
{
    char c;
    int foundStart = 0;

    *dictionaryOffset = brainLength - MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH - 2;
    c = brain[*dictionaryOffset];

    while( *dictionaryOffset >= 0 && !foundStart){
        /* Search for the start of the dictionary */
        if (c == MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH) {
            char word[MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH + 1];
            memset(word, 0, MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH + 1);
            memcpy(
                word,
                &brain[*dictionaryOffset + 1],
                MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH
            );

            if(strncmp(
                word,
                MEGAHAL_FIRST_DICTIONARY_WORD, MEGAHAL_FIRST_DICTIONARY_WORD_LENGTH
            ) == 0){
                foundStart = 1;
            }
        }
        if(*dictionaryOffset > 0){
            --*dictionaryOffset;
            c = brain[*dictionaryOffset];
        }
    }

    ++*dictionaryOffset;
    *dictionaryOffset -= sizeof(uint32_t);

    if (*dictionaryOffset < 0) {
        return MEGAHAL_INVALID_FILE;
    }

    return BRNFLIP_NO_ERROR;
}

/* This function counts the number of words in the dictionary, returning
 * MEGAHAL_INVALID_FILE if there are no words in the dictionary, and
 * BRNFLIP_NO_ERROR otherwise.
 */
brnflip_error brnflip_count_words_in_dictionary(
    char*    brain,
    size_t   brainLength,
    off_t    dictionaryOffset,
    int32_t* numWordsInDictionary
)
{
    unsigned char  wordLength = 0;
    off_t position = dictionaryOffset + sizeof(uint32_t);

    *numWordsInDictionary = 0;

    while(position < brainLength && *numWordsInDictionary >= 0){
        wordLength = brain[position];
        ++*numWordsInDictionary;
        position += wordLength + 1;
    }

    if (*numWordsInDictionary <= 0 || position > brainLength + 1) {
        return MEGAHAL_INVALID_FILE;
    }

    return BRNFLIP_NO_ERROR;
}

/* This function attempts to traverse a MegaHALv8 tree. If the tree extends past
 * bound, the function returns MEGAHAL_INVALID_FILE, otherwise, it advances
 * posiiton to the end of the tree and returns BRNFLIP_NO_ERROR.
 */
brnflip_error brnflip_traverse_tree(
    char*  brain,
    size_t brainLength,
    off_t* position,
    int    assumeFlipped
)
{
    uint16_t numBranches = 0;

    brnflip_error returnCode = BRNFLIP_NO_ERROR;

    if (*position + MEGAHAL_TREE_NODE_LENGTH < brainLength) {
        memcpy(
            &numBranches,
            brain + *position + MEGAHAL_TREE_NODE_LENGTH - sizeof(uint16_t),
            sizeof(uint16_t)
        );

        if (assumeFlipped) {
            brnflip_flip_16_in_place((char*) &numBranches);
        }

        *position += MEGAHAL_TREE_NODE_LENGTH;
    }

    uint32_t i;
    for (i = 0; i < numBranches && returnCode == BRNFLIP_NO_ERROR; ++i) {
        returnCode = returnCode || brnflip_traverse_tree(
            brain,
            brainLength,
            position,
            assumeFlipped
        );
    }

    return returnCode;
}

void brnflip_flip_16_in_place(char* x)
{
    uint8_t temp = x[0];
    x[0] = x[1];
    x[1] = temp;
}

void brnflip_flip_32_in_place(char* x){
    char temp[4];
    memcpy(&temp, x, 4);
    x[0] = temp[3];
    x[1] = temp[2];
    x[2] = temp[1];
    x[3] = temp[0];
}
