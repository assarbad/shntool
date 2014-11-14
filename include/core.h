/*  core.h - private type definitions, defines and macros
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

/*
 * $Id: core.h,v 1.90 2007/10/22 06:36:27 jason Exp $
 */

#ifndef __CORE_H__
#define __CORE_H__

/* program info */
#define RELEASE   VERSION
#define COPYRIGHT "Copyright (C) 2000-2007"
#define AUTHOR    "Jason Jordan <shnutils@freeshell.org>"
#define URL1      "http://www.etree.org/shnutils/"
#define URL2      "http://shnutils.freeshell.org/"

/* options reserved for global use - modes cannot use these */
#define GLOBAL_OPTS        "DHP:hi:qr:vw"
#define GLOBAL_OPTS_OUTPUT "O:a:d:o:z:"
#define GLOBAL_OPTS_CORE   "fhjmv"

/* set this environment variable to enable debugging.  can also use -D, but this enables it earlier */
#define SHNTOOL_DEBUG_ENV "ST_DEBUG"

/* various buffer sizes */
#define PROGNAME_SIZE 256

#ifdef HAVE_VSNPRINTF
#define st_vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#else
#define st_vsnprintf(a,b,c,d) vsprintf(a,c,d)
#endif

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(x) sys_errlist[x]
#endif

/* macro for natural filename sorting */
#ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#    define PARAMS(Args) Args
#  else
#    define PARAMS(Args) ()
#  endif
#endif

/* ID3v2 definitions */
#define ID3V2_MAGIC "ID3"

/* id3v2 header */
typedef struct _id3v2_header {
  char magic[3];
  unsigned char version[2];
  unsigned char flags[1];
  unsigned char size[4];
} id3v2_header;

/* file clobber actions */
typedef enum {
  CLOBBER_ACTION_ASK,
  CLOBBER_ACTION_ALWAYS,
  CLOBBER_ACTION_NEVER
} clobber_action_types;

/* progress indicator types */
typedef enum {
  PROGRESS_NONE,
  PROGRESS_PERCENT,
  PROGRESS_DOT,
  PROGRESS_SPIN,
  PROGRESS_FACE
} progress_types;

/* argument sources */
typedef enum {
  ARGSRC_FORMAT,
  ARGSRC_CMDLINE,
  ARGSRC_ENV
} argument_sources;

/* mode and format module arrays */
extern mode_module *st_modes[];
extern format_module *st_formats[];

/* private global options */
typedef struct _private_opts {
  char *progname;
  char *progmode;
  char  fullprogname[PROGNAME_SIZE];
  int   debug_level;
  int   clobber_action;
  int   reorder_type;
  int   progress_type;
  bool  is_aliased;
  bool  show_hmmss;
  bool  suppress_warnings;
  bool  suppress_stderr;
  bool  screen_dirty;
  mode_module *mode;
} private_opts;

extern private_opts st_priv;

/* miscellaneous functions */

/* generic version printing function */
void st_version(void);

/* functions for building argument lists in format modules */
void arg_init(format_module *);

#endif
