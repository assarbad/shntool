/*  mode_cue.c - cue sheet mode module
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
 * $Id: mode_cue.c,v 1.6 2004/04/02 00:51:56 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "fileio.h"
#include "wave.h"
#include "misc.h"

enum {
  TYPE_UNKNOWN,
  TYPE_CUESHEET,
  TYPE_SPLITPOINTS
};

static int output_type = TYPE_UNKNOWN;

static double total_data_size = 0.0;
static int numfiles = 0;

static wave_info *totals = NULL;

static void cue_main(int,char **,int);

mode_module mode_cue = {
  "cue",
  "shncue",
  "generates a CUE sheet or split points from a set of files",
  cue_main
};

static void show_usage()
{
  printf("Usage: %s [OPTIONS] [files]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -c  generate CUE sheet (default)\n");
  printf("\n");
  printf("  -s  generate split points in explcit byte-offset format\n");
  printf("\n");
  printf("  -D  print debugging information\n");
  printf("\n");
  printf("  --  indicates that everything following it is a file name\n");
  printf("\n");
  printf("  -v  shows version information\n");
  printf("  -h  shows this help screen\n");
  printf("\n");
  printf("  If no file names are given, then file names are read from standard input.\n");
  printf("\n");
  printf("  ------------\n");
  printf("  Sample uses:\n");
  printf("  ------------\n");
  printf("\n");
  printf("  %% %s *.shn\n",fullprogname);
  printf("\n");

  exit(0);
}

static void parse(int argc,char **argv,int begin,int *first_arg)
{
  int start;

  start = begin;

  while (start < argc && argv[start][0] == '-') {
    if (argc > start && 0 == strcmp(argv[start],"--")) {
      start++;
      break;
    }
    else if (argc > start && 0 == strcmp(argv[start],"-c"))
      output_type = TYPE_CUESHEET;
    else if (argc > start && 0 == strcmp(argv[start],"-s"))
      output_type = TYPE_SPLITPOINTS;
    else if (argc > start && 0 == strcmp(argv[start],"-v"))
      internal_version();
    else if (argc > start && 0 == strcmp(argv[start],"-h"))
      show_usage();
    else if (argc > start && 0 == strcmp(argv[start],"-D"))
      shntool_debug = 1;
    else
      st_help("unknown argument: %s",argv[start]);
    start++;
  }

  if (TYPE_UNKNOWN == output_type) {
    output_type = TYPE_CUESHEET;
    st_warning("no output type specified - assuming CUE sheet");
  }

  *first_arg = start;
}

static void verify_wave_info(wave_info *info)
{
  if (output_type == TYPE_CUESHEET && PROB_NOT_CD(info)) {
    st_error("file '%s' is not CD-quality",info->filename);
  }

  if (0 == totals->wave_format && 0 == totals->channels &&
      0 == totals->samples_per_sec && 0 == totals->avg_bytes_per_sec &&
      0 == totals->bits_per_sample && 0 == totals->block_align) {
    totals->wave_format = info->wave_format;
    totals->channels = info->channels;
    totals->samples_per_sec = info->samples_per_sec;
    totals->avg_bytes_per_sec = info->avg_bytes_per_sec;
    totals->bits_per_sample = info->bits_per_sample;
    totals->block_align = info->block_align;
    totals->problems = info->problems;
    return;
  }

  if (info->wave_format != totals->wave_format)
    st_error("WAVE format differs among these files");

  if (info->channels != totals->channels)
    st_error("number of channels differs among these files");

  if (info->samples_per_sec != totals->samples_per_sec)
    st_error("samples per second differs among these files");

  if (info->avg_bytes_per_sec != totals->avg_bytes_per_sec)
    st_error("average bytes per second differs among these files");

  if (info->bits_per_sample != totals->bits_per_sample)
    st_error("bits per sample differs among these files");

  if (info->block_align != totals->block_align) {
    st_warning("block align differs among these files");
  }
}

static void output_init()
{
  if (NULL == (totals = new_wave_info(NULL)))
    st_error("could not allocate memory for totals");

  if (output_type == TYPE_CUESHEET) {
    printf("FILE \"joined.wav\" WAVE\n");
  }
}

static void output_track(char *filename)
{
  wave_info *info;
  wlong curr_data_size = 0;
  char *p;

  if (NULL == (info = new_wave_info(filename)))
    return;

  verify_wave_info(info);

  numfiles++;

  curr_data_size = info->data_size;

  if (output_type == TYPE_CUESHEET) {
    info->data_size = (wlong)total_data_size;
    info->length = info->data_size / info->rate;
    length_to_str(info);
    if ((p = strstr(info->m_ss,".")))
      *p = ':';
    printf("  TRACK %02d AUDIO\n",numfiles);
    printf("    INDEX 01 %s\n",info->m_ss);
  }
  else if (output_type == TYPE_SPLITPOINTS) {
    if (total_data_size > 0.0)
      printf("%0.0f\n",total_data_size);
  }

  total_data_size += (double)curr_data_size;
}

static void output_end()
{
  if (TYPE_CUESHEET == output_type && numfiles < 1) {
    st_error("need one or more file names in order to generate CUE sheet");
  }

  if (TYPE_SPLITPOINTS == output_type && numfiles < 2) {
    st_error("need two or more file names in order to generate split points");
  }
}

static void process(int argc,char **argv,int start)
{
  int i;
  char filename[FILENAME_SIZE];

  output_init();

  if (argc < start + 1) {
    /* no filenames were given, so we're reading files from standard input. */
    fgets(filename,FILENAME_SIZE-1,stdin);
    while (0 == feof(stdin)) {
      filename[strlen(filename)-1] = '\0';
      output_track(filename);
      fgets(filename,FILENAME_SIZE-1,stdin);
    }
  }
  else {
    for (i=start;i<argc;i++) {
      output_track(argv[i]);
    }
  }

  output_end();
}

static void cue_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
