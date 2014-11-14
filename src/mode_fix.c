/*  mode_fix.c - fix mode module
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
 * $Id: mode_fix.c,v 1.48 2004/04/06 18:45:16 jason Exp $
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

#define POSTFIX_FIXED "fixed"

typedef enum {
  SHIFT_UNKNOWN,
  SHIFT_BACKWARD,
  SHIFT_FORWARD,
  SHIFT_ROUND
} fix_shifts;

static int order = 0;
static int pad = 1;
static int pad_bytes = 0;
static int preview = 0;
static int skip = 1;
static int warned = 0;
static int output_dir_flag = 0;
static int desired_shift = SHIFT_UNKNOWN;
static int numfiles;

static char *output_directory = ".";

static wave_info **files;

static format_module *op = NULL;

static void fix_main(int,char **,int);

mode_module mode_fix = {
  "fix",
  "shnfix",
  "fixes sector-boundary problems with CD-quality PCM WAVE data",
  fix_main
};

static void show_usage()
{
  int i,j=0;

  printf("Usage: %s [OPTIONS] file1 file2 [...]\n",fullprogname);
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
  printf("  -s type  specifies the shift type, where type is one of the following:\n");
  printf("\n");
  printf("             b  shifts track breaks backward to the previous multiple of the\n");
  printf("                block size (%d bytes) [DEFAULT]\n",CD_BLOCK_SIZE);
  printf("             f  shifts track breaks forward to the next multiple of the block\n");
  printf("                size (%d bytes)\n",CD_BLOCK_SIZE);
  printf("             r  rounds track breaks to the nearest multiple of the block size\n");
  printf("                (%d bytes)\n",CD_BLOCK_SIZE);
  printf("\n");
  printf("  -nopad   specifies that zero-padding should not be done (default is to pad)\n");
  printf("\n");
  printf("  -noskip  don't skip first N files that won't be changed from a WAVE data\n");
  printf("           perspective (default is to skip them to avoid unnecessary work)\n");
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
  printf("    %% %s -s r -o shn -nopad *.wav\n",fullprogname);
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
      pad = 0;
    else if (argc > start && 0 == strcmp(argv[start],"-noskip"))
      skip = 0;
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
    else if (argc > start && 0 == strcmp(argv[start],"-s")) {
      start++;
      if (desired_shift != SHIFT_UNKNOWN)
        st_help("too many shift types specified");
      if (argc <= start)
        st_help("missing shift type");
      if (0 == strcmp(argv[start],"b")) {
        desired_shift = SHIFT_BACKWARD;
      }
      else if (0 == strcmp(argv[start],"f")) {
        desired_shift = SHIFT_FORWARD;
      }
      else if (0 == strcmp(argv[start],"r")) {
        desired_shift = SHIFT_ROUND;
      }
      else {
        st_help("unknown shift type: %s",argv[start]);
      }
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

  if (start >= argc - 1)
    st_help("need two or more files to process");

  if (NULL == op) {
    op = formats[found_output];
    st_warning("no output format specified - assuming %s",op->name);
    warned = 1;
  }

  if (SHIFT_UNKNOWN == desired_shift) {
    desired_shift = SHIFT_BACKWARD;
    st_warning("no shift direction specified - assuming backward shift");
    warned = 1;
  }

  *first_arg = start;
}

static void calculate_breaks_backward()
{
  int i,remainder = 0;
  wlong tmp,begin_before = 0,begin_after = 0;

  for (i=0;i<numfiles-1;i++) {
    files[i]->beginning_byte = begin_before;
    files[i]->new_beginning_byte = begin_after;

    tmp = files[i]->data_size + remainder;
    remainder = tmp % CD_BLOCK_SIZE;
    files[i]->new_data_size = tmp - remainder;

    begin_before += files[i]->data_size;
    begin_after += files[i]->new_data_size;
  }
  files[numfiles-1]->new_data_size = files[numfiles-1]->data_size + remainder;
  files[numfiles-1]->beginning_byte = begin_before;
  files[numfiles-1]->new_beginning_byte = begin_after;
}

static void calculate_breaks_forward()
{
  int i,used = 0,remainder;
  wlong tmp,begin_before = 0,begin_after = 0;

  for (i=0;i<numfiles-1;i++) {
    files[i]->beginning_byte = begin_before;
    files[i]->new_beginning_byte = begin_after;

    tmp = files[i]->data_size - used;
    remainder = tmp % CD_BLOCK_SIZE;
    if (remainder) {
      used = CD_BLOCK_SIZE - remainder;
      files[i]->new_data_size = tmp + used;
    }
    else {
      used = 0;
      files[i]->new_data_size = tmp;
    }

    begin_before += files[i]->data_size;
    begin_after += files[i]->new_data_size;
  }
  files[numfiles-1]->new_data_size = files[numfiles-1]->data_size - used;
  files[numfiles-1]->beginning_byte = begin_before;
  files[numfiles-1]->new_beginning_byte = begin_after;
}

static void calculate_breaks_round()
{
  int i,give_or_take = 0;
  wlong tmp,how_much,begin_before = 0,begin_after = 0;

  for (i=0;i<numfiles-1;i++) {
    files[i]->beginning_byte = begin_before;
    files[i]->new_beginning_byte = begin_after;

    tmp = files[i]->data_size + give_or_take;
    how_much = (((tmp + CD_BLOCK_SIZE/2) / CD_BLOCK_SIZE) * CD_BLOCK_SIZE);
    give_or_take = tmp - how_much;
    files[i]->new_data_size = how_much;

    begin_before += files[i]->data_size;
    begin_after += files[i]->new_data_size;
  }
  files[numfiles-1]->new_data_size = files[numfiles-1]->data_size + give_or_take;
  files[numfiles-1]->beginning_byte = begin_before;
  files[numfiles-1]->new_beginning_byte = begin_after;
}

static void calculation_sanity_check()
{
  int i;
  wlong old_total = 0,new_total = 0;

  for (i=0;i<numfiles;i++) {
    old_total += files[i]->data_size;
    new_total += files[i]->new_data_size;
  }

  if (old_total != new_total) {
    st_warning("total WAVE data size differs from newly calculated total -\n"
               "please file a bug report with the following data:");
    fprintf(stderr,"\nshift type: %s\n\n",(SHIFT_BACKWARD == desired_shift)?"backward":
                                         ((SHIFT_FORWARD == desired_shift)?"forward":
                                         ((SHIFT_ROUND == desired_shift)?"round":"unknown")));
    for (i=0;i<numfiles;i++) {
      fprintf(stderr,"file %2d:  data size = %10lu, new data size = %10lu\n",i+1,files[i]->data_size,files[i]->new_data_size);
    }
    fprintf(stderr,"\ntotals :  data size = %10lu, new data size = %10lu\n",old_total,new_total);
    exit(1);
  }
}

static void make_outfilename(char *infilename,char *outfilename)
/* creates an output file name for shifted data, used in fix mode */
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
  strcat(outfilename,POSTFIX_FIXED);
  strcat(outfilename,".");
  strcat(outfilename,op->extension);
}

