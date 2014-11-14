/*  core_wave.c - functions for parsing WAVE headers
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
 * $Id: core_wave.c,v 1.73 2004/05/05 07:15:56 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "shntool.h"
#include "convert.h"
#include "fileio.h"
#include "misc.h"
#include "wave.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#endif

int is_valid_file(wave_info *info)
/* determines whether the given filename (info->filename) is a regular file, and is readable */
{
  struct stat sz;
  FILE *f;

  if (stat(info->filename,&sz)) {
    if (errno == ENOENT)
      st_warning("cannot open '%s' because it does not exist",info->filename);
    else if (errno == EACCES)
      st_warning("cannot open '%s' due to insufficient permissions",info->filename);
    else if (errno == EFAULT)
      st_warning("cannot open '%s' due to bad address",info->filename);
    else if (errno == ENOMEM)
      st_warning("cannot open '%s' because the kernel ran out of memory",info->filename);
    else if (errno == ENAMETOOLONG)
      st_warning("cannot open '%s' because the file name is too long",info->filename);
    else
      st_warning("cannot open '%s' due to an unknown problem",info->filename);
    return 0;
  }
  if (0 == S_ISREG(sz.st_mode)) {
    if (S_ISLNK(sz.st_mode))
      st_warning("cannot open '%s' because it is a symbolic link, not a regular file",info->filename);
    else if (S_ISDIR(sz.st_mode))
      st_warning("cannot open '%s' because it is a directory, not a regular file",info->filename);
    else if (S_ISCHR(sz.st_mode))
      st_warning("cannot open '%s' because it is a character device, not a regular file",info->filename);
    else if (S_ISBLK(sz.st_mode))
      st_warning("cannot open '%s' because it is a block device, not a regular file",info->filename);
    else if (S_ISFIFO(sz.st_mode))
      st_warning("cannot open '%s' because it is a named pipe, not a regular file",info->filename);
    else if (S_ISSOCK(sz.st_mode))
      st_warning("cannot open '%s' because it is a socket, not a regular file",info->filename);
    return 0;
  }

  info->actual_size = (wlong)sz.st_size;

  if (NULL == (f = fopen(info->filename,"rb"))) {
    st_warning("could not open file '%s': %s",info->filename,
#ifdef HAVE_STRERROR
          strerror(errno)
#else
          sys_errlist[errno]
#endif
          );
    return 0;
  }

  fclose(f);

  return 1;
}

int aiff_input_kluge(unsigned char *header,wave_info *info)
/* kluge to set data_size and chunk_size, since sox can't fseek() on
 * stdout to backfill these values.  This relies on values in the
 * COMM chunk.
 */
{
  FILE *f;
  unsigned long be_long = 0,chunk_size,samples = 0;
  unsigned short be_short = 0,channels = 0,bits_per_sample = 0;
  unsigned char tag[4];
  int is_compressed = 0;

  st_debug("performing AIFF input header kluges for file '%s'",info->filename);

  if (NULL == (f = open_input(info->filename)))
    return 0;

  /* look for FORM header */
  if (0 == read_tag(f,tag) || tagcmp(tag,AIFF_FORM))
    goto aiff_kluge_failed;

  /* skip FORM chunk size, read in FORM type */
  if (0 == read_be_long(f,&be_long) || 0 == read_tag(f,tag))
    goto aiff_kluge_failed;

  /* if FORM type is not AIFF or AIFC, bail out */
  if (tagcmp(tag,AIFF_FORM_TYPE_AIFF) && tagcmp(tag,AIFF_FORM_TYPE_AIFC))
    goto aiff_kluge_failed;

  if (0 == tagcmp(tag,AIFF_FORM_TYPE_AIFC))
    is_compressed = 1;

  /* now let's check AIFC compression type - it's in the COMM chunk */
  while (1) {
    /* read chunk id */
    if (0 == read_tag(f,tag))
      goto aiff_kluge_failed;

    if (0 == tagcmp(tag,AIFF_COMM))
      break;

    /* not COMM, so read size of this chunk and skip it */
    if (0 == read_be_long(f,&chunk_size) || 0 != fseek(f,(long)chunk_size,SEEK_CUR))
      goto aiff_kluge_failed;
  }

  /* now read channels, samples, and bits/sample from COMM chunk */
  if (0 == read_be_long(f,&chunk_size) || 0 == read_be_short(f,&channels) ||
      0 == read_be_long(f,&samples) || 0 == read_be_short(f,&bits_per_sample) ||
      0 == read_be_long(f,&be_long) || 0 == read_be_long(f,&be_long) ||
      0 == read_be_short(f,&be_short))
  {
    goto aiff_kluge_failed;
  }

  if (is_compressed) {
    if (0 == read_tag(f,tag))
      goto aiff_kluge_failed;

    if (tagcmp(tag,AIFF_COMPRESSION_NONE) && tagcmp(tag,AIFF_COMPRESSION_SOWT)) {
      st_debug("found unsupported AIFF-C compression type [%c%c%c%c]",tag[0],tag[1],tag[2],tag[3]);
      goto aiff_kluge_failed;
    }
  }

  fclose(f);

  /* set proper data size */
  info->data_size = channels * samples * (bits_per_sample/8);

  /* now set chunk size based on data size, and the canonical WAVE header size, which sox generates */
  info->chunk_size = info->data_size + CANONICAL_HEADER_SIZE - 8;
  if (PROB_ODD_SIZED_DATA(info))
    info->chunk_size++;

  make_canonical_header(header,info);

  return 1;

aiff_kluge_failed:

  fclose(f);
  return 0;
}

