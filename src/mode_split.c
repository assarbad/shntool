/*  mode_split.c - split mode module
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
 * $Id: mode_split.c,v 1.50 2004/04/12 03:46:21 jason Exp $
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

#define SPLIT_PREFIX "split-track"
#define MAX_PIECES   256

enum {
  SPLIT_INPUT_UNKNOWN,
  SPLIT_INPUT_RAW,
  SPLIT_INPUT_CUE
};

static int split_point_file_flag = 0;
static int prefix_flag = 0;
static int output_dir_flag = 0;
static int numfiles = 0;
static int preview = 0;
static int offset_flag = 0;
static int offset = 1;
static int input_is_cd_quality = 0;
static int repeated_split_point_flag = 0;
static int input_type = SPLIT_INPUT_UNKNOWN;

static char *split_point_file = NULL;
static char *prefix = SPLIT_PREFIX;
static char *output_directory = ".";
static char *repeated_split_point = NULL;

static wave_info *files[MAX_PIECES];

static format_module *op = NULL;

static void split_main(int,char **,int);

mode_module mode_split = {
  "split",
  "shnsplit",
  "splits PCM WAVE data from one file into multiple files",
  split_main
};

static void show_usage()
{
  int i, j = 0;

  printf("Usage: %s [OPTIONS] file\n",fullprogname);
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
  printf("  -f file  specifies a file from which to read split point data\n");
  printf("\n");
  printf("  -l len   split input file into files of length len\n");
  printf("\n");
  printf("  -n name  specifies a prefix for output files (default is \"%s\")\n",SPLIT_PREFIX);
  printf("\n");
  printf("  -c num   start counting from num when naming output files (default is 1)\n");
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
  printf("  If no split point file is given, then split points are read from standard\n");
  printf("  input.  See the man page (or README.txt) for more details about split points.\n");
  printf("\n");
  printf("  ------------\n");
  printf("  Sample uses:\n");
  printf("  ------------\n");
  printf("\n");
  printf("  %% %s -f offsets.txt -n bandYYYY-MM-DDd2t bigfile.wav\n",fullprogname);
  printf("\n");
  printf("  %% %s -p -d disc1 bigfile.shn < splitpoints\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-p"))
      preview = 1;
    else if (argc > start && 0 == strcmp(argv[start],"-c")) {
      if (1 == offset_flag)
        st_help("too many starting counts specified");
      start++;
      offset_flag = 1;
      if (argc <= start)
        st_help("missing starting count");
      offset = atoi(argv[start]);
      if (offset < 0)
        st_help("starting count cannot be negative");
    }
    else if (argc > start && 0 == strcmp(argv[start],"-d")) {
      if (1 == output_dir_flag)
        st_help("too many output directories specified");
      start++;
      output_dir_flag = 1;
      if (argc <= start)
        st_help("missing output directory");
      output_directory = argv[start];
    }
    else if (argc > start && 0 == strcmp(argv[start],"-n")) {
      if (1 == prefix_flag)
        st_help("too many prefixes files specified");
      start++;
      prefix_flag = 1;
      if (argc <= start)
        st_help("missing prefix");
      prefix = argv[start];
    }
    else if (argc > start && 0 == strcmp(argv[start],"-f")) {
      if (1 == split_point_file_flag)
        st_help("too many split point files specified");
      start++;
      split_point_file_flag = 1;
      if (argc <= start)
        st_help("missing split point file");
      split_point_file = argv[start];
    }
    else if (argc > start && 0 == strcmp(argv[start],"-l")) {
      if (1 == repeated_split_point_flag)
        st_help("too many repeated split points specified");
      start++;
      repeated_split_point_flag = 1;
      if (argc <= start)
        st_help("missing repeated split point");
      repeated_split_point = argv[start];
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

  if (start != argc - 1)
    st_help("need exactly one file to process");

  if (NULL == op) {
    op = formats[found_output];
    st_warning("no output format specified - assuming %s",op->name);
  }

  *first_arg = start;
}

static void make_outfilename(char *outfilename,int num)
{
  if (1 == output_dir_flag)
    my_snprintf(outfilename,FILENAME_SIZE,"%s/%s%03d.%s",output_directory,prefix,num+offset,op->extension);
  else
    my_snprintf(outfilename,FILENAME_SIZE,"%s%03d.%s",prefix,num+offset,op->extension);
}

static void split_file(wave_info *info)
{
  unsigned char header[CANONICAL_HEADER_SIZE];
  char outfilename[FILENAME_SIZE];
  int current,output_pid,success = 0;
  wint discard,bytes;
  FILE *output;

  if (0 != open_input_stream(info))
    st_error("could not reopen file '%s' for input",info->filename);

  discard = info->header_size;
  while (discard > 0) {
    bytes = min(discard,CANONICAL_HEADER_SIZE);
    if (read_n_bytes(info->input,header,bytes) != bytes)
      st_error("error while discarding %d-byte header from file '%s'",info->header_size,info->filename);
    discard -= bytes;
  }

  printf("\n");
  printf("Input file '%s' is being split into %d pieces.\n",info->filename,numfiles);
  printf("\n");

  for (current=0;current<numfiles;current++) {
    make_outfilename(outfilename,current);

    if (NULL == (output = op->output_func(outfilename,&output_pid)))
      st_error("could not open file '%s' for output",outfilename);

    files[current]->chunk_size = files[current]->data_size + CANONICAL_HEADER_SIZE - 8;
    files[current]->channels = info->channels;
    files[current]->samples_per_sec = info->samples_per_sec;
    files[current]->avg_bytes_per_sec = info->avg_bytes_per_sec;
    files[current]->block_align = info->block_align;
    files[current]->bits_per_sample = info->bits_per_sample;
    files[current]->wave_format = info->wave_format;
    files[current]->rate = info->rate;
    files[current]->length = files[current]->data_size / (wlong)info->rate;
    files[current]->exact_length = (double)files[current]->data_size / (double)info->rate;

    if (PROB_ODD_SIZED_DATA(files[current]))
      files[current]->chunk_size++;

    make_canonical_header(header,files[current]);

    length_to_str(files[current]);

    printf("Writing %s (%s) ... ",outfilename,files[current]->m_ss);

    fflush(stdout);

    if (write_n_bytes(output,header,CANONICAL_HEADER_SIZE) != CANONICAL_HEADER_SIZE) {
      st_warning("error while writing %d-byte header",CANONICAL_HEADER_SIZE);
      goto cleanup;
    }

    if (transfer_n_bytes(info->input,output,files[current]->data_size) != files[current]->data_size) {
      st_warning("error while transferring %ld bytes of data",files[current]->data_size);
      goto cleanup;
    }

    if (PROB_ODD_SIZED_DATA(files[current]) && (1 != write_padding(output,1))) {
      st_warning("error while NULL-padding odd-sized data chunk");
      goto cleanup;
    }

    printf("done.\n");

    close_output(output,output_pid);
  }

  close_input_stream(info);

  success = 1;

cleanup:
  if (0 == success) {
    close_output(output,output_pid);
    remove_file(op,outfilename);
    st_error("split failed");
  }
}

static void show_preview(char *filename,wave_info *info)
{
  int i;
  char outfilename[FILENAME_SIZE];

  printf("\n");
  printf("Preview of changes:\n");
  printf("-------------------\n");
  printf("\n");
  printf("File '%s' will be split into %d pieces:\n",filename,numfiles);
  for (i=0;i<numfiles;i++) {
    make_outfilename(outfilename,i);
    printf("\n");

    files[i]->rate = info->rate;
    files[i]->length = files[i]->data_size / (wlong)info->rate;
    files[i]->exact_length = (double)files[i]->data_size / (double)info->rate;

    length_to_str(files[i]);

    printf("  + %s (%s)\n",outfilename,files[i]->m_ss);
    printf("    - this file will contain %ld bytes of WAVE data (plus %d-byte header)\n",files[i]->data_size,CANONICAL_HEADER_SIZE);
    if (input_is_cd_quality) {
      if (PROB_TOO_SHORT(files[i]))
        printf("    - this file will be too short to be burned\n");
      if (PROB_BAD_BOUND(files[i]))
        printf("    - this file will not be cut on a sector boundary\n");
    }
  }
}

static int is_a_digit(char c)
{
  switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;
    default:
      return 0;
  }
  return 1;
}

static int is_numeric(unsigned char *buf)
{
  unsigned char *p = buf;
  wlong bytes;

  if (0 == strlen(buf))
    return -1;

  while ('\0' != *p) {
    if (0 == is_a_digit(*p))
      return -1;
    p++;
  }

  bytes =
#ifdef HAVE_ATOL
      (wlong)atol(buf);
#else
      (wlong)atoi(buf);
#endif

  return bytes;
}

static wlong is_m_ss(unsigned char *buf,wave_info *info)
{
  unsigned char *colon,*p;
  int len,min,sec;
  wlong bytes;

  len = strlen(buf);

  if (len < 4)
    return -1;

  colon = buf + len - 3;

  if (':' != *colon)
    return -1;

  p = buf;
  while ('\0' != *p) {
    if (p != colon) {
      if (0 == is_a_digit(*p))
        return -1;
    }
    p++;
  }

  *colon = '\0';

  min = atoi(buf);
  sec = atoi(colon+1);

  if (sec >= 60)
    st_error("invalid value for seconds: %d",sec);

  bytes = (wlong)(min * info->rate * 60) +
          (wlong)(sec * info->rate);

  return bytes;
}

static wlong is_m_ss_ff(unsigned char *buf,wave_info *info)
{
  unsigned char *colon,*dot,*p;
  int len,min,sec,frames;
  wlong bytes;

  len = strlen(buf);

  if (len < 7)
    return -1;

  colon = buf + len - 6;
  dot = buf + len - 3;

  if (':' != *colon || '.' != *dot)
    return -1;

  p = buf;
  while ('\0' != *p) {
    if (p != colon && p != dot) {
      if (0 == is_a_digit(*p))
        return -1;
    }
    p++;
  }

  *colon = '\0';
  *dot = '\0';

  if (0 == input_is_cd_quality)
    st_error("m:ss.ff format can only be used with CD-quality input files");

  min = atoi(buf);
  sec = atoi(colon+1);
  frames = atoi(dot+1);

  if (sec >= 60)
    st_error("invalid value for seconds: %d",sec);

  if (frames >= 75)
    st_error("invalid value for frames: %d",frames);

  bytes = (wlong)(min * CD_RATE * 60) +
          (wlong)(sec * CD_RATE) +
          (wlong)(frames * CD_BLOCK_SIZE);

  return bytes;
}

static wlong is_m_ss_nnn(unsigned char *buf,wave_info *info)
{
  unsigned char *colon,*dot,*p;
  int len,min,sec,ms,nearest_byte,nearest_frame;
  wlong bytes;

  len = strlen(buf);

  if (len < 8)
    return -1;

  colon = buf + len - 7;
  dot = buf + len - 4;

  if (':' != *colon || '.' != *dot)
    return -1;

  p = buf;
  while ('\0' != *p) {
    if (p != colon && p != dot) {
      if (0 == is_a_digit(*p))
        return -1;
    }
    p++;
  }

  *colon = '\0';
  *dot = '\0';

  min = atoi(buf);
  sec = atoi(colon+1);
  ms = atoi(dot+1);

  if (sec >= 60)
    st_error("invalid value for seconds: %d",sec);

  nearest_byte = (int)((((double)ms * (double)info->rate) / 1000.0) + 0.5);

  bytes = (wlong)(min * info->rate * 60) +
          (wlong)(sec * info->rate);

  if (input_is_cd_quality) {
    nearest_frame = (int)((double)ms * .075 + 0.5) * CD_BLOCK_SIZE;
    if (0 == nearest_frame && 0 == bytes) {
      st_warning("closest sector boundary to %d:%02d.%03d is the beginning of the file - rounding up to first sector boundary",min,sec,ms);
      nearest_frame = CD_BLOCK_SIZE;
    }
    if (nearest_frame != nearest_byte) {
      st_warning("rounding %d:%02d.%03d (offset: %lu) to nearest sector boundary (offset: %lu)",
              min,sec,ms,bytes+(wlong)nearest_byte,bytes+(wlong)nearest_frame);
      bytes += (wlong)nearest_frame;
    }
  }
  else {
    bytes += nearest_byte;
  }

  return bytes;
}

static wlong smrt_parse(unsigned char *data,wave_info *info)
/* currently not so smart, so I gave it the Homer Simpson spelling :) */
{
  wlong bytes;

  /* check for all digits */
  if (-1 != (bytes = is_numeric(data)))
    return bytes;

  /* check for m:ss */
  if (-1 != (bytes = is_m_ss(data,info)))
    return bytes;

  /* check for m:ss.ff */
  if (-1 != (bytes = is_m_ss_ff(data,info)))
    return bytes;

  /* check for m:ss.nnn */
  if (-1 != (bytes = is_m_ss_nnn(data,info)))
    return bytes;

  /* it was not in any of these formats */

  st_error("split point '%s' is not in bytes, m:ss, m:ss.ff, or m:ss.nnn format",data);

  return -1;
}