static void show_potential_changes()
{
  int i;
  char outfilename[FILENAME_SIZE];

  printf("\n");
  printf("Preview of changes:\n");
  printf("-------------------\n");
  printf("\n");
  printf("Track breaks will be %s when necessary.\n\n",
         (SHIFT_BACKWARD == desired_shift)?"shifted backward":
         ((SHIFT_FORWARD == desired_shift)?"shifted forward":
                                           "rounded to the nearest sector boundary"));

  for (i=0;i<numfiles;i++) {
    make_outfilename(files[i]->filename,outfilename);
    printf("%s --> %s\n",files[i]->filename,outfilename);

    printf("  - beginning of track will ");
    if (files[i]->beginning_byte == files[i]->new_beginning_byte)
      printf("remain unchanged");
    else {
      printf("be moved ");
      if (files[i]->beginning_byte > files[i]->new_beginning_byte)
        printf("backward by %lu bytes",files[i]->beginning_byte - files[i]->new_beginning_byte);
      else
        printf("forward by %lu bytes",files[i]->new_beginning_byte - files[i]->beginning_byte);
    }
    printf("\n");

    printf("  - data size will ");
    if (files[i]->data_size == files[i]->new_data_size)
      printf("remain unchanged");
    else {
      if (files[i]->data_size > files[i]->new_data_size)
        printf("decrease by %lu bytes",files[i]->data_size - files[i]->new_data_size);
      else
        printf("increase by %lu bytes",files[i]->new_data_size - files[i]->data_size);
    }
    printf("\n\n");
  }

  if (1 == pad) {
    printf("The last file '%s' would",outfilename);
    if (0 != pad_bytes)
      printf(" be padded with %d zero-bytes.\n",pad_bytes);
    else
      printf("n't need any padding.\n");
  }
  else {
    printf("The last file '%s' wouldn't be padded, ",outfilename);
    if (0 != pad_bytes)
      printf("but would need %d bytes of padding.\n",pad_bytes);
    else
      printf("and wouldn't need it.\n");
  }
}

