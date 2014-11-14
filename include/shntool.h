/*  shntool.h - global definitions
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
 * $Id: shntool.h,v 1.19 2004/03/29 08:47:11 jason Exp $
 */

#ifndef __SHNTOOL_H__
#define __SHNTOOL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "format.h"
#include "mode.h"

#define RELEASE   VERSION
#define COPYRIGHT "Copyright 2000-2004"
#define AUTHOR    "Jason Jordan <shnutils@freeshell.org>"
#define URLS      "  http://www.etree.org/shnutils/\n  http://shnutils.freeshell.org/"

#define SHNTOOL_DEBUG_ENV "SHNTOOL_DEBUG"

#define PROGNAME_SIZE 256
#define BUF_SIZE      2048
#define FILENAME_SIZE 2048
#define XFER_SIZE     262144

#define MAX_MODULE_NAME_LENGTH "5"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#ifdef HAVE_VSNPRINTF
#define my_vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#else
#define my_vsnprintf(a,b,c,d) vsprintf(a,c,d)
#endif

extern mode_module *modes[];
extern format_module *formats[];

extern char *progname;
extern char *progmode;
extern char fullprogname[];
extern char *author;
extern char *version;
extern char *copyright;
extern char *urls;

extern int aliased;
extern int shntool_debug;

void st_warning(char *, ...);
void st_error(char *, ...);
void st_debug(char *, ...);
void st_help(char *, ...);

#endif
