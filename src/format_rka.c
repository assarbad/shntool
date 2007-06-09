/*  format_rka.c - rka format module
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

CVSID("$Id: format_rka.c,v 1.1 2007/01/05 17:48:42 jason Exp $")

#define RKA "rkau"

#define RKA_MAGIC "RKA7"

static FILE *open_for_input(char *,proc_info *);

format_module format_rka = {
  "rka",
  "RKA Audio audio compressor",
  CVSIDSTR,
  TRUE,
  FALSE,
  FALSE,
  TRUE,
  TRUE,
  FALSE,
  NULL,
  RKA_MAGIC,
  0,
  "rka",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  open_for_input,
  NULL,
  NULL,
  NULL,
  NULL
};

static FILE *open_for_input(char *filename,proc_info *pinfo)
{
  st_debug1("encountered unsupported RKAU file: [%s]",filename);

  pinfo->pid = NO_CHILD_PID;

  return NULL;
}