static void create_new_splitfile()
{
  if (MAX_PIECES - 1 == numfiles)
    st_error("too many split files would be created - maximum is %d",MAX_PIECES);

  if (NULL == (files[numfiles] = new_wave_info(NULL)))
    st_error("could not allocate memory for split points array");
}

static void adjust_splitfile(int whichfile)
{
  if (input_is_cd_quality) {
    if (files[whichfile]->data_size < CD_MIN_BURNABLE_SIZE) {
      files[whichfile]->problems |= PROBLEM_CD_BUT_TOO_SHORT;
      st_warning("file %d will be too short to be burned%s",whichfile+1,
              (0 != files[whichfile]->data_size % CD_BLOCK_SIZE)?", and will not be cut on a sector boundary":"");
      if (0 != files[whichfile]->data_size % CD_BLOCK_SIZE)
        files[whichfile]->problems |= PROBLEM_CD_BUT_BAD_BOUND;
    }
    else if (0 != files[whichfile]->data_size % CD_BLOCK_SIZE) {
      files[whichfile]->problems |= PROBLEM_CD_BUT_BAD_BOUND;
      st_warning("file %d will not be cut on a sector boundary",whichfile+1);
    }
  }
  else {
    files[whichfile]->problems |= PROBLEM_NOT_CD_QUALITY;
  }
}

