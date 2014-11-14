/*  mode_len.c - len mode module
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
 * $Id: mode_len.c,v 1.35 2004/05/05 07:15:56 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shntool.h"
#include "wave.h"
#include "misc.h"

#define LEN_OK             "-"
#define LEN_NOT_APPLICABLE "x"

typedef enum {
  LEVEL_UNKNOWN = -1,
  LEVEL_BYTES,
  LEVEL_KBYTES,
  LEVEL_MBYTES,
  LEVEL_GBYTES
} len_levels;

static int desired_level = LEVEL_UNKNOWN;
static int processed = 0;
static int all_cd_quality = 1;

static double total_size = 0.0;
static double total_data_size = 0.0;
static double total_disk_size = 0.0;
static double total_length = 0.0;
static double unit_divs[4] = {1.0, 1024.0, 1048576.0, 1073741824.0};

static char *units[4] = {"B ","KB","MB","GB"};

static void len_main(int,char **,int);

mode_module mode_len = {
  "len",
  "shnlen",
  "displays length, size and properties of PCM WAVE data",
  len_main
};

static void show_usage()
{
  int i;

  printf("Usage: %s [OPTIONS] [files]\n",fullprogname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -u unit  specifies alternate output unit for totals line, where unit is one\n");
  printf("           of the following:\n");
  printf("\n");
  printf("              b  shows totals in terms of bytes (DEFAULT)\n");
  printf("             kb  shows totals in terms of kilobytes\n");
  printf("             mb  shows totals in terms of megabytes\n");
  printf("             gb  shows totals in terms of gigabytes\n");
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
  printf("  %% %s",fullprogname);
  for (i=0;formats[i];i++)
    if (formats[i]->input_func)
      printf(" *.%s",formats[i]->extension);
  printf("\n");
  printf("\n");
  printf("  %% find /your/audio/dir -type f | %s -u mb\n",fullprogname);
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
    else if (argc > start && 0 == strcmp(argv[start],"-u")) {
      start++;
      if (desired_level != LEVEL_UNKNOWN)
        st_help("too many alternate totals units specified");
      if (argc <= start)
        st_help("missing alternate totals unit");
      if (0 == strcmp(argv[start],"b")) {
        desired_level = LEVEL_BYTES;
      }
      else if (0 == strcmp(argv[start],"kb")) {
        desired_level = LEVEL_KBYTES;
      }
      else if (0 == strcmp(argv[start],"mb")) {
        desired_level = LEVEL_MBYTES;
      }
      else if (0 == strcmp(argv[start],"gb")) {
        desired_level = LEVEL_GBYTES;
      }
      else
        st_help("unknown unit: %s",argv[start]);
    }
    else
      st_help("unknown argument: %s",argv[start]);
    start++;
  }

  if (LEVEL_UNKNOWN == desired_level)
    desired_level = LEVEL_BYTES;

  *first_arg = start;
}

static void show_len_banner()
{
  printf("    length     expanded size   cdr  WAVE problems filename\n");
}

static void print_formatted_length(wave_info *info)
{
  int i,len;

  len = strlen(info->m_ss);

  if (PROB_NOT_CD(info)) {
    for (i=0;i<13-len;i++)
      printf(" ");
    printf("%s",info->m_ss);
  }
  else {
    for (i=0;i<12-len;i++)
      printf(" ");
    printf("%s ",info->m_ss);
  }
}

static void show_stats(wave_info *info)
{
  wlong appended_bytes;

  print_formatted_length(info);

  printf("%14lu   ",info->total_size);

  /* CD-R properties */

  printf(" ");

  if (PROB_NOT_CD(info))
    printf("c%s%s",LEN_NOT_APPLICABLE,LEN_NOT_APPLICABLE);
  else {
    printf("%s",LEN_OK);
    if (PROB_BAD_BOUND(info))
      printf("b");
    else
      printf("%s",LEN_OK);

    if (PROB_TOO_SHORT(info))
      printf("s");
    else
      printf("%s",LEN_OK);
  }

  /* WAVE properties */

  printf("   ");

  if (PROB_HDR_NOT_CANONICAL(info))
    printf("h");
  else
    printf("%s",LEN_OK);

  if (PROB_EXTRA_CHUNKS(info))
    printf("e");
  else
    printf("%s",LEN_OK);

  /* problems */

  printf("   ");

  if (info->file_has_id3v2_tag)
    printf("3");
  else
    printf("%s",LEN_OK);

  if (PROB_DATA_NOT_ALIGNED(info))
    printf("a");
  else
    printf("%s",LEN_OK);

  if (PROB_HDR_INCONSISTENT(info))
    printf("i");
  else
    printf("%s",LEN_OK);

  if ((0 == info->input_format->is_compressed) && (0 == info->input_format->is_translated)) {
    appended_bytes = info->actual_size - info->total_size - info->id3v2_tag_size;

    if (PROB_TRUNCATED(info))
      printf("t");
    else
      printf("%s",LEN_OK);

    if (PROB_JUNK_APPENDED(info) && appended_bytes > 0)
      printf("j");
    else
      printf("%s",LEN_OK);
  }
  else
    printf("%s%s",LEN_NOT_APPLICABLE,LEN_NOT_APPLICABLE);

  printf("   %s\n",info->filename);
}

