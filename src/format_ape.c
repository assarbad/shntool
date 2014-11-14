/*  format_ape.c - ape format module
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
 * $Id: format_ape.c,v 1.21 2004/03/30 07:52:42 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"

#define MAC "mac"

#define MAC_MAGIC "MAC "

static FILE *open_for_input(char *,int *);
static FILE *open_for_output(char *,int *);
static int is_our_file(char *);

format_module format_ape = {
  "ape",
  "Monkey's Audio Compressor",
  "ape",
  0,
  1,
  MAC,
  MAC,
  is_our_file,
  open_for_input,
  open_for_output,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
/* NOTE: mac does not supply the correct chunk size in the WAVE header for
 * some reason.  This is fixed via a kluge - see ape_input_kluge() in
 * src/core_wave.c for details.
 */
{
  FILE *input,*output;

  arg_add(MAC);
  arg_add(filename);
  arg_add("-");
  arg_add("-d");

  *pid = spawn_input(&input,&output);

  if (output)
    fclose(output);

  return input;
}

static FILE *open_for_output(char *filename,int *pid)
{
  FILE *input,*output;

  if (CLOBBER_FILE_YES != clobber_check(filename))
    return NULL;

  arg_add(MAC);
  arg_add("-");
  arg_add(filename);
  arg_add("-c2000");

  *pid = spawn_output(&input,&output);

  if (input)
    fclose(input);

  return output;
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

  if (0 == tagcmp(buf,MAC_MAGIC))
    return 1;

  /* it's not a MAC */
  return 0;
}
