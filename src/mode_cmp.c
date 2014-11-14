/*  mode_cmp.c - cmp mode module
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
 * $Id: mode_cmp.c,v 1.33 2004/04/02 00:51:56 jason Exp $
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

#define CMP_SIZE 529200      /* 3 seconds of CD-quality data.  Should be enough to catch
                                2-second silences added to beginning of ripped tracks */
#define CMP_MATCH_SIZE 2352  /* arbitrary number of bytes that must match before attempting
                                to check whether shifted data in the files is identical */
static int align = 0;
static int list = 0;
static int fuzz = 0;
static int fuzz_flag = 0;

static void cmp_main(int,char **,int);

mode_module mode_cmp = {
  "cmp",
  "shncmp",
  "compares PCM WAVE data in two files",
  cmp_main
};

static void show_usage()
{
  printf("Usage: %s [OPTIONS] [file1 file2]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -s       check if WAVE data in the files is identical modulo a byte-shift\n");
  printf("           (within first %d bytes, or first %d seconds of CD-quality data)\n",CMP_SIZE,CMP_SIZE/CD_RATE);
  printf("\n");
  printf("  -l       list offsets and values of all differing bytes\n");
  printf("\n");
  printf("  -f fuzz  fuzz factor - maximum number of allowable mismatched bytes when\n");
  printf("           detecting a byte-shift (can only be used with -s)\n");
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
  printf("  %% %s filename1.wav filename2.shn\n",fullprogname);
  printf("\n");
  printf("  %% %s -s original-filename.wav ripped-filename.wav\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-s"))
      align = 1;
    else if (argc > start && 0 == strcmp(argv[start],"-l"))
      list = 1;
    else if (argc > start && 0 == strcmp(argv[start],"-f")) {
      if (1 == fuzz_flag)
        st_help("too many fuzz factors specified");
      start++;
      fuzz_flag = 1;
      if (argc <= start)
        st_help("missing fuzz factor");
      fuzz = atoi(argv[start]);
      if (fuzz < 0)
        st_help("fuzz factor cannot be negative");
    }
    else {
      st_help("unknown argument: %s",argv[start]);
    }
    start++;
  }

  if (fuzz > 0 && 0 == align)
    st_help("-f can only be used with -s");

  if (start != argc && start != argc - 2)
    st_help("need exactly two files to process");

  *first_arg = start;
}

static void check_headers(wave_info *info1,wave_info *info2,int shift)
{
  int data_size1,data_size2,real_shift = (shift < 0) ? -shift : shift;

  data_size1 = info1->data_size;
  data_size2 = info2->data_size;

  if (shift > 0)
    data_size1 -= real_shift;
  else if (shift < 0)
    data_size2 -= real_shift;

  if (info1->wave_format != info2->wave_format)
    st_error("WAVE format differs between these files");

  if (info1->channels != info2->channels)
    st_error("number of channels differs between these files");

  if (info1->samples_per_sec != info2->samples_per_sec)
    st_error("samples per second differs between these files");

  if (info1->avg_bytes_per_sec != info2->avg_bytes_per_sec)
    st_error("average bytes per second differs between these files");

  if (info1->bits_per_sample != info2->bits_per_sample)
    st_error("bits per sample differs between these files");

  if (info1->block_align != info2->block_align)
    st_warning("block align differs between these files");

  if (data_size1 != data_size2)
    st_warning("size of data to be compared differs between these files -\n"
               "WAVE data will only be compared up to the smaller size");
}

static int my_memcmp(unsigned char *str1,unsigned char *str2,int len,int fuzz)
{
  int i,firstbad = -1,badcount = 0;

  for (i=0;i<len;i++) {
    if (str1[i] != str2[i]) {
      if (0 == fuzz)
        return i;
      if (firstbad < 0)
        firstbad = i;
      badcount++;
      if (badcount > fuzz)
        return firstbad;
    }
  }

  return -1;
}