static void trim(unsigned char *data)
/* parse out raw lengths, or lengths on INDEX lines of cue sheets */
{
  unsigned char *p,*q;

  if (0 == strlen(data))
    return;

  if (strstr(data,"INDEX") && (NULL == strstr(data,":"))) {
    strcpy(data,"");
    return;
  }

  /* isolate length */
  p = data + strlen(data) - 1;

  while (p >= data && (0 == is_a_digit(*p) && ':' != *p && '.' != *p)) {
    *p = 0;
    p--;
  }

  while (p >= data && (is_a_digit(*p) || ':' == *p || '.' == *p))
    p--;

  p++;

  /* copy length to beginning of buffer */
  q = data;

  while (0 != *p) {
    *q = *p;
    p++;
    q++;
  }

  *q = 0;

  /* convert m:ss:ff to m:ss.ff */

  p = strrchr(data,':');
  q = strchr(data,':');

  if (p && q && p != q)
    *p = '.';
}

static void get_length_token(FILE *input,unsigned char *token)
{
  strcpy(token,"");

  if (feof(input))
    return;

  switch (input_type) {
    case SPLIT_INPUT_UNKNOWN:
      fgets(token,BUF_SIZE-1,input);
      if (feof(input))
        return;
      if (strstr(token,"FILE")) {
        input_type = SPLIT_INPUT_CUE;
        get_length_token(input,token);
      }
      else {
        input_type = SPLIT_INPUT_RAW;
        trim(token);
      }
      break;
    case SPLIT_INPUT_RAW:
      fgets(token,BUF_SIZE-1,input);
      if (feof(input))
        return;
      trim(token);
      break;
    case SPLIT_INPUT_CUE:
      fgets(token,BUF_SIZE-1,input);
      while (0 == feof(input) && (strstr(token,"REM") || NULL == strstr(token,"INDEX 01")))
        fgets(token,BUF_SIZE-1,input);
      if (feof(input))
        return;
      trim(token);
      break;
  }
}

