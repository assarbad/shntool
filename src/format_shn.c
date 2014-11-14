/*  format_shn.c - shorten format module
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
 * $Id: format_shn.c,v 1.30 2004/03/30 07:52:42 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"

#define SHORTEN "shorten"

#define SHORTEN_MAGIC "ajkg"
#define SHORTEN_SEEKTABLE_MAGIC "SHNAMPSK"

static FILE *open_for_input(char *,int *);
static FILE *open_for_output(char *,int *);
static int is_our_file(char *);
static void show_extra_info(char *);

format_module format_shn = {
  "shn",
  "Shorten low complexity waveform coder",
  "shn",
  0,
  1,
  SHORTEN,
  SHORTEN,
  is_our_file,
  open_for_input,
  open_for_output,
  show_extra_info,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
{
  FILE *input,*output;

  arg_add(SHORTEN);
  arg_add("-x");
  arg_add(filename);
  arg_add("-");

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

  arg_add(SHORTEN);
  arg_add("-");
  arg_add(filename);

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

  if (0 == tagcmp(buf,SHORTEN_MAGIC))
    return 1;

  /* it's not a SHN */
  return 0;
}

static void show_extra_info(char *filename)
{
  FILE *f;
  char buf[9];

  printf("  seekable:                   ");

  if (NULL == (f = fopen(filename,"r"))) {
    printf("could not open file\n");
    return;
  }

  if (0 != fseek(f,-8,SEEK_END)) {
    printf("could not seek to end of file\n");
    fclose(f);
    return;
  }

  if (8 != fread(buf,1,8,f)) {
    printf("could not read last 8 bytes of file\n");
    fclose(f);
    return;
  }

  fclose(f);

  buf[8] = 0;

  if (0 == tagcmp(buf,SHORTEN_SEEKTABLE_MAGIC))
    printf("yes\n");
  else
    printf("no\n");
}
