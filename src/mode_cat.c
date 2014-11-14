/*  mode_cat.c - cat mode module
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
 * $Id: mode_cat.c,v 1.33 2004/04/13 07:16:57 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "fileio.h"
#include "wave.h"
#include "misc.h"

static int cat_header = 1;
static int cat_data = 1;
static int cat_padded_data = 1;
static int cat_extra = 1;

static void cat_main(int,char **,int);

mode_module mode_cat = {
  "cat",
  "shncat",
  "writes PCM WAVE data from one or more files to stdout",
  cat_main
};

static void show_usage()
{
  printf("Usage: %s [OPTIONS] [files]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -nh  suppress WAVE headers\n");
  printf("\n");
  printf("  -nd  suppress WAVE data\n");
  printf("\n");
  printf("  -nr  suppress extra RIFF chunks\n");
  printf("\n");
  printf("  -np  suppress NULL pad byte at end of odd-sized data chunks, if present\n");
  printf("\n");
  printf("  -D   print debugging information\n");
  printf("\n");
  printf("  --   indicates that everything following it is a file name\n");
  printf("\n");
  printf("  -v   shows version information\n");
  printf("  -h   shows this help screen\n");
  printf("\n");
  printf("  If no file names are given, then file names are read from standard input.\n");
  printf("\n");
  printf("  ------------\n");
  printf("  Sample uses:\n");
  printf("  ------------\n");
  printf("\n");
  printf("  %% %s -nh filename.wav\n",fullprogname);
  printf("\n");
  printf("  %% echo filename.shn | %s\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-nh"))
      cat_header = 0;
    else if (argc > start && 0 == strcmp(argv[start],"-nd"))
      cat_data = 0;
    else if (argc > start && 0 == strcmp(argv[start],"-nr"))
      cat_extra = 0;
    else if (argc > start && 0 == strcmp(argv[start],"-np"))
      cat_padded_data = 0;
    else
      st_help("unknown argument: %s",argv[start]);
    start++;
  }

  if (0 == cat_header && 0 == cat_data && 0 == cat_extra)
    st_help("nothing to do if -nh, -nd and -nr are specified");

  *first_arg = start;
}

static void cat_file(wave_info *info)
{
  unsigned char *header,nullpad[BUF_SIZE];
  FILE *devnull,*data_dest;
  int bytes;

  if (NULL == (devnull = open_output("/dev/null")))
    st_error("could not open /dev/null for output");

  if (0 == cat_header && 0 == cat_data && 0 == PROB_EXTRA_CHUNKS(info)) {
    st_warning("file '%s' contains no extra RIFF chunks - nothing to do",info->filename);
    return;
  }

  if (0 != open_input_stream(info))
    st_error("could not reopen file '%s' for input",info->filename);

  if (NULL == (header = malloc(info->header_size * sizeof(unsigned char))))
    st_error("could not allocate %d bytes for WAVE header from file '%s'",info->header_size,info->filename);

  if (read_n_bytes(info->input,header,info->header_size) != info->header_size)
    st_error("error while reading in %d-byte header from file '%s'",info->header_size,info->filename);

  if (1 == cat_header) {
    if (0 == do_header_kluges(header,info))
      st_error("could not fix WAVE header from file '%s'",info->filename);

    if (write_n_bytes(stdout,header,info->header_size) != info->header_size)
      st_error("error while writing %d bytes of header from file '%s'",info->header_size,info->filename);
  }

  if (1 == cat_data)
    data_dest = stdout;
  else
    data_dest = devnull;

  if (transfer_n_bytes(info->input,data_dest,info->data_size) != info->data_size)
    st_error("error while transferring %lu bytes of data from file '%s'",info->data_size,info->filename);

  if (PROB_ODD_SIZED_DATA(info)) {
    nullpad[0] = 1;

    bytes = read_n_bytes_quiet(info->input,nullpad,1);

    if ((1 != bytes) && (0 != bytes))
      st_error("error while reading possible NULL pad byte from file '%s'",info->filename);

    if ((0 == bytes) || ((1 == bytes) && (0 != nullpad[0]))) {
      st_debug("file '%s' does not contain NULL pad byte for odd-sized data chunk per RIFF specs",info->filename);
    }

    if (1 == bytes) {
      if ((0 == nullpad[0]) && (1 == cat_padded_data)) {
        if (write_n_bytes(data_dest,nullpad,1) != 1)
          st_error("error while writing NULL pad byte from file '%s'",info->filename);
      }
      if ((0 != nullpad[0]) && (1 == cat_extra)) {
        if (write_n_bytes(stdout,nullpad,1) != 1)
          st_error("error while writing initial extra byte from file '%s'",info->filename);
      }
    }
  }

  if ((1 == cat_extra) && PROB_EXTRA_CHUNKS(info)) {
    if (info->extra_riff_size != transfer_n_bytes(info->input,stdout,info->extra_riff_size))
      st_error("error while transferring %lu extra bytes from file '%s'",info->extra_riff_size,info->filename);
  }

  fclose(devnull);

  free(header);

  close_input_stream(info);
}

static void process_file(char *filename)
{
  wave_info *info;

  if (NULL == (info = new_wave_info(filename)))
    return;

  cat_file(info);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  char filename[FILENAME_SIZE];
  int i;

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

static void cat_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