static void open_file(wave_info *info)
{
  if (0 != open_input_stream(info))
    st_error("could not reopen file '%s' for input",info->filename);
}

static void discard_header(wave_info *info)
{
  unsigned char *header;

  if (NULL == (header = malloc(info->header_size * sizeof(unsigned char))))
    st_error("could not allocate %d bytes for WAVE header",info->header_size);

  if (read_n_bytes(info->input,header,info->header_size) != info->header_size)
    st_error("error while discarding %d-byte header from file '%s'",info->header_size,info->filename);

  free(header);
}

static void cmp_files(wave_info *info1,wave_info *info2,int shift)
{
  unsigned char *buf1,*buf2;
  wlong bytes_to_check,
        bytes_checked = 0,
        bytes,
        shifted_data_size1 = info1->data_size,
        shifted_data_size2 = info2->data_size;
  int offset, real_shift, i, differed = 0, did_l_header = 0;

  /* kluge to work around free(buf2) dumping core if malloc()'d separately */
  if (NULL == (buf1 = malloc(2 * XFER_SIZE * sizeof(unsigned char))))
    st_error("could not allocate %d bytes for comparison buffer",XFER_SIZE);

  buf2 = buf1 + XFER_SIZE;

  real_shift = (shift < 0) ? -shift : shift;

  open_file(info1);
  open_file(info2);

  discard_header(info1);
  discard_header(info2);

  if (shift > 0) {
    if (read_n_bytes(info1->input,buf1,real_shift) != real_shift)
      st_error("error while reading %d bytes from file '%s'",real_shift,info1->filename);

    shifted_data_size1 -= real_shift;
  }
  else if (shift < 0) {
    if (read_n_bytes(info2->input,buf2,real_shift) != real_shift)
      st_error("error while reading %d bytes from file '%s'",real_shift,info2->filename);

    shifted_data_size2 -= real_shift;
  }

  printf("comparing %sWAVE data in files '%s' and '%s' ... ",(0 == shift) ? "" : "aligned ",info1->filename,info2->filename);
  fflush(stdout);

  bytes_to_check = min(shifted_data_size1,shifted_data_size2);

  while (bytes_to_check > 0) {
    bytes = min(bytes_to_check,XFER_SIZE);
    if (read_n_bytes(info1->input,buf1,(int)bytes) != (int)bytes)
      st_error("error while reading %d bytes from file '%s'",(int)bytes,info1->filename);

    if (read_n_bytes(info2->input,buf2,(int)bytes) != (int)bytes)
      st_error("error while reading %d bytes from file '%s'",(int)bytes,info2->filename);

    if (-1 != (offset = my_memcmp(buf1,buf2,bytes,0))) {
      differed = 1;
      if (0 == list) {
        printf("\n\nWAVE data differs at byte offset %lu.\n",bytes_checked+(wlong)offset+1);
        return;
      }
      i = 0;
      while (offset >= 0) {
        i += offset + 1;
        if (0 == did_l_header) {
          printf("\n\n");
          printf("    offset   1   2\n");
          printf("   ----------------\n");
          did_l_header = 1;
        }
        printf("%10ld %3d %3d\n",bytes_checked + i,(int)*(buf1 + i - 1),(int)*(buf2 + i - 1));
        offset = my_memcmp(buf1 + i,buf2 + i,bytes - i,0);
      }
    }
    bytes_to_check -= bytes;
    bytes_checked += bytes;
  }

  close_input_stream(info1);
  close_input_stream(info2);

  if (0 == differed) {
    printf("\n\n%scontents of these files are identical",(0 == shift) ? "" : "aligned ");
    if (shifted_data_size1 != shifted_data_size2)
      printf(" (up to the first %lu bytes of WAVE data)",min(shifted_data_size1,shifted_data_size2));
    printf(".\n");
  }
  else
    printf("\n%scontents of these files differed as indicated above.\n",(0 == shift) ? "" : "aligned ");

  free(buf1);
}

