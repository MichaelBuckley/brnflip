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

#ifndef __BRNFLIP_H__
#define __BRNFLIP_H__

#define MEGAHAL_UNKNOWN_INPUT -1
#define MEGAHAL_UNSUPPORTED_ENDIANESS -2

typedef enum
{
    BRNFLIP_NO_ERROR     =  0,
    MEGAHAL_INVALID_FILE = -1,
} brnflip_error;

typedef enum
{
    MEGAHAL_UNKNOWN_FILETYPE = 0,
    MEGAHAL_BIG_ENDIAN       = 1,
    MEGAHAL_LITTLE_ENDIAN    = 2
} megahal_filetype;

#if BYTE_ORDER == BIG_ENDIAN
#define NATIVE_MEGAHAL_ENDIANESS MEGAHAL_BIG_ENDIAN
#elif BYTE_ORDER == LITTLE_ENDIAN
#define NATIVE_MEGAHAL_ENDIANESS MEGAHAL_LITTLE_ENDIAN
#else
#define NATIVE_MEGAHAL_ENDIANESS MEGAHAL_UNKNOWN_FILETYPE
#endif

/* This function takes as input a buffer with the contents of a MegaHAL brain
 * file and attempts to determine its endianess, placing the result into
 * outFileType. If it is unable to determine the endianess, or if the input
 * buffer is not a valid MegaHAL brain, it will return MEGAHAL_INVALID_INPUT and
 * place MEGAHAL_UNKNOWN_FILETYPE into outFiletype.
 */

brnflip_error brnflip_detect_endianess(
    char*             brain,
    size_t            brainLength,
    megahal_filetype* outFiletype
);

/* This function flips the endianess of a buffer in-place. It must perform one
 * check to complete the conversion, but otherwise does not check to ensure that
 * the buffer is a valid MegaHAL brain.
 */

brnflip_error brnflip_flip_buffer(
    char*  brain,
    size_t brainLength
);

#endif
