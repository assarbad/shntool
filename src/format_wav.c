/*  format_wav.c - WAVE format module
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
 * $Id: format_wav.c,v 1.30 2004/04/07 17:54:09 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "format.h"
#include "fileio.h"
#include "misc.h"

static FILE *open_for_input(char *,int *);
static FILE *open_for_output(char *,int *);
static int is_our_file(char *);

#define WAVPACK_MAGIC "wvpk"

format_module format_wav = {
  "wav",
  "RIFF WAVE file format",
  "wav",
  0,
  0,
  NULL,
  NULL,
  is_our_file,
  open_for_input,
  open_for_output,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
{
  *pid = NO_CHILD_PID;
  return open_input(filename);
}

static FILE *open_for_output(char *filename,int *pid)
{
  if (CLOBBER_FILE_YES != clobber_check(filename))
    return NULL;

  *pid = NO_CHILD_PID;
  return open_output(filename);
}

static int is_our_file(char *filename)
{
  wave_info *info;
  unsigned char buf[4];
  int i;

  if (0 == (info = new_wave_info(NULL)))
    st_error("could not allocate memory for WAVE info in wav check");

  info->filename = filename;

  if (NULL == (info->input = open_input(filename))) {
    free(info);
    return 0;
  }

  for (i=0;formats[i];i++)
    if (0 == strcmp(formats[i]->name,"wav")) {
      info->input_format = formats[i];
      break;
    }

  if (0 == verify_wav_header(info)) {
    fclose(info->input);
    free(info);
    return 0;
  }

  /* WavPack header might follow RIFF header - make sure this isn't a WavPack file */
  if (4 != fread(buf,1,4,info->input)) {
    fclose(info->input);
    free(info);
    return 1;
  }

  fclose(info->input);
  free(info);

  if (tagcmp(buf,WAVPACK_MAGIC))
    return 1;

  /* it's not a WAVE */
  return 0;
}
