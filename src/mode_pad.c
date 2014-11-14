/*  mode_pad.c - pad mode module
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
 * $Id: mode_pad.c,v 1.21 2004/04/04 10:33:28 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "fileio.h"
#include "wave.h"
#include "misc.h"

enum {
  PAD_UNKNOWN,
  PAD_PREPAD,
  PAD_POSTPAD
};

#define POSTFIX_PREPADDED  "prepadded"
#define POSTFIX_POSTPADDED "postpadded"

static int output_dir_flag = 0;
static int pad_type = PAD_UNKNOWN;
static int preview = 0;

static char *output_directory = ".";

static format_module *op = NULL;

static void pad_main(int,char **,int);

mode_module mode_pad = {
  "pad",
  "shnpad",
  "pads CD-quality files not aligned on sector boundaries with silence",
  pad_main
};

static void show_usage()
{
  int i,j=0;

  printf("Usage: %s [OPTIONS] [files]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -o fmt   specifies the output file format, where fmt is one of the following:\n");
  printf("\n");
  for (i=0;formats[i];i++)
    if (formats[i]->output_func)
      printf("              %" MAX_MODULE_NAME_LENGTH "s  (%s) %s\n",formats[i]->name,formats[i]->description,(0 == (j++))?"[DEFAULT]":"");
  printf("\n");
  printf("  -O val   specifies whether output files should be overwritten if they exist.\n");
  printf("           val must be one of:  ask always never  (default is always)\n");
  printf("\n");
  printf("  -d dir   specifies alternate output directory (default is same as input file)\n");
  printf("\n");
  printf("  -prepad  specifies padding should be added to the beginning of the file\n");
  printf("\n");
  printf("  -postpad specifies padding should be added to the end of the file (default)\n");
  printf("\n");
  printf("  -p       preview what changes would be made without actually making them\n");
  printf("\n");
  printf("  -D       print debugging information\n");
  printf("\n");
  printf("  --       indicates that everything following it is a file name\n");
  printf("\n");
  printf("  -v       shows version information\n");
  printf("  -h       shows this help screen\n");
  printf("\n");
  printf("  If no file names are given, then file names are read from standard input.\n");
  printf("\n");
  printf("  ------------\n");
  printf("  Sample uses:\n");
  printf("  ------------\n");
  printf("\n");
  printf("  %% %s -prepad -o shn filename.wav\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-prepad"))
      pad_type = PAD_PREPAD;
    else if (argc > start && 0 == strcmp(argv[start],"-postpad"))
      pad_type = PAD_POSTPAD;
    else if (argc > start && 0 == strcmp(argv[start],"-p"))
      preview = 1;
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

  if (PAD_UNKNOWN == pad_type) {
    pad_type = PAD_POSTPAD;
    st_warning("no padding type specified - assuming postpad");
  }

  *first_arg = start;
}

static void make_outfilename(char *infilename,char *outfilename)
/* creates an output file name for padded data */
{
  char *basename;

  if ((basename = strrchr(infilename,'/')))
    basename++;
  else
    basename = infilename;

  if (1 == output_dir_flag)
    my_snprintf(outfilename,FILENAME_SIZE,"%s/%s",output_directory,basename);
  else
    my_snprintf(outfilename,FILENAME_SIZE,"%s",basename);

  if (filename_contains_a_dot(outfilename))
    *(strrchr(outfilename,'.')) = '\0';

  strcat(outfilename,"-");
  strcat(outfilename,(pad_type == PAD_PREPAD)?POSTFIX_PREPADDED:POSTFIX_POSTPADDED);
  strcat(outfilename,".");
  strcat(outfilename,op->extension);
}