static void show_totals_line()
{
  wave_info *info;
  wlong seconds;
  double ratio;

  if (0 == (info = new_wave_info(NULL)))
    st_error("could not allocate memory for totals");

  if (1 == all_cd_quality) {
    /* Note to users from the year 2376:  the m:ss.ff value on the totals line will only be
     * correct as long as the total data size is less than 689 terabytes (2^32 * 176400 bytes).
     * Hopefully, by then you'll all have 2048-bit processers to go with your petabyte keychain
     * raid arrays, and this won't be an issue.
     */

    /* calculate floor of total length in seconds */
    seconds = (wlong)(total_data_size / (double)CD_RATE);

    /* since length_to_str() only considers (data_size % CD_RATE) when the file is CD-quality,
     * we don't need to risk overflowing a 32-bit unsigned long with a double - we can cheat
     * and just store the modulus.
     */
    info->data_size = (wlong)(total_data_size - (double)(seconds * CD_RATE));

    info->length = seconds;
    info->rate = CD_RATE;
  }
  else {
    info->problems |= PROBLEM_NOT_CD_QUALITY;
    info->length = (wlong)total_length;
  }

  info->exact_length = total_length;

  length_to_str(info);

  print_formatted_length(info);

  ratio = (processed > 0) ? (total_disk_size / total_size) : 0.0;

  total_size /= unit_divs[desired_level];

  if (desired_level > 0)
    printf("%14.2f",total_size);
  else
    printf("%14.0f",total_size);

  printf(" %s                    (total%s for %d file%s, %0.4f overall compression ratio)\n",
    units[desired_level],(processed != 1)?"s":"",processed,(processed != 1)?"s":"",ratio);

  free(info);
}

static void update_totals(wave_info *info)
{
  total_size += (double)info->total_size;
  total_data_size += (double)info->data_size;
  total_length += (double)info->data_size / (double)info->avg_bytes_per_sec;

  if (PROB_NOT_CD(info))
    all_cd_quality = 0;

  total_disk_size += (double)info->actual_size;

  processed++;
}

static void process_file(char *filename)
{
  wave_info *info;

  if (NULL == (info = new_wave_info(filename)))
    return;

  show_stats(info);
  update_totals(info);

  free(info);
}

static void process(int argc,char **argv,int start)
{
  int i;
  char filename[FILENAME_SIZE];

  show_len_banner();

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

  show_totals_line();
}

static void len_main(int argc,char **argv,int start)
{
  int first_arg;

  parse(argc,argv,start,&first_arg);

  process(argc,argv,first_arg);
}
