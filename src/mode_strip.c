/*  mode_strip.c - strip mode module
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
 * $Id: mode_strip.c,v 1.42 2004/04/05 15:59:40 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "convert.h"
#include "fileio.h"
#include "wave.h"
#include "misc.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define POSTFIX_STRIP "-stripped"

static int output_dir_flag = 0;
static int strip_header = 1;
static int strip_chunks = 1;
static int preview = 0;

static char *output_directory = ".";

static format_module *op = NULL;

static void strip_main(int,char **,int);

mode_module mode_strip = {
  "strip",
  "shnstrip",
  "strips extra RIFF chunks and/or writes canonical headers",
  strip_main
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
  printf("  -nh     don't rewrite WAVE header in canonical format (default is yes)\n");
  printf("\n");
  printf("  -nr     don't strip unnecessary RIFF chunks (default is yes)\n");
  printf("\n");
  printf("  -p      preview what changes would be made without actually making them\n");
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
  printf("  %% %s -o shn -d /mnt/shn/stripped *.shn *.wav\n",fullprogname);
  printf("\n");
  printf("  %% find /your/audio/dir -type f | %s\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-nh"))
      strip_header = 0;
    else if (argc > start && 0 == strcmp(argv[start],"-nc")) {
      st_warning("-nc will soon be deprecated; use -nr instead");
      strip_chunks = 0;
    }
    else if (argc > start && 0 == strcmp(argv[start],"-nr"))
      strip_chunks = 0;
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

  if (0 == strip_header && 0 == strip_chunks)
    st_help("nothing to strip if both -nh and -nc are specified");

  if (NULL == op) {
    op = formats[found_output];
    st_warning("no output format specified - assuming %s",op->name);
  }

  *first_arg = start;
}

static void make_outfilename(char *infilename,char *outfilename)
{
  char *basename;

  if ((basename = strrchr(infilename,'/')))
    basename++;
  else
    basename = infilename;

  if (1 == output_dir_flag)
    my_snprintf(outfilename,FILENAME_SIZE,"%s/%s",output_directory,basename);
  else
    my_snprintf(outfilename,FILENAME_SIZE,"%s",infilename);

  if (filename_contains_a_dot(outfilename))
    *(strrchr(outfilename,'.')) = '\0';

  strcat(outfilename,POSTFIX_STRIP);
  strcat(outfilename,".");
  strcat(outfilename,op->extension);
}

static void strip_and_canonicize(wave_info *info)
{
  wint new_header_size;
  wlong new_chunk_size;
  unsigned char *header;
  char outfilename[FILENAME_SIZE];
  FILE *output;
  int output_pid,has_null_pad,success = 0;
  long possible_extra_stuff;

  if (PROB_HDR_INCONSISTENT(info)) {
    st_warning("file '%s' has an inconsistent header - skipping.",info->filename);
    return;
  }

  if (PROB_TRUNCATED(info)) {
    st_warning("file '%s' seems to be truncated - skipping.",info->filename);
    return;
  }

  if ((1 == strip_header && 1 == strip_chunks) && ((0 == PROB_EXTRA_CHUNKS(info)) && (0 == PROB_HDR_NOT_CANONICAL(info)))) {
    st_warning("file '%s' already has a canonical header and no extra RIFF chunks - skipping.",info->filename);
    return;
  }

  if ((1 == strip_chunks && 0 == strip_header) && (0 == PROB_EXTRA_CHUNKS(info))) {
    st_warning("file '%s' already has no extra RIFF chunks - skipping.",info->filename);
    return;
  }

  if ((1 == strip_header && 0 == strip_chunks) && (0 == PROB_HDR_NOT_CANONICAL(info))) {
    st_warning("file '%s' already has a canonical header - skipping.",info->filename);
    return;
  }

  if (files_are_identical(info->filename,outfilename)) {
    st_warning("output file '%s' would overwrite input file '%s' - skipping.",outfilename,info->filename);
    return;
  }

  has_null_pad = odd_sized_data_chunk_is_null_padded(info);

  if (0 == has_null_pad)
    info->extra_riff_size++;

  new_header_size = info->header_size;
  possible_extra_stuff = (info->extra_riff_size > 0) ? info->extra_riff_size : 0;

  if (1 == strip_header)
    new_header_size = CANONICAL_HEADER_SIZE;

  if (1 == strip_chunks)
    possible_extra_stuff = 0;

  new_chunk_size = info->chunk_size - (info->header_size - new_header_size) - (info->extra_riff_size - possible_extra_stuff);

  if (1 == preview) {
    printf("%s --> %s\n",info->filename,outfilename);
    if (1 == strip_header && PROB_HDR_NOT_CANONICAL(info))
      printf("  - will rewrite %d-byte WAVE header to the canonical %d-byte header\n",info->header_size,CANONICAL_HEADER_SIZE);
    if (1 == strip_chunks && PROB_EXTRA_CHUNKS(info))
      printf("  - will strip %ld byte%s worth of extra RIFF chunk(s) from the end of this file\n",possible_extra_stuff,(possible_extra_stuff > 1)?"s":"");
    printf("\n");
    return;
  }

  if (0 != open_input_stream(info)) {
    st_warning("could not reopen file '%s' for input - skipping.",info->filename);
    return;
  }

  make_outfilename(info->filename,outfilename);

  if (NULL == (output = op->output_func(outfilename,&output_pid))) {
    st_warning("could not open file '%s' for output - skipping.",outfilename);
    goto cleanup1;
  }

  if (NULL == (header = malloc(info->header_size * sizeof(unsigned char)))) {
    st_warning("could not allocate %d bytes for WAVE header from file '%s' - skipping.",info->header_size,info->filename);
    goto cleanup2;
  }

  if (read_n_bytes(info->input,header,info->header_size) != info->header_size) {
    st_warning("error while reading %d bytes of data from file '%s' - skipping.",info->header_size,info->filename);
    goto cleanup3;
  }

  /* already read in header, now check to see if we need to overwrite that with a canonical one */
  if (1 == strip_header)
    make_canonical_header(header,info);

  put_chunk_size(header,new_chunk_size);

  printf("%s --> %s ... ",info->filename,outfilename);
  fflush(stdout);

  if (write_n_bytes(output,header,new_header_size) != new_header_size) {
    st_warning("error while writing %d bytes of data - skipping.",new_header_size);
    goto cleanup3;
  }

  if (transfer_n_bytes(info->input,output,info->data_size) != info->data_size) {
    st_warning("error while transferring %lu bytes of data - skipping.",info->data_size);
    goto cleanup3;
  }

  if (PROB_ODD_SIZED_DATA(info) && (1 == has_null_pad) && (1 != transfer_n_bytes(info->input,output,1))) {
    st_warning("error while transferring NULL pad byte - skipping.");
    goto cleanup3;
  }

  if ((possible_extra_stuff > 0) && (transfer_n_bytes(info->input,output,possible_extra_stuff) != possible_extra_stuff)) {
    st_warning("error while transferring %lu extra bytes - skipping.",possible_extra_stuff);
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

  strip_and_canonicize(info);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  int i;
  char filename[FILENAME_SIZE];

  if (1 == preview) {
    printf("\n");
    printf("Preview of changes:\n");
    printf("-------------------\n");
    printf("\n");
  }
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

static void strip_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
