/*  mode_join.c - join mode module
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
 * $Id: mode_join.c,v 1.43 2004/05/04 23:39:06 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "fileio.h"
#include "wave.h"
#include "misc.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define PREFIX_JOIN   "joined"

enum {
  JOIN_UNKNOWN,
  JOIN_NOPAD,
  JOIN_PREPAD,
  JOIN_POSTPAD
};

static int order = 0;
static int print_to_stdout = 0;
static int pad_bytes = 0;
static int all_files_cd_quality = 1;
static int preview = 0;
static int output_dir_flag = 0;
static int numfiles;
static int pad_type = JOIN_UNKNOWN;

static char *output_directory = ".";

static wave_info **files;

static format_module *op = NULL;

static void join_main(int,char **,int);

mode_module mode_join = {
  "join",
  "shnjoin",
  "joins PCM WAVE data from multiple files into one",
  join_main
};

static void show_usage()
{
  int i,j=0;

  printf("Usage: %s [OPTIONS] file1 file2 [file3 ...]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -o fmt   specifies the output file format, where fmt is one of the following:\n");
  printf("\n");
  for (i=0;formats[i];i++)
    if (formats[i]->output_func)
      printf("             %" MAX_MODULE_NAME_LENGTH "s  (%s) %s\n",formats[i]->name,formats[i]->description,(0 == (j++))?"[DEFAULT]":"");
  printf("\n");
  printf("  -O val   specifies whether output files should be overwritten if they exist.\n");
  printf("           val must be one of:  ask always never  (default is always)\n");
  printf("\n");
  printf("  -d dir   specifies alternate output directory (default is current directory)\n");
  printf("\n");
  printf("  -nopad   specifies that zero-padding should not be done\n");
  printf("\n");
  printf("  -prepad  specifies that zero-padding should be added to the beginning of the file\n");
  printf("\n");
  printf("  -postpad specifies that zero-padding should be added to the end of the file (default)\n");
  printf("\n");
  printf("  -stdout  send joined data to stdout (can only be used with wav output format)\n");
  printf("\n");
  printf("  -order   allows you to edit the order of the files before processing them\n");
  printf("           (useful for certain filenames, where *.shn might give d1t1.shn,\n");
  printf("            d1t10.shn, d1t11.shn, d1t2.shn, d1t3.shn, ...)\n");
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
  printf("  ------------\n");
  printf("  Sample uses:\n");
  printf("  ------------\n");
  printf("\n");
  printf("    %% %s -o wav -order *.shn\n",fullprogname);
  printf("\n");
  printf("    %% %s -o shn -nopad -p *.wav\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-order"))
      order = 1;
    else if (argc > start && 0 == strcmp(argv[start],"-nopad"))
      pad_type = JOIN_NOPAD;
    else if (argc > start && 0 == strcmp(argv[start],"-prepad"))
      pad_type = JOIN_PREPAD;
    else if (argc > start && 0 == strcmp(argv[start],"-postpad"))
      pad_type = JOIN_POSTPAD;
    else if (argc > start && 0 == strcmp(argv[start],"-stdout"))
      print_to_stdout = 1;
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
    else
      st_help("unknown argument: %s",argv[start]);
    start++;
  }

  if (start >= argc - 1)
    st_help("need two or more files to process");

  if (NULL == op) {
    op = formats[found_output];
    st_warning("no output format specified - assuming %s",op->name);
  }

  if (print_to_stdout && strcmp(op->name,"wav"))
    st_help("-stdout can only be used with 'wav' output format");

  if (JOIN_UNKNOWN == pad_type) {
    pad_type = JOIN_POSTPAD;
    st_warning("no padding type specified - assuming postpad");
  }

  *first_arg = start;
}

static void do_join()
{
  int i,bytes_to_skip,bytes_to_xfer,output_pid,success = 0;
  char outfilename[FILENAME_SIZE];
  wlong total=0;
  unsigned char header[CANONICAL_HEADER_SIZE];
  FILE *output;
  wave_info *joined_info;

  if (1 == print_to_stdout) {
    strcpy(outfilename,"stdout");
  }
  else {
    if (1 == output_dir_flag)
      my_snprintf(outfilename,FILENAME_SIZE,"%s/%s.%s",output_directory,PREFIX_JOIN,op->extension);
    else
      my_snprintf(outfilename,FILENAME_SIZE,"%s.%s",PREFIX_JOIN,op->extension);
  }

  for (i=0;i<numfiles;i++)
    total += files[i]->data_size;

  if (1 == all_files_cd_quality && (total % CD_BLOCK_SIZE) != 0) {
    pad_bytes = CD_BLOCK_SIZE - (total % CD_BLOCK_SIZE);
    if (JOIN_NOPAD != pad_type)
      total += pad_bytes;
  }

  if (1 == preview) {
    printf("\n");
    printf("Preview of changes:\n");
    printf("-------------------\n");
    printf("\n");
    printf("WAVE data from the following files:\n\n");
    for (i=0;i<numfiles;i++)
      printf("  %s\n",files[i]->filename);
    printf("\n");
    printf("will be joined into file '%s', in the order shown above.\n",outfilename);
    printf("\n");
    if (1 == all_files_cd_quality) {
      if (JOIN_NOPAD != pad_type) {
        if (0 != pad_bytes)
          printf("This file would then be %s-padded with %d zero-bytes.\n",((JOIN_PREPAD == pad_type)?"pre":"post"),pad_bytes);
        else
          printf("This file wouldn't need any padding.\n");
      }
      else {
        if (0 != pad_bytes)
          printf("This file wouldn't be padded, but would need %d bytes of padding.\n",pad_bytes);
        else
          printf("This file wouldn't be padded, and wouldn't need it.\n");
      }
    }
    else {
      printf("Padding would not apply to these files because they are not CD-quality.\n");
    }
    return;
  }

  if (NULL == (joined_info = new_wave_info(NULL)))
    st_error("could not allocate memory for joined file information");

  if (1 == print_to_stdout)
    output = stdout;
  else {
    if (NULL == (output = op->output_func(outfilename,&output_pid)))
      st_error("could not open file '%s' for output",outfilename);
  }

  joined_info->chunk_size = total + CANONICAL_HEADER_SIZE - 8;
  joined_info->channels = files[0]->channels;
  joined_info->samples_per_sec = files[0]->samples_per_sec;
  joined_info->avg_bytes_per_sec = files[0]->avg_bytes_per_sec;
  joined_info->block_align = files[0]->block_align;
  joined_info->bits_per_sample = files[0]->bits_per_sample;
  joined_info->data_size = total;
  joined_info->wave_format = files[0]->wave_format;

  if (PROB_ODD_SIZED_DATA(joined_info))
    joined_info->chunk_size++;

  make_canonical_header(header,joined_info);

  if (write_n_bytes(output,header,CANONICAL_HEADER_SIZE) != CANONICAL_HEADER_SIZE) {
    st_warning("error while writing %d-byte header",CANONICAL_HEADER_SIZE);
    goto cleanup;
  }

  if (1 == all_files_cd_quality && JOIN_PREPAD == pad_type && 0 != pad_bytes) {
    if (pad_bytes != write_padding(output,pad_bytes)) {
      st_warning("error while pre-padding with %d zero-bytes",pad_bytes);
      goto cleanup;
    }
  }

  for (i=0;i<numfiles;i++) {
    fprintf(stderr,"Adding contents of %s to %s ... ",files[i]->filename,outfilename);
    if (0 != open_input_stream(files[i])) {
      st_warning("could not reopen file '%s' for input",files[i]->filename);
      goto cleanup;
    }
    bytes_to_skip = files[i]->header_size;
    while (bytes_to_skip > 0) {
      bytes_to_xfer = min(bytes_to_skip,CANONICAL_HEADER_SIZE);
      if (read_n_bytes(files[i]->input,header,bytes_to_xfer) != bytes_to_xfer) {
        st_warning("error while reading %d bytes of data from file '%s'",bytes_to_xfer,files[i]->filename);
        goto cleanup;
      }
      bytes_to_skip -= bytes_to_xfer;
    }
    if (transfer_n_bytes(files[i]->input,output,files[i]->data_size) != files[i]->data_size) {
      st_warning("error while transferring %lu bytes of data",files[i]->data_size);
      goto cleanup;
    }
    fprintf(stderr,"done.\n");
    close_input_stream(files[i]);
  }

  if (1 == all_files_cd_quality && JOIN_POSTPAD == pad_type && 0 != pad_bytes) {
    if (pad_bytes != write_padding(output,pad_bytes)) {
      st_warning("error while post-padding with %d zero-bytes",pad_bytes);
      goto cleanup;
    }
  }

  if ((JOIN_NOPAD == pad_type) && PROB_ODD_SIZED_DATA(joined_info) && (1 != write_padding(output,1))) {
    st_warning("error while NULL-padding odd-sized data chunk");
    goto cleanup;
  }

  success = 1;

  if (1 == all_files_cd_quality) {
    if (JOIN_NOPAD != pad_type) {
      if (0 != pad_bytes)
        fprintf(stderr,"%s-padded '%s' with %d zero-bytes.\n",((JOIN_PREPAD == pad_type)?"Pre":"Post"),outfilename,pad_bytes);
      else
        fprintf(stderr,"No padding needed for '%s'.\n",outfilename);
    }
    else {
      fprintf(stderr,"File '%s' was not padded, ",outfilename);
      if (0 != pad_bytes)
        fprintf(stderr,"though it needs %d bytes of padding.\n",pad_bytes);
      else
        fprintf(stderr,"nor was it needed.\n");
    }
  }

cleanup:
  if (0 == print_to_stdout) {
    if ((CLOSE_CHILD_ERROR_OUTPUT == close_output(output,output_pid)) || (0 == success)) {
      remove_file(op,outfilename);
      st_error("join failed");
    }
  }
}

static void check_headers()
{
  int i,ba_warned = 0;

  for (i=0;i<numfiles;i++) {
    if (PROB_NOT_CD(files[i]))
      all_files_cd_quality = 0;

    if (PROB_HDR_INCONSISTENT(files[i]))
      st_error("file '%s' has an inconsistent header",files[i]->filename);

    if (PROB_TRUNCATED(files[i]))
      st_error("file '%s' seems to be truncated",files[i]->filename);
  }

  for (i=1;i<numfiles;i++) {
    if (files[i]->wave_format != files[0]->wave_format)
      st_error("WAVE format differs among these files");

    if (files[i]->channels != files[0]->channels)
      st_error("number of channels differs among these files");

    if (files[i]->samples_per_sec != files[0]->samples_per_sec)
      st_error("samples per second differs among these files");

    if (files[i]->avg_bytes_per_sec != files[0]->avg_bytes_per_sec)
      st_error("average bytes per second differs among these files");

    if (files[i]->bits_per_sample != files[0]->bits_per_sample)
      st_error("bits per sample differs among these files");

    if (files[i]->block_align != files[0]->block_align) {
      if (0 == ba_warned) {
        st_warning("block align differs among these files");
        ba_warned = 1;
      }
    }
  }
}

static void process_files(int argc,char **argv,int start)
{
  int i;

  if (NULL == (files = malloc((numfiles + 1) * sizeof(wave_info *))))
    st_error("could not allocate memory for file info array");

  for (i=0;i<numfiles;i++) {
    if (NULL == (files[i] = new_wave_info(argv[start+i]))) {
      st_error("could not open file '%s'",argv[start+i]);
    }
  }

  files[numfiles] = NULL;

  check_headers();

  if (1 == order)
    alter_file_order(files,numfiles);

  do_join();

  for (i=0;i<numfiles;i++)
    free(files[i]);

  free(files);
}

static void join_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  numfiles = argc - first_arg;

  process_files(argc,argv,first_arg);
}
