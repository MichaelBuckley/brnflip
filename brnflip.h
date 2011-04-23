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

#ifndef __BRNCONVERT_H__
#define __BRNCONVERT_H__

#define MEGAHAL_UNKNOWN_INPUT -1
#define MEGAHAL_UNSUPPORTED_ENDIANESS -2

typedef enum
{
    MEGAHAL_UNKNOWN_FILETYPE,
    MEGAHAL_BIG_ENDIAN,
    MEGAHAL_LITTLE_ENDIAN
} megahal_filetype;

/* This function takes as input a buffer with the contents of a MegaHALv8 brain
 * file and automatically determines its endianess. It then converts the buffer
 * in place to the target endianness if necessary.
 *
 * Because MegaHALv8 brains do not specify their endianess, this function makes
 * a best guess as to the endianess of the buffer. Although it cannot be 100%
 * accurate, it should be accurate enough to cover all real-world cases. If for
 * some reason this function returns 0 without making a conversion when the
 * file's endianess and the target endianess are different, it has guessed
 * incorrectly. The function forceMegahalConversion can be used to successfully
 * convert these files.
 *
 * This function will only run on little-endian machines or big-endian machines.
 * I am currently unaware of any port of MegaHAL to middle-endian machines, or
 * to any machines with more exotic endianesses. The function
 * forceMegahalConversion can be used to force a conversion from big-endian to
 * little endian or vice-versa on these machines, but there is no function to
 * convert MegaHALv8 brains to or from any other endianess. If this function is
 * called on an unsupported machine, no conversion will be performed at the 
 *
 * However, if you are interested in a converter for an endianess not currently
 * supported, I would be happy to help. You can contact me at
 * http://angrymen.org/contact/
 *
 * The function returns 0 on success, MEGAHAL_UNKNOWN_INPUT if the buffer is not
 * a MegaHALv8 brain, and MEGAHAL_UNSUPPORTED_ENDIANESS if the machine is
 * neither big-endian or little-endian.
 */
int convertMegahalBrain(uint8_t* brain, long brainLen, megahal_filetype target);

/* This function flips the endianess of a buffer. It must perform one check to
 * perform the conversion, but otherwise does not check to ensure that the
 * buffer is a valid MegaHALv8 brain. This function can be run on any machine,
 * but will only convert from big-endian to little-endian and vice-versa.
 */
int forceMegahalConversion(uint8_t* brain, long brainLen);

#endif