static void open_and_read_beginning(wave_info *info,unsigned char *buf,int bytes)
{
  open_file(info);

  discard_header(info);

  if (read_n_bytes(info->input,buf,(int)bytes) != (int)bytes)
    st_error("error while reading %d bytes from file '%s'",(int)bytes,info->filename);

  close_input_stream(info);
}

static void shift_comparison(wave_info *info1,wave_info *info2)
{
  unsigned char *buf1,*buf2;
  wlong bytes;
  int i,shift = 0,found_possible_shift = 0,real_shift = 0;

  printf("checking for byte-shift between input files...\n\n");

  bytes = min(min(info1->data_size,info2->data_size),CMP_SIZE);

  if (NULL == (buf1 = malloc(2 * bytes * sizeof(unsigned char))))
    st_error("could not allocate %d bytes for comparison buffer",bytes);

  buf2 = buf1 + bytes;

  open_and_read_beginning(info1,buf1,bytes);
  open_and_read_beginning(info2,buf2,bytes);

  for (i=0;i<bytes-CMP_MATCH_SIZE+1;i++) {
    if (-1 == my_memcmp(buf1 + i,buf2,bytes - i,fuzz)) {
      shift = i;
      real_shift = i;
      found_possible_shift = 1;
      break;
    }
    if (-1 == my_memcmp(buf1,buf2 + i,bytes - i,fuzz)) {
      shift = -i;
      real_shift = i;
      found_possible_shift = 1;
      break;
    }
  }

  free(buf1);

  if (1 == found_possible_shift) {
    if (fuzz > 0)
      printf("with fuzz factor %d, ",fuzz);
    if (0 == shift) {
      printf("there is no difference between these files so far.\n");
    }
    else {
      printf("file '%s' seems to have:\n\n",(0 < shift) ? info1->filename : info2->filename);
      printf("  %d extra bytes (%d extra samples",real_shift,(real_shift + 2)/4);
      if (!PROB_NOT_CD(info1) && !PROB_NOT_CD(info2))
        printf(", or %d extra sectors",(real_shift+(CD_BLOCK_SIZE/2))/CD_BLOCK_SIZE);
      printf(")\n\n");
      printf("these extra bytes will be discarded before comparing the data.\n");
    }
    printf("\npreparing to do full comparison...\n\n");

    check_headers(info1,info2,shift);
    cmp_files(info1,info2,shift);
  }
  else
    printf("files '%s' and '%s' do not share identical data within the first %ld bytes.\n",info1->filename,info2->filename,bytes);
}

static void straight_comparison(wave_info *info1,wave_info *info2)
{
  check_headers(info1,info2,0);
  cmp_files(info1,info2,0);
}

static void process_files(char *filename1,char *filename2)
{
  wave_info *info1,*info2;

  if (NULL == (info1 = new_wave_info(filename1)))
    return;

  if (NULL == (info2 = new_wave_info(filename2)))
    return;

  if (1 == align)
    shift_comparison(info1,info2);
  else
    straight_comparison(info1,info2);

  free(info1);
  free(info2);
}

static void process(int argc,char **argv,int start)
{
  char filename1[FILENAME_SIZE],
       filename2[FILENAME_SIZE];

  if (argc < start + 1) {
    /* no filenames were given, so we're reading two file names from standard input. */
    fgets(filename1,FILENAME_SIZE-1,stdin);
    if (0 == feof(stdin)) {
      fgets(filename2,FILENAME_SIZE-1,stdin);
      if (0 == feof(stdin)) {
        filename1[strlen(filename1)-1] = '\0';
        filename2[strlen(filename2)-1] = '\0';
        process_files(filename1,filename2);
      }
      else
        st_error("need two file names to process");
    }
  }
  else
    process_files(argv[start],argv[start+1]);
}

static void cmp_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
