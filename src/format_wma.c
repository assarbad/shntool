/*  format_wma.c - wma format module
 *  Copyright (C) 2000-2006  Jason Jordan <shnutils@freeshell.org>
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

#include "format.h"
#include <string.h>

CVSID("$Id: format_wma.c,v 1.7 2007/01/05 18:25:02 jason Exp $")

static char WMA_MAGIC[] = { 0x30, 0x26, 0xB2, 0x75, 0x00 };

#define ASF_NO_OBJECT                           "00000000-0000-0000-0000-000000000000"
#define ASF_HEADER_OBJECT                       "75B22630-668E-11CF-A6D9-00AA0062CE6C"
#define ASF_FILE_PROPERTIES_OBJECT              "8CABDCA1-A947-11CF-8EE4-00C00C205365"
#define ASF_STREAM_PROPERTIES_OBJECT            "B7DC0791-A9B7-11CF-8EE6-00C00C205365"
#define ASF_HEADER_EXTENSION_OBJECT             "5FBF03B5-A92E-11CF-8EE3-00C00C205365"
#define ASF_CODEC_LIST_OBJECT                   "86D15240-311D-11D0-A3A4-00A0C90348F6"
#define ASF_SCRIPT_COMMAND_OBJECT               "1EFB1A30-0B62-11D0-A39B-00A0C90348F6"
#define ASF_MARKER_OBJECT                       "F487CD01-A951-11CF-8EE6-00C00C205365"
#define ASF_BITRATE_MUTUAL_EXCLUSION_OBJECT     "D6E229DC-35DA-11D1-9034-00A0C90349BE"
#define ASF_ERROR_CORRECTION_OBJECT             "75B22635-668E-11CF-A6D9-00AA0062CE6C"
#define ASF_CONTENT_DESCRIPTION_OBJECT          "75B22633-668E-11CF-A6D9-00AA0062CE6C"
#define ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT "D2D0A440-E307-11D2-97F0-00A0C95EA850"
#define ASF_CONTENT_BRANDING_OBJECT             "2211B3FA-BD23-11D2-B4B7-00A0C955FC6E"
#define ASF_STREAM_BITRATE_PROPERTIES_OBJECT    "7BF875CE-468D-11D1-8D82-006097C9A2B2"
#define ASF_CONTENT_ENCRYPTION_OBJECT           "2211B3FB-BD23-11D2-B4B7-00A0C955FC6E"
#define ASF_EXTENDED_CONTENT_ENCRYPTION_OBJECT  "298AE614-2622-4C17-B935-DAE07EE9289C"
#define ASF_DIGITAL_SIGNATURE_OBJECT            "2211B3FC-BD23-11D2-B4B7-00A0C955FC6E"
#define ASF_PADDING_OBJECT                      "1806D474-CADF-4509-A4BA-9AABCB96AAE8"
#define ASF_AUDIO_MEDIA                         "F8699E40-5B4D-11CF-A8FD-00805F5C442B"
#define ASF_WMA9_LOSSLESS                       0x163

static bool is_our_file(char *);
static FILE *open_for_input(char *,proc_info *);

format_module format_wma = {
  "wma",
  "Windows Media Audio Lossless",
  CVSIDSTR,
  TRUE,
  FALSE,
  FALSE,
  TRUE,
  TRUE,
  FALSE,
  NULL,
  WMA_MAGIC,
  0,
  "wma",
  NULL,
  NULL,
  NULL,
  NULL,
  is_our_file,
  open_for_input,
  NULL,
  NULL,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,proc_info *pinfo)
{
  st_debug1("encountered unsupported WMA lossless audio file: [%s]",filename);

  pinfo->pid = NO_CHILD_PID;

  return NULL;
}

static bool read_guid(FILE *f,unsigned char *guid)
{
  unsigned char guid_bin[16];

  strcpy((char *)guid,ASF_NO_OBJECT);

  if (!read_be_long(f,(unsigned long *)(guid_bin)))
    return FALSE;

  if (!read_be_short(f,(unsigned short *)(guid_bin+4)))
    return FALSE;

  if (!read_be_short(f,(unsigned short *)(guid_bin+6)))
    return FALSE;

  if (!read_le_long(f,(unsigned long *)(guid_bin+8)))
    return FALSE;

  if (!read_le_long(f,(unsigned long *)(guid_bin+12)))
    return FALSE;

  st_snprintf((char *)guid,48,"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X"
            ,*(guid_bin+0)
            ,*(guid_bin+1)
            ,*(guid_bin+2)
            ,*(guid_bin+3)
            ,*(guid_bin+4)
            ,*(guid_bin+5)
            ,*(guid_bin+6)
            ,*(guid_bin+7)
            ,*(guid_bin+8)
            ,*(guid_bin+9)
            ,*(guid_bin+10)
            ,*(guid_bin+11)
            ,*(guid_bin+12)
            ,*(guid_bin+13)
            ,*(guid_bin+14)
            ,*(guid_bin+15)
           );

  st_debug2("found ASF GUID: [%s]",guid);

  return TRUE;
}

static bool read_objsize(FILE *f,unsigned long *objsize)
{
  unsigned long le_long;

  if (!read_le_long(f,objsize))
    return FALSE;

  if (!read_le_long(f,&le_long))
    return FALSE;

  return TRUE;
}

static bool skip_asf_object(FILE *f,unsigned char *guid)
{
  long seek;
  unsigned long objsize;

  if (!strcmp((char *)guid,ASF_HEADER_OBJECT)) {
    seek = 14;
  }
  else {
    if (!read_objsize(f,&objsize))
      return FALSE;
    seek = (long)objsize - 24;
  }

  if (seek < 0)
    return FALSE;

  if (fseek(f,seek,SEEK_CUR))
    return FALSE;

  return TRUE;
}

static bool is_our_file(char *filename)
{
  FILE *f;
  unsigned char guid[48];
  unsigned long le_long;
  unsigned short le_short;

  if (NULL == (f = open_input(filename)))
    return FALSE;

  /* check for ASF_Header_Object */
  if (!read_guid(f,guid) || strcmp((char *)guid,ASF_HEADER_OBJECT))
    goto invalid_asf_header;

  /* scan for ASF_Stream_Object */
  while (strcmp((char *)guid,ASF_STREAM_PROPERTIES_OBJECT)) {
    if (!skip_asf_object(f,guid) || !read_guid(f,guid))
      goto invalid_asf_header;
  }

  /* check stream object fields for lossless audio */

  if (!read_objsize(f,&le_long))
    goto invalid_asf_header;

  /* stream type */
  if (!read_guid(f,guid))
    goto invalid_asf_header;

  if (strcmp((char *)guid,ASF_AUDIO_MEDIA))
    goto invalid_asf_header;

  /* error correction type */
  if (!read_guid(f,guid))
    goto invalid_asf_header;

  /* time offset */
  if (!read_objsize(f,&le_long))
    goto invalid_asf_header;

  /* type-specific data length */
  if (!read_le_long(f,&le_long))
    goto invalid_asf_header;

  /* error correction data length */
  if (!read_le_long(f,&le_long))
    goto invalid_asf_header;

  /* flags */
  if (!read_le_short(f,&le_short))
    goto invalid_asf_header;

  /* reserved */
  if (!read_le_long(f,&le_long))
    goto invalid_asf_header;

  /* codec id */
  if (!read_le_short(f,&le_short))
    goto invalid_asf_header;

  if (ASF_WMA9_LOSSLESS != le_short)
    goto invalid_asf_header;

  fclose(f);
  return TRUE;

invalid_asf_header:

  fclose(f);
  return FALSE;
}
