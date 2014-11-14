/*  mode_conv.c - conv mode module
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
 * $Id: mode_conv.c,v 1.45 2004/04/13 07:16:57 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "fileio.h"
#include "wave.h"
#include "misc.h"

static int output_dir_flag = 0;

static char *output_directory = ".";

static format_module *op = NULL;

static void conv_main(int,char **,int);

mode_module mode_conv = {
  "conv",
  "shnconv",
  "converts files from one format to another",
  conv_main
};

static void show_usage()
{
  int i,j=0;

  printf("Usage: %s [OPTIONS] [files]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -o fmt  specifies the output file format, where fmt is one of the following:\n");
  printf("\n");
  for (i=0;formats[i];i++)
    if (formats[i]->output_func)
      printf("             %" MAX_MODULE_NAME_LENGTH "s  (%s) %s\n",formats[i]->name,formats[i]->description,(0 == (j++))?"[DEFAULT]":"");
  printf("\n");
  printf("  -O val  specifies whether output files should be overwritten if they exist.\n");
  printf("          val must be one of:  ask always never  (default is always)\n");
  printf("\n");
  printf("  -d dir  specifies alternate output directory (default is same as input file)\n");
  printf("\n");
  printf("  -D      print debugging information\n");
  printf("\n");
  printf("  --      indicates that everything following it is a file name\n");
  printf("\n");
  printf("  -v      shows version information\n");
  printf("  -h      shows this help screen\n");
  printf("\n");
  printf("  If no file names are given, then file names are read from standard input.\n");
  printf("\n");
  printf("  ------------\n");
  printf("  Sample uses:\n");
  printf("  ------------\n");
  printf("\n");
  printf("  %% %s -o shn filename.wav\n",fullprogname);
  printf("\n");
  printf("  %% find / -name \\*.shn | %s -o flac\n",fullprogname);
  printf("\n");

  exit(0);
}

static void parse(int argc,char **argv,int begin,int *first_arg)
{
  int start,i,found_output = -1;

  start = begin;

  for (i=0;formats[i];i++)
    if (formats[i]->output_func) {
      found_output = i;
      break;
    }

  if (-1 == found_output)
    st_error("no output formats found, cannot continue");

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
    else if (argc > start && 0 == strcmp(argv[start],"-d")) {
      if (1 == output_dir_flag)
        st_help("too many output directories specified");
      start++;
      output_dir_flag = 1;
      if (argc <= start)
        st_help("missing output directory");
      output_directory = argv[start];
    }
    else if (argc > start && 0 == strcmp(argv[start],"-o")) {
      if (NULL != op)
        st_help("too many output file formats specified");
      op = output_format_init(argc,argv,&start);
    }
    else if (argc > start && 0 == strcmp(argv[start],"-O")) {
      start++;
      if (argc <= start)
        st_help("missing overwrite action");
      if (0 == strcmp(argv[start],"ask"))
        clobberflag = CLOBBER_ACTION_ASK;
      else if (0 == strcmp(argv[start],"always"))
        clobberflag = CLOBBER_ACTION_ALWAYS;
      else if (0 == strcmp(argv[start],"never"))
        clobberflag = CLOBBER_ACTION_NEVER;
      else
        st_help("invalid overwrite action: %s",argv[start]);
    }
    else {
      st_help("unknown argument: %s",argv[start]);
    }
    start++;
  }

  if (NULL == op) {
    op = formats[found_output];
    st_warning("no output format specified - assuming %s",op->name);
  }

  *first_arg = start;
}

static void make_outfilename(wave_info *info,char *outfilename)
{
  char *ext,*basename,infilename[FILENAME_SIZE];

  strcpy(infilename,info->filename);

  if ((basename = strrchr(infilename,'/')))
    basename++;
  else
    basename = infilename;

  if ((ext = strrchr(basename,'.')) && (0 == strcmp(ext+1,info->input_format->extension)))
    *ext = '\0';

  if (1 == output_dir_flag)
    my_snprintf(outfilename,FILENAME_SIZE,"%s/%s.%s",output_directory,basename,op->extension);
  else
    my_snprintf(outfilename,FILENAME_SIZE,"%s.%s",infilename,op->extension);
}

static void conv_file(wave_info *info)
{
  int output_pid,bytes,success = 0;
  FILE *output;
  char outfilename[FILENAME_SIZE];
  unsigned char *header,nullpad[BUF_SIZE];

  make_outfilename(info,outfilename);

  if (files_are_identical(info->filename,outfilename)) {
    st_warning("output file '%s' would overwrite input file '%s' - skipping.",outfilename,info->filename);
    return;
  }

  if (0 != open_input_stream(info)) {
    st_warning("could not open file '%s' for input - skipping.",info->filename);
    return;
  }

  if (NULL == (output = op->output_func(outfilename,&output_pid))) {
    st_warning("could not open file '%s' for output - skipping.",outfilename);
    goto cleanup1;
  }

  if (NULL == (header = malloc(info->header_size * sizeof(unsigned char)))) {
    st_warning("could not allocate %d bytes for WAVE header from file '%s' - skipping.",info->header_size,info->filename);
    goto cleanup2;
  }

  printf("converting '%s' to '%s' ... ",info->filename,outfilename);
  fflush(stdout);

  if (read_n_bytes(info->input,header,info->header_size) != info->header_size) {
    st_warning("error while discarding %d-byte header - skipping.",info->header_size);
    goto cleanup3;
  }

  if (0 == do_header_kluges(header,info)) {
    st_warning("could not fix WAVE header - skipping.");
    goto cleanup3;
  }

  if ((info->header_size > 0) && write_n_bytes(output,header,info->header_size) != info->header_size) {
    st_warning("error while writing %d-byte header - skipping.",info->header_size);
    goto cleanup3;
  }

  if ((info->data_size > 0) && (transfer_n_bytes(info->input,output,info->data_size) != info->data_size)) {
    st_warning("error while transferring %lu-byte data chunk - skipping.",info->data_size);
    goto cleanup3;
  }

  if (PROB_ODD_SIZED_DATA(info)) {
    nullpad[0] = 1;

    bytes = read_n_bytes_quiet(info->input,nullpad,1);

    if ((1 != bytes) && (0 != bytes)) {
      st_warning("error while reading possible NULL pad byte from file '%s'",info->filename);
      goto cleanup3;
    }

    if ((0 == bytes) || ((1 == bytes) && (0 != nullpad[0]))) {
      st_debug("file '%s' does not contain NULL pad byte for odd-sized data chunk per RIFF specs",info->filename);
    }

    if (1 == bytes) {
      if (0 == nullpad[0]) {
        if (write_n_bytes(output,nullpad,1) != 1) {
          st_warning("error while writing NULL pad byte");
          goto cleanup3;
        }
      }
      else {
        if (write_n_bytes(output,nullpad,1) != 1) {
          st_warning("error while writing initial extra byte");
          goto cleanup3;
        }
      }
    }
  }

  if (PROB_EXTRA_CHUNKS(info) && (transfer_n_bytes(info->input,output,info->extra_riff_size) != info->extra_riff_size)) {
    st_warning("error while transferring %lu extra bytes - skipping.",info->extra_riff_size);
    goto cleanup3;
  }

  success = 1;

  printf("done.\n");

cleanup3:
  free(header);

cleanup2:
  if ((CLOSE_CHILD_ERROR_OUTPUT == close_output(output,output_pid)) || (0 == success)) {
    remove_file(op,outfilename);
  }

cleanup1:
  close_input_stream(info);
}

static void process_file(char *filename)
{
  wave_info *info;

  if (NULL == (info = new_wave_info(filename)))
    return;

  conv_file(info);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  char filename[FILENAME_SIZE];
  int i;

  if (argc < start + 1) {
    /* no filename was given, so we're reading one file name from standard input. */
    fgets(filename,FILENAME_SIZE-1,stdin);
    while (0 == feof(stdin)) {
      filename[strlen(filename)-1] = '\0';
      process_file(filename);
      fgets(filename,FILENAME_SIZE-1,stdin);
    }
  }
  else {
    for (i=start;i<argc;i++)
      process_file(argv[i]);
  }
}

static void conv_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
