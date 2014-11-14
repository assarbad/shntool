/*  convert.h - conversion function definitions
 *  Copyright (C) 2000-2004  Jason Jordan <shnutils@freeshell.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * $Id: convert.h,v 1.9 2004/04/12 16:48:29 jason Exp $
 */

#ifndef __CONVERT_H__
#define __CONVERT_H__

/* converts 4 bytes stored in little-endian format to an unsigned long */
unsigned long uchar_to_ulong_le(unsigned char *);

/* converts 2 bytes stored in little-endian format to an unsigned short */
unsigned short uchar_to_ushort_le(unsigned char *);

/* converts an unsigned long to 4 bytes stored in little-endian format */
void ulong_to_uchar_le(unsigned char *,unsigned long);

/* converts an unsigned short to 2 bytes stored in little-endian format */
void ushort_to_uchar_le(unsigned char *,unsigned short);

/* converts 4 bytes stored in big-endian format to an unsigned long */
unsigned long uchar_to_ulong_be(unsigned char *);

/* converts 2 bytes stored in big-endian format to an unsigned short */
unsigned short uchar_to_ushort_be(unsigned char *);

/* converts an unsigned long to 4 bytes stored in big-endian format */
void ulong_to_uchar_be(unsigned char *,unsigned long);

/* converts an unsigned short to 2 bytes stored in big-endian format */
void ushort_to_uchar_be(unsigned char *,unsigned short);

/* converts 4 bytes stored in synchsafe integer format to an unsigned long */
unsigned long synchsafe_int_to_ulong(unsigned char *);

#endif
