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

// Constants

#if BYTE_ORDER == BIG_ENDIAN
const megahal_filetype megahal_native_endianess = big_endian;
#elif BYTE_ORDER == LITTLE_ENDIAN
const megahal_filetype megahal_native_endianess = little_endian;
#else
const megahal_filetype megahal_native_endianess = unknown_filetype;
#endif

const char*  cookie                 = "MegaHALv8";
const size_t cookie_length          = 9;
const char   model_order            = 5;
const off_t  header_length          = cookie_length + sizeof(model_order);

const uint32_t num_trees            = 2;
const off_t    tree_node_length     = sizeof(uint16_t) * 3 + sizeof(uint32_t);

const char*  firstDictWord          = "<ERROR>";
const size_t first_dict_word_length = 7;

const size_t min_dict_length = sizeof(uint32_t) +
    first_dict_word_length + 1;

const size_t min_brain_length = header_length +
    tree_node_length * num_trees +
    min_dict_length;

void brnflip_flip_16_in_place(char* x);
void brnflip_flip_32_in_place(char* x);

// Function declarations

brnflip_error brnflip_verify_header(char* brain, size_t brain_length);

brnflip_error brnflip_find_dictionary_offset(
    char*  brain,
    size_t brain_length,
    off_t* dictionary_offset
);

brnflip_error brnflip_count_words_in_dictionary(
    char*    brain,
    size_t   brain_length,
    off_t    dictionary_offset,
    int32_t* num_words_in_dictionary
);

brnflip_error brnflip_traverse_tree(
    char*  brain,
    size_t brain_length,
    off_t* position,
    int    assume_flipped
);

// Function implementations

brnflip_error brnflip_detect_endianess(
    char*             brain,
    size_t            brain_length,
    megahal_filetype* out_file_type
)
{
    off_t    position                  = header_length;
    off_t    dictionary_offset         = 0;
    uint32_t dictionary_length         = 0;
    uint32_t flipped_dictionary_length = 0;
    int32_t  num_words_in_dictionary   = 0;
    int      assume_flipped            = 0;

    *out_file_type = unknown_filetype;

    brnflip_error return_code = brnflip_verify_header(
        brain,
        brain_length
    );

    return_code = return_code || brnflip_find_dictionary_offset(
        brain,
        brain_length,
        &dictionary_offset
    );

    /* Now that we know where the dictionary starts, we can read in the
     * dictionary length saved in the file. We can then progress to the end of
     * the file, counting the actual number of words in the dictionary. If the
     * two counts differ, we know that the file is not in our native endianess,
     * but if the counts are the same, we can't make any assumptions, since
     * there are some numbers with the same representation in both endianesses.
     */

    if (return_code == no_error) {
        memcpy(
            &dictionary_length,
            brain + dictionary_offset,
            sizeof(uint32_t)
        );

        flipped_dictionary_length = dictionary_length;
        brnflip_flip_32_in_place((char*) &flipped_dictionary_length);
    }

    return_code = return_code || brnflip_count_words_in_dictionary(
        brain,
        brain_length,
        dictionary_offset,
        &num_words_in_dictionary
    );

    if (flipped_dictionary_length == num_words_in_dictionary) {
        assume_flipped = 1;
    } else if (dictionary_length != num_words_in_dictionary) {
        return_code = invalid_file;
    }

    /* Next, we traverse the trees. After the trees comes the dictionary, so
     * if the file is in our assumed endianess, we should end up at our
     * previously calculated dictionary position.
     */

    if (return_code == no_error) {
        return_code = return_code || brnflip_traverse_tree(
            brain,
            brain_length,
            &position,
            assume_flipped
        );

        return_code = return_code || brnflip_traverse_tree(
            brain,
            brain_length,
            &position,
            assume_flipped
        );


        if (return_code != no_error || position != dictionary_offset) {
            /* If dictionary_length is the same number when its endianess is
             * flipped, we could have gotten assume_flipped wrong, so try
             * traversing the trees again with the opposite assumed endianess.
             */
            if (dictionary_length == flipped_dictionary_length) {
                position      = header_length;
                assume_flipped = !assume_flipped;
                return_code    = no_error;

                return_code = return_code || brnflip_traverse_tree(
                    brain,
                    brain_length,
                    &position,
                    assume_flipped
                );

                return_code = return_code || brnflip_traverse_tree(
                    brain,
                    brain_length,
                    &position,
                    assume_flipped
                );
            } else {
                return_code = invalid_file;
            }
        }
    }

    if (return_code == no_error) {
        // We can now be sure that assume_flipped is a correct assumption.
        if (assume_flipped) {
            #if BYTE_ORDER == BIG_ENDIAN
            *out_file_type = little_endian;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            *out_file_type = big_endian;
            #endif
        } else {
            #if BYTE_ORDER == BIG_ENDIAN
            *out_file_type = big_endian;
            #elif BYTE_ORDER == LITTLE_ENDIAN
            *out_file_type = little_endian;
            #endif
        }
    }

    return return_code;
}

