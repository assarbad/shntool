/*  mode_info.c - info mode module
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
 * $Id: mode_info.c,v 1.35 2004/05/05 07:23:40 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "wave.h"
#include "misc.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static void info_main(int,char **,int);

mode_module mode_info = {
  "info",
  "shninfo",
  "displays detailed information about PCM WAVE data",
  info_main
};

static void show_usage()
{
  int i;

  printf("Usage: %s [OPTIONS] [files]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
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
  printf("  %% %s",fullprogname);
  for (i=0;formats[i];i++)
    if (formats[i]->input_func)
      printf(" *.%s",formats[i]->extension);
  printf("\n");
  printf("\n");
  printf("  %% find /your/audio/dir -type f | %s\n",fullprogname);
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

  *first_arg = start;
}

static void show_info(wave_info *info)
{
  wlong appended_bytes,missing_bytes;

  printf("-------------------------------------------------------------------------------\n");
  printf("file name:                    %s\n",info->filename);
  printf("handled by:                   %s format module\n",info->input_format->name);
  printf("length:                       %s\n",info->m_ss);
  printf("WAVE format:                  0x%04x (%s)\n",info->wave_format,format_to_str(info->wave_format));
  printf("channels:                     %hu\n",info->channels);
  printf("bits/sample:                  %hu\n",info->bits_per_sample);
  printf("samples/sec:                  %lu\n",info->samples_per_sec);
  printf("average bytes/sec:            %lu\n",info->avg_bytes_per_sec);
  printf("rate (calculated):            %lu\n",info->rate);
  printf("block align:                  %hu\n",info->block_align);
  printf("header size:                  %d bytes\n",info->header_size);
  printf("data size:                    %lu byte%s\n",info->data_size,(1 == info->data_size)?"":"s");
  printf("chunk size:                   %lu bytes\n",info->chunk_size);
  printf("total size (chunk size + 8):  %lu bytes\n",info->total_size);
  printf("actual file size:             %lu\n",info->actual_size);
  printf("file is compressed:           %s\n",(0 == info->input_format->is_compressed)?"no":"yes");
  printf("compression ratio:            %0.4f\n",(double)info->actual_size/(double)info->total_size);
  printf("CD-quality properties:\n");
  printf("  CD quality:                 %s\n",(PROB_NOT_CD(info))?"no":"yes");
  printf("  cut on sector boundary:     %s\n",(PROB_NOT_CD(info))?"n/a":((PROB_BAD_BOUND(info))?"no":"yes"));

  printf("  sector misalignment:        ");
  if (PROB_NOT_CD(info))
    printf("n/a\n");
  else
    printf("%lu byte%s\n",info->data_size % CD_BLOCK_SIZE,(1 == (info->data_size % CD_BLOCK_SIZE))?"":"s");

  printf("  long enough to be burned:   ");
  if (PROB_NOT_CD(info))
    printf("n/a\n");
  else {
    if (PROB_TOO_SHORT(info))
      printf("no - needs to be at least %d bytes\n",CD_MIN_BURNABLE_SIZE);
    else
      printf("yes\n");
  }

  printf("WAVE properties:\n");
  printf("  non-canonical header:       %s\n",(PROB_HDR_NOT_CANONICAL(info))?"yes":"no");

  printf("  extra RIFF chunks:          ");
  if (PROB_EXTRA_CHUNKS(info)) {
    if (PROB_ODD_SIZED_DATA(info))
      printf("yes (%ld or %ld bytes)\n",info->extra_riff_size,info->extra_riff_size + 1);
    else
      printf("yes (%ld bytes)\n",info->extra_riff_size);
  }
  else
    printf("no\n");

  printf("Possible problems:\n");

  printf("  file contains ID3v2 tag:    ");
  if (info->file_has_id3v2_tag)
    printf("yes (%lu bytes)\n",info->id3v2_tag_size);
  else
    printf("no\n");

  printf("  data chunk block-aligned:   ");
  if (PROB_DATA_NOT_ALIGNED(info))
    printf("no\n");
  else
    printf("yes\n");

  printf("  inconsistent header:        %s\n",(PROB_HDR_INCONSISTENT(info))?"yes":"no");

  printf("  file probably truncated:    ");
  if ((0 == info->input_format->is_compressed) && (0 == info->input_format->is_translated)) {
    if (PROB_TRUNCATED(info)) {
      missing_bytes = info->total_size - (info->actual_size - info->id3v2_tag_size);
      printf("yes (missing %lu byte%s)\n",missing_bytes,(1 == missing_bytes)?"":"s");
    }
    else
      printf("no\n");
  }
  else
    printf("unknown\n");

  printf("  junk appended to file:      ");
  if ((0 == info->input_format->is_compressed) && (0 == info->input_format->is_translated)) {
    appended_bytes = info->actual_size - info->total_size - info->id3v2_tag_size;
    if (PROB_JUNK_APPENDED(info) && appended_bytes > 0)
      printf("yes (%lu byte%s)\n",appended_bytes,(1 == appended_bytes)?"":"s");
    else
      printf("no\n");
  }
  else
    printf("unknown\n");

  printf("  odd data size has pad byte: ");
  if (0 == PROB_ODD_SIZED_DATA(info))
    printf("n/a\n");
  else {
    if (-1 == info->extra_riff_size)
      printf("no\n");
    else if (0 == info->extra_riff_size)
      printf("yes\n");
    else
      printf("unknown\n");
  }

  if (info->input_format->extra_info) {
    printf("Extra %s-specific info:\n",info->input_format->name);
    info->input_format->extra_info(info->filename);
  }
}

static void process_file(char *filename)
{
  wave_info *info;

  if (NULL == (info = new_wave_info(filename)))
    return;

  show_info(info);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  int i;
  char filename[FILENAME_SIZE];

  if (argc < start + 1) {
    /* no filenames were given, so we're reading files from standard input. */
    fgets(filename,FILENAME_SIZE-1,stdin);
    while (0 == feof(stdin)) {
      filename[strlen(filename)-1] = '\0';
      process_file(filename);
      fgets(filename,FILENAME_SIZE-1,stdin);
    }
  }
  else
    for (i=start;i<argc;i++)
      process_file(argv[i]);
}

static void info_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
