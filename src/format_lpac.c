/*  format_pac.c - pac format module
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
 * $Id: format_lpac.c,v 1.17 2004/03/28 21:35:02 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"

#define LPAC "lpac"

#define LPAC_MAGIC "LPAC"

static FILE *open_for_input(char *,int *);
static int is_our_file(char *);

format_module format_lpac = {
  "lpac",
  "LPAC - Lossless Predictive Audio Compression",
  "pac",
  0,
  1,
  NULL,
  LPAC,
  is_our_file,
  open_for_input,
  NULL,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
{
  FILE *input,*output;

  arg_add(LPAC);
  arg_add("-x");
  arg_add(filename);
  arg_add("-");

  *pid = spawn_input(&input,&output);

  if (output)
    fclose(output);

  return input;
}

static int is_our_file(char *filename)
{
  FILE *file;
  unsigned char buf[4];

  if (NULL == (file = open_input(filename)))
    return 0;

  if (4 != fread(buf,1,4,file)) {
    fclose(file);
    return 0;
  }
  fclose(file);

  if (0 == tagcmp(buf,LPAC_MAGIC))
    return 1;

  /* it's not a LPAC */
  return 0;
}