static void read_split_points_file(wave_info *info)
{
  FILE *fd;
  unsigned char data[BUF_SIZE];
  wlong current,previous = 0;

  if (1 == split_point_file_flag) {
    if (NULL == (fd = fopen(split_point_file,"rb")))
      st_error("could not open split point file '%s'",split_point_file);
  }
  else
    fd = stdin;

  get_length_token(fd,data);

  while (0 == feof(fd)) {
    current = smrt_parse(data,info);

    if (0 == numfiles && 0 == current) {
      st_warning("discarding initial zero-valued split point");
    }
    else {
      create_new_splitfile();

      files[numfiles]->beginning_byte = current;
      if (files[numfiles]->beginning_byte <= previous)
        st_error("split point %lu is not greater than previous split point %lu",files[numfiles]->beginning_byte,previous);

      files[numfiles]->data_size = files[numfiles]->beginning_byte - previous;

      adjust_splitfile(numfiles);

      numfiles++;

      previous = current;
    }

    get_length_token(fd,data);
  }

  if (NULL == (files[numfiles] = new_wave_info(NULL)))
    st_error("could not allocate memory for split points array");

  numfiles++;

  if (1 == split_point_file_flag)
    fclose(fd);

  if (1 == numfiles)
    st_error("no split points given - nothing to do");
}