int ape_input_kluge(unsigned char *header,wave_info *info)
{
  wlong adjusted_data_size;

  st_debug("performing APE input header kluges for file '%s'",info->filename);

  adjusted_data_size = info->data_size;
  if (PROB_ODD_SIZED_DATA(info))
    adjusted_data_size++;

  if (info->chunk_size < adjusted_data_size)
    info->chunk_size = info->header_size + adjusted_data_size - 8;

  put_data_size(header,info->header_size,info->data_size);

  return 1;
}

int do_header_kluges(unsigned char *header,wave_info *info)
{
  int failed = 0;

  if (0 == strcmp(info->input_format->name,"aiff") && 0 == aiff_input_kluge(header,info))
    failed = 1;
  else if (0 == strcmp(info->input_format->name,"ape") && 0 == ape_input_kluge(header,info))
    failed = 1;

  if (failed) {
    st_warning("while processing file '%s': %s to wav input kluge failed",info->filename,info->input_format->name);
    return 0;
  }

  return 1;
}

int verify_wav_header_internal(wave_info *info,int verbose)
/* verifies that data coming in on the file descriptor info->input describes a valid WAVE header */
{
  unsigned long le_long=0,bytes;
  unsigned char buf[2];
  unsigned char tag[4];
  int header_len = 0;

  /* look for "RIFF" in header */
  if (0 == read_tag(info->input,tag) || tagcmp(tag,WAVE_RIFF)) {
    if (0 == tagcmp(tag,AIFF_FORM)) {
      if (verbose) st_warning("while processing file '%s': file contains AIFF data, which is currently not supported",info->filename);
    }
    else {
      if (verbose) st_warning("while processing file '%s': WAVE header is missing RIFF tag - possible truncated/corrupt file",info->filename);
    }
    return 0;
  }

  if (0 == read_le_long(info->input,&info->chunk_size)) {
    if (verbose) st_warning("while processing file '%s': could not read chunk size from WAVE header",info->filename);
    return 0;
  }

  /* look for "WAVE" in header */
  if (0 == read_tag(info->input,tag) || tagcmp(tag,WAVE_WAVE)) {
    if (verbose) st_warning("while processing file '%s': WAVE header is missing WAVE tag",info->filename);
    return 0;
  }

  header_len += 12;

  for (;;) {
    if (0 == read_tag(info->input,tag) || 0 == read_le_long(info->input,&le_long)) {
      if (verbose) st_warning("while processing file '%s': reached end of file while looking for fmt tag",info->filename);
      return 0;
    }
    header_len += 8;
    if (0 == tagcmp(tag,WAVE_FMT))
      break;
    bytes = le_long;
    while (bytes > 0) {
      if (fread(buf, 1, 1, info->input) != 1) {
        if (verbose) st_warning("while processing file '%s': reached end of file while jumping ahead %lu bytes during search for fmt tag",info->filename,le_long);
        return 0;
      }
      bytes--;
    }
    header_len += le_long;
  }

  if (le_long < 16) {
    if (verbose) st_warning("while processing file '%s': fmt chunk in WAVE header was too short",info->filename);
    return 0;
  }

  /* now we read the juicy stuff */
  if (0 == read_le_short(info->input,&info->wave_format)) {
    if (verbose) st_warning("while processing file '%s': reached end of file while reading format",info->filename);
    return 0;
  }

  switch (info->wave_format) {
    case WAVE_FORMAT_PCM:
      break;
    default:
      if (verbose) st_warning("while processing file '%s': unsupported format 0x%04x (%s) - only PCM data is supported at this time",
            info->filename,info->wave_format,format_to_str(info->wave_format));
      return 0;
  }

  if (0 == read_le_short(info->input,&info->channels)) {
    if (verbose) st_warning("while processing file '%s': reached end of file while reading channels",info->filename);
    return 0;
  }

  if (0 == read_le_long(info->input,&info->samples_per_sec)) {
    if (verbose) st_warning("while processing file '%s': reached end of file while reading samples/sec",info->filename);
    return 0;
  }

  if (0 == read_le_long(info->input,&info->avg_bytes_per_sec)) {
    if (verbose) st_warning("while processing file '%s': reached end of file while reading average bytes/sec",info->filename);
    return 0;
  }

  if (0 == read_le_short(info->input,&info->block_align)) {
    if (verbose) st_warning("while processing file '%s': reached end of file while reading block align",info->filename);
    return 0;
  }

  if (0 == read_le_short(info->input,&info->bits_per_sample)) {
    if (verbose) st_warning("while processing file '%s': reached end of file while reading bits/sample",info->filename);
    return 0;
  }

  header_len += 16;

  le_long -= 16;

  if (0 != le_long) {
    bytes = le_long;
    while (bytes > 0) {
      if (fread(buf, 1, 1, info->input) != 1) {
        if (verbose) st_warning("while processing file '%s': reached end of file while jumping ahead %lu bytes",info->filename,le_long);
        return 0;
      }
      bytes--;
    }
    header_len += le_long;
  }

  /* now let's look for the data chunk.  Following the string "data" is the
     length of the following WAVE data. */
  for (;;) {
    if (0 == read_tag(info->input,tag) || 0 == read_le_long(info->input,&le_long)) {
      if (verbose) st_warning("while processing file '%s': reached end of file while looking for data tag",info->filename);
      return 0;
    }
    header_len += 8;
    if (0 == tagcmp(tag,WAVE_DATA))
      break;
    bytes = le_long;
    while (bytes > 0) {
      if (fread(buf, 1, 1, info->input) != 1) {
        if (verbose) st_warning("while processing file '%s': end of file encountered when jumping ahead %lu bytes while looking for data tag",info->filename,le_long);
        return 0;
      }
      bytes--;
    }
    header_len += le_long;
  }

  info->data_size = le_long;

  info->header_size = header_len;

  if (0 == do_header_kluges(NULL,info))
    return 0;

  info->rate = ((wint)info->samples_per_sec * (wint)info->channels * (wint)info->bits_per_sample) / 8;

  info->total_size = info->chunk_size + 8;

  /* per RIFF specs, data chunk is always an even number of bytes - a NULL pad byte should be present when data size is odd */
  info->padded_data_size = info->data_size;
  if (PROB_ODD_SIZED_DATA(info))
    info->padded_data_size++;

  info->extra_riff_size = info->total_size - (info->padded_data_size + info->header_size);

  info->length = info->data_size / info->rate;
  info->exact_length = (double)info->data_size / (double)info->rate;

  if (info->channels == CD_CHANNELS &&
      info->bits_per_sample == CD_BITS_PER_SAMPLE &&
      info->samples_per_sec == CD_SAMPLES_PER_SEC &&
/*
  removed, since this doesn't seem to matter
      info->block_align == CD_BLOCK_ALIGN &&
*/
      info->avg_bytes_per_sec == CD_RATE &&
      info->rate == CD_RATE)
  {
    if (info->data_size < CD_MIN_BURNABLE_SIZE)
      info->problems |= PROBLEM_CD_BUT_TOO_SHORT;
    if (info->data_size % CD_BLOCK_SIZE != 0)
      info->problems |= PROBLEM_CD_BUT_BAD_BOUND;
  }
  else
    info->problems |= PROBLEM_NOT_CD_QUALITY;

  if (info->header_size != CANONICAL_HEADER_SIZE)
    info->problems |= PROBLEM_HEADER_NOT_CANONICAL;

  if (info->data_size > info->total_size - (wlong)info->header_size)
    info->problems |= PROBLEM_HEADER_INCONSISTENT;

  if ((info->data_size % info->block_align) != 0)
    info->problems |= PROBLEM_DATA_NOT_ALIGNED;

  if ((0 == info->input_format->is_compressed) && (0 == info->input_format->is_translated)) {
    if (info->total_size < (info->actual_size - info->id3v2_tag_size))
      info->problems |= PROBLEM_JUNK_APPENDED;

    if (info->total_size > (info->actual_size - info->id3v2_tag_size))
      info->problems |= PROBLEM_MAY_BE_TRUNCATED;
  }

  if (info->extra_riff_size > 0)
    info->problems |= PROBLEM_EXTRA_CHUNKS;

  length_to_str(info);

  /* header looks ok */
  return 1;
}

