/*  fileio.h - file I/O function definitions
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
 * $Id: fileio.h,v 1.17 2004/04/03 20:03:05 jason Exp $
 */

#ifndef __FILEIO_H__
#define __FILEIO_H__

#include <stdio.h>

#define read_tag(f,t)      read_value_long(f,NULL,NULL,t)
#define read_be_long(f,b)  read_value_long(f,b,NULL,NULL)
#define read_le_long(f,l)  read_value_long(f,NULL,l,NULL)
#define read_be_short(f,b) read_value_short(f,b,NULL)
#define read_le_short(f,l) read_value_short(f,NULL,l)

#define read_n_bytes(a,b,c) read_n_bytes_internal(a,b,c,1)
#define write_n_bytes(a,b,c) write_n_bytes_internal(a,b,c,1)
#define transfer_n_bytes(a,b,c) transfer_n_bytes_internal(a,b,c,1)

#define read_n_bytes_quiet(a,b,c) read_n_bytes_internal(a,b,c,0)
#define write_n_bytes_quiet(a,b,c) write_n_bytes_internal(a,b,c,0)
#define transfer_n_bytes_quiet(a,b,c) transfer_n_bytes_internal(a,b,c,0)

/* writes the specified number of zero bytes to the file descriptor given */
int write_padding(FILE *,int);

/* reads n bytes from a file into a buffer */
int read_n_bytes_internal(FILE *,unsigned char *,int,int);

/* writes n bytes from a buffer into a file */
int write_n_bytes_internal(FILE *,unsigned char *,int,int);

/* transfers n bytes from a file into another file */
unsigned long transfer_n_bytes_internal(FILE *,FILE *,unsigned long,int);

/* reads an unsigned long in big- and/or little-endian format from a file descriptor */
int read_value_long(FILE * file,unsigned long *,unsigned long *,unsigned char *);

/* reads an unsigned short in big- and/or little-endian format from a file descriptor */
int read_value_short(FILE * file,unsigned short *,unsigned short *);

#endif