static void read_split_points_repeated(wave_info *info)
{
  wlong repeated_split_size = smrt_parse(repeated_split_point,info);

  if (repeated_split_size >= info->data_size)
    st_error("repeated split size is not less than data size of input file - nothing to do");

  while ((numfiles + 1) * repeated_split_size < info->data_size) {
    create_new_splitfile();

    files[numfiles]->beginning_byte = (numfiles+1) * repeated_split_size;

    files[numfiles]->data_size = repeated_split_size;

    adjust_splitfile(numfiles);

    numfiles++;
  }

  if (NULL == (files[numfiles] = new_wave_info(NULL)))
    st_error("could not allocate memory for split points array");

  numfiles++;
}

static void process_file(char *filename)
{
  int i;
  wave_info *info;

  if (NULL == (info = new_wave_info(filename)))
    st_error("cannot continue due to error(s) shown above");

  input_is_cd_quality = (PROB_NOT_CD(info) ? 0 : 1);

  if (repeated_split_point_flag)
    read_split_points_repeated(info);
  else
    read_split_points_file(info);

  if (files[numfiles-2]->beginning_byte > info->data_size)
    st_error("split points go beyond input file's data size");

  if (files[numfiles-2]->beginning_byte == info->data_size) {
    free(files[numfiles-1]);
    numfiles--;
  }
  else {
    files[numfiles-1]->beginning_byte = info->data_size;
    files[numfiles-1]->data_size = info->data_size - files[numfiles-2]->beginning_byte;

    adjust_splitfile(numfiles-1);
  }

  if (1 == preview)
    show_preview(info->filename,info);
  else
    split_file(info);

  for (i=0;i<numfiles;i++)
    free(files[i]);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  process_file(argv[start]);
}

static void split_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