static int reopen_input_file(int i)
{
  unsigned char *header;

  if (0 != open_input_stream(files[i])) {
    st_warning("could not reopen file '%s' for input",files[i]->filename);
    return 0;
  }

  if (NULL == (header = malloc(files[i]->header_size * sizeof(unsigned char)))) {
    st_warning("could not allocate %d bytes for WAVE header",files[i]->header_size);
    return 0;
  }

  if (read_n_bytes(files[i]->input,header,files[i]->header_size) != files[i]->header_size) {
    st_warning("error while reading %d-byte header",files[i]->header_size);
    free(header);
    return 0;
  }

  free(header);

  return 1;
}

static int open_output_file(int i,char *outfilename)
{
  unsigned char header[CANONICAL_HEADER_SIZE];

  make_outfilename(files[i]->filename,outfilename);

  if (NULL == (files[i]->output = op->output_func(outfilename,&files[i]->output_pid)))
    st_error("could not open file '%s' for output",outfilename);

  make_canonical_header(header,files[i]);

  if (numfiles - 1 == i && 1 == pad)
    put_data_size(header,CANONICAL_HEADER_SIZE,files[i]->new_data_size+pad_bytes);
  else {
    put_data_size(header,CANONICAL_HEADER_SIZE,files[i]->new_data_size);
    if (files[i]->new_data_size & 1)
      put_chunk_size(header,(files[i]->new_data_size + 1) + CANONICAL_HEADER_SIZE - 8);
  }

  if (write_n_bytes(files[i]->output,header,CANONICAL_HEADER_SIZE) != CANONICAL_HEADER_SIZE) {
    st_warning("error while writing %d-byte header",CANONICAL_HEADER_SIZE);
    return 0;
  }

  return 1;
}

static void write_fixed_files()
{
  int cur_input,cur_output,success = 0;
  unsigned long bytes_have,bytes_needed,bytes_to_xfer;
  char outfilename[FILENAME_SIZE];

  cur_input = cur_output = 0;
  bytes_have = (unsigned long)files[cur_input]->data_size;
  bytes_needed = (unsigned long)files[cur_output]->new_data_size;

  if (0 == reopen_input_file(cur_input))
    goto cleanup;

  if (0 == open_output_file(cur_output,outfilename))
    goto cleanup;

  printf("%s --> %s ... ",files[cur_input]->filename,outfilename);
  fflush(stdout);

  while (cur_input < numfiles && cur_output < numfiles) {
    bytes_to_xfer = min(bytes_have,bytes_needed);

    if (transfer_n_bytes(files[cur_input]->input,files[cur_output]->output,bytes_to_xfer) != bytes_to_xfer) {
      st_warning("error while transferring %lu bytes of data",bytes_to_xfer);
      goto cleanup;
    }

    bytes_have -= bytes_to_xfer;
    bytes_needed -= bytes_to_xfer;

    if (0 == bytes_have) {
      printf("done.\n");
      close_input_stream(files[cur_input]);
      files[cur_input]->input = NULL;
      cur_input++;
      if (cur_input < numfiles) {
        bytes_have = (unsigned long)files[cur_input]->data_size;

        if (0 == reopen_input_file(cur_input))
          goto cleanup;

        make_outfilename(files[cur_input]->filename,outfilename);

        printf("%s --> %s ... ",files[cur_input]->filename,outfilename);
        fflush(stdout);
      }
    }

    if (0 == bytes_needed) {
      if (numfiles - 1 == cur_output) {
        if (1 == pad) {
          if (0 != pad_bytes) {
            if (pad_bytes != write_padding(files[cur_output]->output,pad_bytes)) {
              st_warning("error while padding with %d zero-bytes",pad_bytes);
              goto cleanup;
            }
            printf("Padded '%s' with %d zero-bytes.\n",outfilename,pad_bytes);
          }
          else
            printf("No padding needed for '%s'.\n",outfilename);
        }
        else {
          printf("File '%s' was not padded, ",outfilename);
          if (0 != pad_bytes)
            printf("though it needs %d bytes of padding.\n",pad_bytes);
          else
            printf("nor was it needed.\n");

          if ((files[cur_output]->new_data_size & 1) && (1 != write_padding(files[cur_output]->output,1))) {
            st_warning("error while NULL-padding odd-sized data chunk");
            goto cleanup;
          }
        }
      }
      close_output_stream(files[cur_output]);
      files[cur_output]->output = NULL;
      cur_output++;
      if (cur_output < numfiles) {
        bytes_needed = (unsigned long)files[cur_output]->new_data_size;

        if (0 == open_output_file(cur_output,outfilename))
          goto cleanup;
      }
    }
  }

  success = 1;

cleanup:
  if (0 == success) {
    close_output_stream(files[cur_output]);
    remove_file(op,outfilename);
    st_error("fix failed");
  }
}

