/*  format.h - format module definitions
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
 * $Id: format.h,v 1.14 2004/03/29 06:38:42 jason Exp $
 */

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <stdio.h>

typedef struct {
  char   *name;                               /* format name, specified on command line in certain modes */
  char   *description;                        /* one line description of this format */
  char   *extension;                          /* extension to append to files created by this module */
  int     is_translated;                      /* is this file format a simple translation (e.g. byte-swap) of WAV? */
  int     is_compressed;                      /* is this file format a compression format? */
  char   *encoder;                            /* name of external program to handle encoding of this format - set to NULL if not applicable */
  char   *decoder;                            /* name of external program to handle decoding of this format - set to NULL if not applicable */
  int   (*is_our_file)(char *);               /* routine to determine whether the given file belongs to this format plugin */
  FILE *(*input_func)(char *,int *);          /* routine to open a file of this format for input - set to NULL if not applicable */
  FILE *(*output_func)(char *,int *);         /* routine to open a file of this format for output - set to NULL if not applicable */
  void  (*extra_info)(char *);                /* routine to display extra information in info mode - set to NULL if not applicable */
  void  (*get_arguments)(int,char **,int *);  /* routine to process any extra command-line arguments for output modes - set to NULL if not applicable */
} format_module;

#endif