static void pad_file(wave_info *info)
{
  int output_pid,pad_bytes,has_null_pad,success = 0;
  FILE *output;
  char outfilename[FILENAME_SIZE];
  unsigned char *header,nullpad[BUF_SIZE];

  make_outfilename(info->filename,outfilename);

  pad_bytes = CD_BLOCK_SIZE - (info->data_size % CD_BLOCK_SIZE);

  if (preview) {
    printf("File '%s' would be %s-padded as '%s' with %d zero-bytes.\n",info->filename,((pad_type == PAD_PREPAD)?"pre":"post"),outfilename,pad_bytes);
    return;
  }

  if (files_are_identical(info->filename,outfilename)) {
    st_warning("output file '%s' would overwrite input file '%s' - skipping.",outfilename,info->filename);
    return;
  }

  has_null_pad = odd_sized_data_chunk_is_null_padded(info);

  if (0 != open_input_stream(info)) {
    st_warning("could not open file '%s' for input - skipping.",info->filename);
    return;
  }

  if (NULL == (output = op->output_func(outfilename,&output_pid))) {
    st_warning("could not open file '%s' for output - skipping.",outfilename);
    goto cleanup1;
  }

  printf("%s-padding '%s' as '%s' with %d zero-bytes ... ",((pad_type == PAD_PREPAD)?"pre":"post"),info->filename,outfilename,pad_bytes);
  fflush(stdout);

  if (NULL == (header = malloc(info->header_size * sizeof(unsigned char)))) {
    st_warning("could not allocate %d bytes for WAVE header - skipping.",info->header_size);
    goto cleanup2;
  }

  if (read_n_bytes(info->input,header,info->header_size) != info->header_size) {
    st_warning("error while discarding %d-byte header - skipping.",info->header_size);
    goto cleanup3;
  }

  if (0 == do_header_kluges(header,info)) {
    st_warning("could not fix WAVE header - skipping.");
    goto cleanup3;
  }

  put_data_size(header,info->header_size,info->data_size+pad_bytes);

  if (PROB_EXTRA_CHUNKS(info)) {
    if (0 == has_null_pad)
      info->extra_riff_size++;
    put_chunk_size(header,info->header_size+info->data_size+pad_bytes+info->extra_riff_size-8);
  }
  else
    put_chunk_size(header,info->header_size+info->data_size+pad_bytes-8);

  if ((info->header_size > 0) && write_n_bytes(output,header,info->header_size) != info->header_size) {
    st_warning("error while writing %d-byte header - skipping.",info->header_size);
    goto cleanup3;
  }

  if (PAD_PREPAD == pad_type) {
    if (pad_bytes != write_padding(output,pad_bytes)) {
      st_warning("error while pre-padding with %d zero-bytes",pad_bytes);
      goto cleanup3;
    }
  }

  if ((info->data_size > 0) && (transfer_n_bytes(info->input,output,info->data_size) != info->data_size)) {
    st_warning("error while transferring %lu-byte data chunk - skipping.",info->data_size);
    goto cleanup3;
  }

  if (PAD_POSTPAD == pad_type) {
    if (pad_bytes != write_padding(output,pad_bytes)) {
      st_warning("error while post-padding with %d zero-bytes",pad_bytes);
      goto cleanup3;
    }
  }

  if (PROB_ODD_SIZED_DATA(info) && (1 == has_null_pad)) {
    if (1 != read_n_bytes(info->input,nullpad,1)) {
      st_warning("error while discarding NULL pad byte");
      goto cleanup3;
    }
  }

  if ((info->extra_riff_size > 0) && (transfer_n_bytes(info->input,output,info->extra_riff_size) != info->extra_riff_size)) {
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

  if (PROB_NOT_CD(info)) {
    st_warning("file '%s' is not CD-quality - skipping.",filename);
    free(info);
    return;
  }

  if (0 == PROB_BAD_BOUND(info)) {
    st_warning("file '%s' is already sector-aligned - skipping.",filename);
    free(info);
    return;
  }

  pad_file(info);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  char filename[FILENAME_SIZE];
  int i;

  if (preview) {
    printf("\n");
    printf("Preview of changes:\n");
    printf("-------------------\n");
    printf("\n");
  }

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

static void pad_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
