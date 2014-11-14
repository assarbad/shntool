/*  format_ofr.c - ofr format module
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
 * $Id: format_ofr.c,v 1.23 2004/04/08 06:35:45 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"
#include "shntool.h"

#define OPTIMFROG "ofr"

#define OPTIMFROG_MAGIC     "OFR "
#define OPTIMFROG_MAGIC_OLD "*RIFF"

static FILE *open_for_input(char *,int *);
static FILE *open_for_output(char *,int *);
static int is_our_file(char *);

format_module format_ofr = {
  "ofr",
  "OptimFROG Lossless WAVE Audio Coder",
  "ofr",
  0,
  1,
  OPTIMFROG,
  OPTIMFROG,
  is_our_file,
  open_for_input,
  open_for_output,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
{
  FILE *input,*output,*f;
  int file_has_id3v2_tag;

  arg_add(OPTIMFROG);
  arg_add("--decode");

  /* check for ID3v2 tag on input */
  f = open_input_internal(filename,&file_has_id3v2_tag,NULL);

  if ((f) && (0 != file_has_id3v2_tag)) {
    st_debug("OFR input file '%s' has an ID3v2 tag, which '%s' can't handle - attempting to\n"
             "work around this by skipping the tag and feeding the OFR data on its standard input",filename,OPTIMFROG);
    arg_add("-");
  }
  else {
    fclose(f);
    file_has_id3v2_tag = 0;
    arg_add(filename);
  }

  arg_add("--output");
  arg_add("-");

  if (0 != file_has_id3v2_tag) {
    *pid = spawn_input_fd(&input,&output,f);
    fclose(f);
  }
  else
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

  arg_add(OPTIMFROG);
  arg_add("--encode");
  arg_add("-");
  arg_add("--output");
  arg_add(filename);

  *pid = spawn_output(&input,&output);

  if (input)
    fclose(input);

  return output;
}

static int is_our_file(char *filename)
{
  FILE *file;
  unsigned char buf[5];

  if (NULL == (file = open_input(filename)))
    return 0;

  if (5 != fread(buf,1,5,file)) {
    fclose(file);
    return 0;
  }
  fclose(file);

  if (0 == tagcmp(buf,OPTIMFROG_MAGIC) || 0 == tagcmp(buf,OPTIMFROG_MAGIC_OLD))
    return 1;

  /* it's not an OPTIMFROG */
  return 0;
}