wave_info *new_wave_info(char *filename)
/* if filename is NULL, return a fresh wave_info struct with all data zero'd out.
 * Otherwise, check that the file referenced by filename exists, is readable, and
 * contains a valid WAVE header - if so, return a wave_info struct filled out with
 * the values in the WAVE header, otherwise return NULL.
 */
{
  int i;
  FILE *f;
  wave_info *info;
  char buf[8];
  char msg[BUF_SIZE],tmp[BUF_SIZE];

  if (NULL == (info = malloc(sizeof(wave_info)))) {
    st_warning("could not allocate memory for WAVE info struct");
    goto invalid_wave_data;
  }

  /* set defaults */
  memset((void *)info,0,sizeof(wave_info));

  if (NULL == filename)
    return info;

  info->filename = filename;

  if (0 == is_valid_file(info))
    goto invalid_wave_data;

  /* check which format module (if any) handles this file */
  for (i=0;formats[i];i++) {
    if (formats[i]->input_func && formats[i]->is_our_file(info->filename)) {

      info->input_format = formats[i];

      /* check if file contains an ID3v2 tag, and set flag accordingly */
      if (NULL == (f = open_input_internal(info->filename,&info->file_has_id3v2_tag,&info->id3v2_tag_size))) {
        st_warning("could not open file '%s' to set ID3v2 flag",info->filename);
        goto invalid_wave_data;
      }

      fclose(f);

      /* make sure the file can be opened by the output format - this skips over any ID3v2 tags in the stream */
      if (0 != open_input_stream(info)) {
        st_warning("could not open file '%s', though the '%s' format module claims to support it",info->filename,formats[i]->name);
        goto invalid_wave_data;
      }

      /* make sure we can read data from the output format (primarily to ensure the decoder program is sending us data) */
      if (1 != fread(&buf,1,1,info->input)) {
        my_snprintf(msg,BUF_SIZE,"could not read data from file '%s' using the '%s' format module - possible causes:\n",
                    info->filename,info->input_format->name);

        if (info->input_format->decoder) {
          if (info->file_has_id3v2_tag) {
            my_snprintf(tmp,BUF_SIZE,"+ decoder program '%s' may not handle files with ID3v2 tags - removing the tag might fix this\n",
                        info->input_format->decoder);
            strcat(msg,tmp);
          }

          my_snprintf(tmp,BUF_SIZE,"+ decoder program '%s' may not have been found - verify it is installed and in your PATH\n",
                      info->input_format->decoder);
          strcat(msg,tmp);
        }

        strcat(msg,"+ this file may be unsupported, truncated or otherwise corrupt");

        st_warning(msg);

        goto invalid_wave_data;
      }

      close_input_stream(info);
      info->input = NULL;

      /* reopen as input stream - this skips over any ID3v2 tags in the stream */
      if (0 != open_input_stream(info))
        goto invalid_wave_data;

      /* finally, make sure a proper WAVE header is being sent */
      if (0 == verify_wav_header_internal(info,1))
        goto invalid_wave_data;

      close_input_stream(info);
      info->input = NULL;

      /* success */
      return info;
    }
  }

  /* if we got here, no file format modules claimed to handle the file */

  st_warning("file '%s' is not handled by any of the builtin format modules",info->filename);

  if ((f = open_input_internal(info->filename,&info->file_has_id3v2_tag,&info->id3v2_tag_size))) {
    i = read_n_bytes_quiet(f,buf,4);
    buf[i] = 0;

    if (info->file_has_id3v2_tag)
      st_debug("after skipping %d-byte ID3v2 tag, file '%s' has %d-byte magic header 0x%08X [%s]",info->id3v2_tag_size,info->filename,i,uchar_to_ulong_be(buf),buf);
    else
      st_debug("file '%s' has %d-byte magic header 0x%08X [%s]",info->filename,i,uchar_to_ulong_be(buf),buf);

    fclose(f);
  }

invalid_wave_data:

  if (info) {
    if (info->input) {
      close_input_stream(info);
      info->input = NULL;
    }
    free(info);
  }

  return NULL;
}

