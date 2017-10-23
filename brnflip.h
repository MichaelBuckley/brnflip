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

#ifndef __BRNFLIP_H__
#define __BRNFLIP_H__

typedef enum
{
    no_error     =  0,
    invalid_file = -1,
} brnflip_error;

typedef enum
{
    unknown_filetype = 0,
    big_endian       = 1,
    little_endian    = 2
} megahal_filetype;

extern const megahal_filetype megahal_native_endianess;

/* This function takes as input a buffer with the contents of a MegaHAL brain
 * file and attempts to determine its endianess, placing the result into
 * outFileType. If it is unable to determine the endianess, or if the input
 * buffer is not a valid MegaHAL brain, it will return MEGAHAL_INVALID_INPUT and
 * place MEGAHAL_UNKNOWN_FILETYPE into outFiletype.
 */

brnflip_error brnflip_detect_endianess(
    char*             brain,
    size_t            brain_length,
    megahal_filetype* out_file_type
);

/* This function flips the endianess of a buffer in-place. It must perform one
 * check to complete the conversion, but otherwise does not check to ensure that
 * the buffer is a valid MegaHAL brain.
 */

brnflip_error brnflip_flip_buffer(
    char*  brain,
    size_t brain_length
);

#endif // __BRNFLIP_H__