static void process_files(int argc,char **argv,int start)
{
  int i,j,remainder,
      badfiles = 0,
      needs_fixing = 0,
      found_errors = 0;

  if (NULL == (files = malloc((numfiles + 1) * sizeof(wave_info *))))
    st_error("could not allocate memory for file info array");

  for (i=0;i<numfiles;i++)
    if (NULL == (files[i] = new_wave_info(argv[start+i])))
      badfiles++;

  files[numfiles] = NULL;

  if (badfiles)
    st_error("could not open %d file%s, see above error messages",badfiles,(badfiles>1)?"s":"");

  /* validate files */
  for (i=0;i<numfiles;i++) {
    if (PROB_NOT_CD(files[i])) {
      st_warning("file '%s' is not CD-quality",files[i]->filename);
      found_errors = 1;
    }
    if (PROB_HDR_INCONSISTENT(files[i])) {
      st_warning("file '%s' has an inconsistent header",files[i]->filename);
      found_errors = 1;
    }
    if (PROB_TRUNCATED(files[i])) {
      st_warning("file '%s' seems to be truncated",files[i]->filename);
      found_errors = 1;
    }
    if (PROB_BAD_BOUND(files[i]))
      needs_fixing = 1;
  }

  if (0 != found_errors)
    st_error("could not fix files due to errors, see above");

  if (0 == needs_fixing)
    st_error("everything seems fine, no need for fixing");

  if (1 == order)
    alter_file_order(files,numfiles);

  i = 0;
  while (0 == PROB_BAD_BOUND(files[i]))
    i++;

  if (1 == skip) {
    if (i != 0) {
      st_warning("skipping first %d file%s because %s would not be changed",i,(1==i)?"":"s",(1==i)?"it":"they");
      for (j=0;j<i;j++)
        free(files[j]);
      for (j=i;j<numfiles;j++) {
        files[j-i] = files[j];
        files[j] = NULL;
      }
      numfiles -= i;
    }
  }
  else
    if (1 == warned)
      printf("\n");

  if (numfiles < 2)
    st_error("need two or more files to process");

  switch (desired_shift) {
    case SHIFT_BACKWARD:
      calculate_breaks_backward();
      break;
    case SHIFT_FORWARD:
      calculate_breaks_forward();
      break;
    case SHIFT_ROUND:
      calculate_breaks_round();
      break;
  }

  calculation_sanity_check();

  remainder = files[numfiles-1]->new_data_size % CD_BLOCK_SIZE;
  if (0 != remainder)
    pad_bytes = CD_BLOCK_SIZE - remainder;

  if (1 == preview)
    show_potential_changes();
  else
    write_fixed_files();

  for (i=0;i<numfiles;i++)
    free(files[i]);

  free(files);
}

static void fix_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  numfiles = argc - first_arg;

  process_files(argc,argv,first_arg);
}