/* This function performs the flipping of the MegaHALv8 brain. Since the
 * dictionary must not be flipped, this function finds the start position of
 * the dictonary.
 */
brnflip_error brnflip_flip_buffer(
    char*  brain,
    size_t brain_length
)
{
    off_t position         = header_length;
    off_t dictionary_offset = 0;

    brnflip_error return_code = brnflip_verify_header(
        brain,
        brain_length
    );

    return_code = return_code || brnflip_find_dictionary_offset(
        brain,
        brain_length,
        &dictionary_offset
    );

    if (return_code == no_error) {
        while (position < dictionary_offset) {
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

    return return_code;
}

/* This function verifies the header of a brain file, returning no_error
 * if the header is valid, and BRNFLIP_INVALID_INPUT otherwise.
 */
brnflip_error brnflip_verify_header(char* brain, size_t brain_length)
{
    char cookie[cookie_length + 1] = { 0 };

    if (brain_length < min_brain_length ||
        brain[cookie_length] != model_order) {
        return invalid_file;
    }

    memcpy(cookie, brain, cookie_length);

    if (strcmp(cookie, cookie) != 0) {
        return invalid_file;
    }

    return no_error;
}

/* This function finds the start of the MegaHALv8 dictionary. It takes
 * advantage of the fact that the dictionary starts with "<ERROR>". If the
 * start is found, it returns no_error, but if not, returns
 * BRNFLIP_INVALID_FILE.
 */

brnflip_error brnflip_find_dictionary_offset(
    char*  brain,
    size_t brain_length,
    off_t* dictionary_offset
)
{
    char c;
    int foundStart = 0;

    *dictionary_offset = brain_length - first_dict_word_length - 1;
    c = brain[*dictionary_offset];

    while (*dictionary_offset >= 0 && !foundStart) {
        // Search for the start of the dictionary
        if (c == first_dict_word_length) {
            char word[first_dict_word_length + 1] = { 0 };

            memcpy(
                word,
                &brain[*dictionary_offset + 1],
                first_dict_word_length
            );

            if(strncmp(word, firstDictWord, first_dict_word_length) == 0){
                foundStart = 1;
            }
        }
        if(*dictionary_offset > 0){
            --*dictionary_offset;
            c = brain[*dictionary_offset];
        }
    }

    ++*dictionary_offset;
    *dictionary_offset -= sizeof(uint32_t);

    if (*dictionary_offset < 0) {
        return invalid_file;
    }

    return no_error;
}

/* This function counts the number of words in the dictionary, returning
 * invalid_file if there are no words in the dictionary, and
 * no_error otherwise.
 */
brnflip_error brnflip_count_words_in_dictionary(
    char*    brain,
    size_t   brain_length,
    off_t    dictionary_offset,
    int32_t* num_words_in_dictionary
)
{
    unsigned char  wordLength = 0;
    off_t position = dictionary_offset + sizeof(uint32_t);

    *num_words_in_dictionary = 0;

    while(position < brain_length && *num_words_in_dictionary >= 0){
        wordLength = brain[position];
        ++*num_words_in_dictionary;
        position += wordLength + 1;
    }

    if (*num_words_in_dictionary <= 0 || position > brain_length + 1) {
        return invalid_file;
    }

    return no_error;
}

/* This function attempts to traverse a MegaHALv8 tree. If the tree extends
 * past the brain length, the function returns invalid_file, otherwise,
 * it advances position to the end of the tree and returns no_error.
 */
brnflip_error brnflip_traverse_tree(
    char*  brain,
    size_t brain_length,
    off_t* position,
    int    assume_flipped
)
{
    uint16_t num_branches = 0;

    brnflip_error return_code = no_error;

    if (*position + tree_node_length < brain_length) {
        memcpy(
            &num_branches,
            brain + *position + tree_node_length - sizeof(uint16_t),
            sizeof(uint16_t)
        );

        if (assume_flipped) {
            brnflip_flip_16_in_place((char*) &num_branches);
        }

        *position += tree_node_length;
    }

    uint32_t i;
    for (i = 0; i < num_branches && return_code == no_error; ++i) {
        return_code = return_code || brnflip_traverse_tree(
            brain,
            brain_length,
            position,
            assume_flipped
        );
    }

    return return_code;
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
