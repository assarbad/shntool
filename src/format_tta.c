/*  format_tta.c - tta format module
 *  Copyright (C) 2000-2007  Jason Jordan <shnutils@freeshell.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "format.h"

CVSID("$Id: format_tta.c,v 1.21 2007/01/06 07:40:52 jason Exp $")

#define TTA "ttaenc"

#define TTA_MAGIC "TTA1"

#ifdef WIN32
static char default_decoder_args[] = "-d -s -o - " FILENAME_PLACEHOLDER;
#else
static char default_decoder_args[] = "-d -s -o " TERMODEVICE " " FILENAME_PLACEHOLDER;
#endif

format_module format_tta = {
  "tta",
  "TTA Lossless Audio Codec",
  CVSIDSTR,
  TRUE,
  FALSE,
  FALSE,
  TRUE,
  TRUE,
  TRUE,
  NULL,
  TTA_MAGIC,
  0,
  "tta",
  TTA,
  default_decoder_args,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
