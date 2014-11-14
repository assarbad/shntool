/*  format_null.c - /dev/null format module
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
 * $Id: format_null.c,v 1.17 2004/03/30 07:52:42 jason Exp $
 */

#include <stdio.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"

static FILE *open_for_output(char *,int *);
static int is_our_file(char *);

format_module format_null = {
  "null",
  "sends output to /dev/null",
  "null (actually /dev/null)",
  0,
  0,
  NULL,
  NULL,
  is_our_file,
  NULL,
  open_for_output,
  NULL,
  NULL
};

static FILE *open_for_output(char *filename,int *pid)
{
  *pid = NO_CHILD_PID;
  return open_output("/dev/null");
}

static int is_our_file(char *filename)
{
  return 0;
}
