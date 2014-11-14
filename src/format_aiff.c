/*  format_aiff.c - aiff format module
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
 * $Id: format_aiff.c,v 1.42 2004/04/08 06:35:45 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"
#include "shntool.h"

#define SOX_AIFF "sox"

static FILE *open_for_input(char *,int *);
static FILE *open_for_output(char *,int *);
static int is_our_file(char *);

format_module format_aiff = {
  "aiff",
  "Audio Interchange File Format (AIFF and uncompressed/sowt AIFF-C only)",
  "aiff",
  1,
  0,
  SOX_AIFF,
  SOX_AIFF,
  is_our_file,
  open_for_input,
  open_for_output,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
/* NOTE: sox cannot fseek() on stdout, so it cannot write the correct
 * chunk size or data size in the WAVE header.  This is fixed via a
 * kluge - see aiff_input_kluge() in src/core_wave.c for details.
 */
{
  FILE *input,*output,*f;
  int file_has_id3v2_tag;

  arg_add(SOX_AIFF);
  arg_add("-t");
  arg_add("aiff");

  /* check for ID3v2 tag on input */
  f = open_input_internal(filename,&file_has_id3v2_tag,NULL);

  if ((f) && (0 != file_has_id3v2_tag)) {
    st_debug("AIFF input file '%s' has an ID3v2 tag, which '%s' can't handle - attempting to\n"
             "work around this by skipping the tag and feeding the AIFF data on its standard input",filename,SOX_AIFF);
    arg_add("-");
  }
  else {
    fclose(f);
    file_has_id3v2_tag = 0;
    arg_add(filename);
  }

  arg_add("-t");
  arg_add("wav");
  arg_add("-");
  arg_add("copy");

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

  arg_add(SOX_AIFF);
  arg_add("-t");
  arg_add("wav");
  arg_add("-");
  arg_add("-t");
  arg_add("aiff");
  arg_add(filename);
  arg_add("copy");

  *pid = spawn_output(&input,&output);

  if (input)
    fclose(input);

  return output;
}

static int is_our_file(char *filename)
{
  FILE *file;
  unsigned long be_long=0;
  unsigned short be_short=0;
  unsigned char tag[4];

  if (NULL == (file = open_input(filename)))
    return 0;

  /* look for FORM header */
  if (0 == read_tag(file,tag) || tagcmp(tag,AIFF_FORM)) {
    fclose(file);
    return 0;
  }

  /* skip FORM chunk size, read in FORM type */
  if (0 == read_be_long(file,&be_long) || 0 == read_tag(file,tag)) {
    fclose(file);
    return 0;
  }

  /* if FORM type is AIFF, it's ok */
  if (0 == tagcmp(tag,AIFF_FORM_TYPE_AIFF)) {
    fclose(file);
    return 1;
  }

  /* if FORM type is not AIFC, it's not ok */
  if (tagcmp(tag,AIFF_FORM_TYPE_AIFC)) {
    fclose(file);
    return 0;
  }

  /* now let's check AIFC compression type - it's in the COMM chunk */
  while (1) {
    /* read chunk id */
    if (0 == read_tag(file,tag)) {
      fclose(file);
      return 0;
    }

    if (0 == tagcmp(tag,AIFF_COMM))
      break;

    /* not COMM, so read size of this chunk and skip it */
    if (0 == read_be_long(file,&be_long) || 0 != fseek(file,(long)be_long,SEEK_CUR)) {
      fclose(file);
      return 0;
    }
  }

  /* now read compression type */
  if (0 == read_be_long(file,&be_long) || 0 == read_be_long(file,&be_long) ||
      0 == read_be_long(file,&be_long) || 0 == read_be_long(file,&be_long) ||
      0 == read_be_long(file,&be_long) || 0 == read_be_short(file,&be_short) ||
      0 == read_tag(file,tag))
  {
    fclose(file);
    return 0;
  }

  fclose(file);

  /* check compression type against types known to be supported by sox */
  if (0 == tagcmp(tag,AIFF_COMPRESSION_NONE) || 0 == tagcmp(tag,AIFF_COMPRESSION_SOWT))
    return 1;

  st_debug("found unsupported AIFF-C compression type [%c%c%c%c]",tag[0],tag[1],tag[2],tag[3]);

  /* it's not readable by recent versions of sox */
  return 0;
}
