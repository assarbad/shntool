/*  format_wv.c - wavpack format module
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
 * $Id: format_wv.c,v 1.16 2004/05/05 05:39:12 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "shntool.h"
#include "format.h"
#include "fileio.h"
#include "misc.h"
#include "wave.h"

#define WAVPACK  "wavpack"
#define WVUNPACK "wvunpack"

#define WAVPACK_MAGIC "wvpk"

#define NOT_WV_FILE 0  /* not a valid WavPack file (or raw mode) */
#define LOSSLESS_WV 1  /* standard lossless WavPack file */
#define HYBRID_WV   2  /* hybrid WavPack file with correction file */
#define LOSSY_WV    3  /* hybrid WavPack file without correction file */

#define RAW_FLAG           4  /* raw mode (no .wav header) */
#define NEW_HIGH_FLAG  0x400  /* new high quality mode (lossless only) */

typedef struct {
  char ckID[4];
  long ckSize;
  short version;
  short bits;
  short flags, shift;
  long total_samples, crc, crc2;
  char extension[4], extra_bc, extras[3];
} WavpackHeader;

static FILE *open_for_input(char *,int *);
static FILE *open_for_output(char *,int *);
static int is_our_file(char *);

format_module format_wv = {
  "wv",
  "WavPack Hybrid Lossless Audio Compression",
  "wv",
  0,
  1,
  WAVPACK,
  WVUNPACK,
  is_our_file,
  open_for_input,
  open_for_output,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,int *pid)
{
  FILE *input,*output;

  arg_add(WVUNPACK);
  arg_add("-y");
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

  arg_add(WAVPACK);
  arg_add("-y");
  arg_add("-");
  arg_add(filename);

  *pid = spawn_output(&input,&output);

  if (input)
    fclose(input);

  return output;
}

static char *filespec_ext(char *filespec)
{
  char *cp = filespec + strlen(filespec);

  while (--cp >= filespec) {
    if (*cp == '\\' || *cp == ':')
      return NULL;

    if (*cp == '.') {
      if (strlen (cp) > 1 && strlen (cp) <= 4)
        return cp;
      else
        return NULL;
    }
  }

  return NULL;
}

static int is_our_file(char *filename)
{
  wave_info *info;
  WavpackHeader wph;
  char *extp;
  FILE *f;
  int i;

  if (0 == (info = new_wave_info(NULL)))
    st_error("could not allocate memory for WAVE info in wv check");

  info->filename = filename;

  if (NULL == (info->input = open_input(filename)))
    return NOT_WV_FILE;

  for (i=0;formats[i];i++)
    if (0 == strcmp(formats[i]->name,"wav")) {
      info->input_format = formats[i];
      break;
    }

  /* find possible WavPack header location - either after RIFF
   * header, or at beginning of the file ("raw" WavPack file)
   */

  if (0 == verify_wav_header(info)) {
    fclose(info->input);
    if (NULL == (info->input = open_input(filename)))
      return NOT_WV_FILE;
  }

  if (sizeof(WavpackHeader) != fread(&wph,1,sizeof(WavpackHeader),info->input)) {
    fclose(info->input);
    return NOT_WV_FILE;
  }

  fclose(info->input);

  if (tagcmp(wph.ckID,WAVPACK_MAGIC) || wph.version < 1 || wph.version > 3 || (wph.flags & RAW_FLAG)) {
    return NOT_WV_FILE;
  }

  if (wph.version == 3 && wph.bits && (wph.flags & NEW_HIGH_FLAG) && wph.crc != wph.crc2) {
    char wvc_filename[FILENAME_SIZE];

    strcpy(wvc_filename,filename);

    extp = filespec_ext(wvc_filename);

    if (extp)
      *extp = '\0';

    strcat(wvc_filename,".wvc");

    if ((f = fopen(wvc_filename,"rb"))) {
      fclose(f);
      return HYBRID_WV;
    }

    /* for non-windows systems we must also try opposite case */

    strcpy(extp,".WVC");

    if ((f = fopen(wvc_filename,"rb"))) {
      fclose(f);
      return HYBRID_WV;
    }

    st_warning("input file '%s' is a lossy WavPack file",filename);
    return LOSSY_WV;
  }

  if (wph.bits) {
    st_warning("input file '%s' is a lossy WavPack file",filename);
    return LOSSY_WV;
  }

  return LOSSLESS_WV;
}