void make_canonical_header(unsigned char *header,wave_info *info)
/* constructs a canonical WAVE header from the values in the wave_info struct */
{
  if (NULL == header)
    return;

  tagcpy(header,WAVE_RIFF);
  ulong_to_uchar_le(header+4,info->chunk_size);
  tagcpy(header+8,WAVE_WAVE);
  tagcpy(header+12,WAVE_FMT);
  ulong_to_uchar_le(header+16,0x00000010);
  ushort_to_uchar_le(header+20,info->wave_format);
  ushort_to_uchar_le(header+22,info->channels);
  ulong_to_uchar_le(header+24,info->samples_per_sec);
  ulong_to_uchar_le(header+28,info->avg_bytes_per_sec);
  ushort_to_uchar_le(header+32,info->block_align);
  ushort_to_uchar_le(header+34,info->bits_per_sample);
  tagcpy(header+36,WAVE_DATA);
  ulong_to_uchar_le(header+40,info->data_size);
}

char *format_to_str(wshort format)
{
  switch (format) {
    case WAVE_FORMAT_UNKNOWN:
      return "Microsoft Official Unknown";
    case WAVE_FORMAT_PCM:
      return "Microsoft PCM";
    case WAVE_FORMAT_ADPCM:
      return "Microsoft ADPCM";
    case WAVE_FORMAT_IEEE_FLOAT:
      return "IEEE Float";
    case WAVE_FORMAT_ALAW:
      return "Microsoft A-law";
    case WAVE_FORMAT_MULAW:
      return "Microsoft U-law";
    case WAVE_FORMAT_OKI_ADPCM:
      return "OKI ADPCM format";
    case WAVE_FORMAT_IMA_ADPCM:
      return "IMA ADPCM";
    case WAVE_FORMAT_DIGISTD:
      return "Digistd format";
    case WAVE_FORMAT_DIGIFIX:
      return "Digifix format";
    case WAVE_FORMAT_DOLBY_AC2:
      return "Dolby AC2";
    case WAVE_FORMAT_GSM610:
      return "GSM 6.10";
    case WAVE_FORMAT_ROCKWELL_ADPCM:
      return "Rockwell ADPCM";
    case WAVE_FORMAT_ROCKWELL_DIGITALK:
      return "Rockwell DIGITALK";
    case WAVE_FORMAT_G721_ADPCM:
      return "G.721 ADPCM";
    case WAVE_FORMAT_G728_CELP:
      return "G.728 CELP";
    case WAVE_FORMAT_MPEG:
      return "MPEG";
    case WAVE_FORMAT_MPEGLAYER3:
      return "MPEG Layer 3";
    case WAVE_FORMAT_G726_ADPCM:
      return "G.726 ADPCM";
    case WAVE_FORMAT_G722_ADPCM:
      return "G.722 ADPCM";
  }
  return "Unknown";
}

void put_chunk_size(unsigned char *header,unsigned long new_chunk_size)
/* replaces the chunk size at beginning of the wave header */
{
  if (NULL == header)
    return;

  ulong_to_uchar_le(header+4,new_chunk_size);
}

void put_data_size(unsigned char *header,int header_size,unsigned long new_data_size)
/* replaces the size reported in the "data" chunk of the wave header with the new size -
   also updates chunk size at beginning of the wave header */
{
  if (NULL == header)
    return;

  ulong_to_uchar_le(header+header_size-4,new_data_size);
  put_chunk_size(header,new_data_size+header_size-8);
}
