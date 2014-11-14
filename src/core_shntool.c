/*  core_shntool.c - functions to handle mode verification and execution
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
 * $Id: core_shntool.c,v 1.25 2004/04/12 03:01:49 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "shntool.h"
#include "misc.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

char *progname,
     *progmode = NULL,
     fullprogname[PROGNAME_SIZE],
     *author = AUTHOR,
     *version = RELEASE,
     *copyright = COPYRIGHT,
     *urls = URLS;

int aliased = 0,
    shntool_debug = 0;

static void show_supported_mode_modules()
{
  int i;

  printf("Supported modes:\n");
  printf("\n");
  for (i=0;modes[i];i++)
    printf("  %" MAX_MODULE_NAME_LENGTH "s  (%s)\n",modes[i]->name,modes[i]->description);
  printf("\n");
  printf("  For help with any of the above modes, type:\n");
  printf("\n");
  printf("    %s mode -h\n",progname);
  printf("\n");
}

static void show_supported_format_modules()
{
  int i;

  printf("Supported file formats:\n");
  for (i=0;formats[i];i++) {
    printf("\n");
    printf("  %" MAX_MODULE_NAME_LENGTH "s  (%s)\n",formats[i]->name,formats[i]->description);
    printf("    %" MAX_MODULE_NAME_LENGTH "s+ audio %sformat - supports ","",(formats[i]->is_compressed)?"compression ":"");

    if (formats[i]->input_func && formats[i]->output_func) {
      if (formats[i]->encoder && formats[i]->decoder && (0 == strcmp(formats[i]->encoder,formats[i]->decoder))) {
        printf("input and output via '%s'",formats[i]->encoder);
      }
      else {
        printf("input");
        if (formats[i]->decoder) {
          printf(" via '%s'",formats[i]->decoder);
        }
        printf(" and output");
        if (formats[i]->encoder) {
          printf(" via '%s'",formats[i]->encoder);
        }
      }
    }
    else {
      if (formats[i]->input_func) {
        printf("input only");
        if (formats[i]->decoder) {
          printf(" via '%s'",formats[i]->decoder);
        }
      }
      else {
        printf("output only");
        if (formats[i]->encoder) {
          printf(" via '%s'",formats[i]->encoder);
        }
      }
    }
    printf("\n");
  }
}

static void show_usage()
{
  printf("Usage: %s mode ...\n",progname);
  printf("       %s [OPTIONS]\n",progname);
  printf("\n");
  printf("Options:\n");
  printf("\n");
  printf("  -m    shows detailed mode module information\n");
  printf("  -f    shows detailed format module information\n");
  printf("  -v    shows version information\n");
  printf("  -h    shows this help screen\n");
  printf("\n");

  exit(0);
}

static void module_sanity_check()
{
  int i;

  if (modes[0]) {
    for (i=0;modes[i];i++) {
      if (NULL == modes[i]->name)
        st_error("found a mode module without a name");

      if (NULL == modes[i]->description)
        st_error("mode module '%s' has no description",modes[i]->name);

      if (NULL == modes[i]->run_main)
        st_error("mode module '%s' does not provide run_main()",modes[i]->name);
    }

    for (i=1;modes[i];i++)
      if (0 == strcmp(modes[0]->name,modes[i]->name))
        st_error("found modes with identical name: %s",modes[0]->name);
  }
  else
    st_error("no mode modules found");

  if (formats[0]) {
    for (i=0;formats[i];i++) {
      if (NULL == formats[i]->name)
        st_error("found a format module without a name");

      if (NULL == formats[i]->description)
        st_error("format module '%s' has no description",formats[i]->name);

      if (NULL == formats[i]->extension)
        st_error("format module '%s' has no extension",formats[i]->name);

      if (NULL == formats[i]->is_our_file)
        st_error("format module '%s' does not provide is_our_file()",formats[i]->name);

      if (NULL == formats[i]->input_func && NULL == formats[i]->output_func)
        st_error("format module '%s' doesn't support input or output",formats[i]->name);

      if (NULL == formats[i]->input_func && formats[i]->decoder)
        st_error("format module '%s' doesn't support input but defines a decoder '%s'",formats[i]->name,formats[i]->decoder);

      if (NULL == formats[i]->output_func && formats[i]->encoder)
        st_error("format module '%s' doesn't support output but defines an encoder '%s'",formats[i]->name,formats[i]->encoder);
    }

    for (i=1;formats[i];i++)
      if (0 == strcmp(formats[0]->name,formats[i]->name))
        st_error("found formats with identical name: %s",formats[0]->name);
  }
  else
    st_error("no file formats modules found");
}

static void parse_main(int argc,char **argv)
{
  int i;

  /* look for a module alias matching progname */

  for (i=0;modes[i];i++) {
    if (NULL != modes[i]->alias && 0 == strcmp(progname,modes[i]->alias)) {
      aliased = 1;
      progmode = modes[i]->name;
      modes[i]->run_main(argc,argv,1);
      return;
    }
  }

  /* failing that, look for a module name matching argv[1] */

  if (argc < 2)
    st_help("missing arguments");

  for (i=0;modes[i];i++) {
    if (0 == strcmp(argv[1],modes[i]->name)) {
      /* found mode - now run it and quit */
      progmode = modes[i]->name;
      strcat(fullprogname," ");
      strcat(fullprogname,progmode);
      modes[i]->run_main(argc,argv,2);
      return;
    }
  }

  if (argc > 1 && 0 == strcmp(argv[1],"-h"))
    show_usage();
  else if (argc > 1 && 0 == strcmp(argv[1],"-v"))
    internal_version();
  else if (argc > 1 && 0 == strcmp(argv[1],"-m"))
    show_supported_mode_modules();
  else if (argc > 1 && 0 == strcmp(argv[1],"-f"))
    show_supported_format_modules();
  else if (argc > 1 && 0 == strcmp(argv[1],"-j")) {
    printf("Guru Meditashn #00000002.9091968B\n");
  }
  else {
    /* didn't find any matching modes */
    st_help("invalid mode or option: %s",argv[1]);
  }
}

int main(int argc,char **argv)
{
  signal(SIGPIPE,SIG_IGN);

  /* set up globals */
  progname = (0 == strrchr(argv[0],'/')) ? argv[0] : strrchr(argv[0],'/') + 1;
  strcpy(fullprogname,progname);

  /* show debugging output? */
  shntool_debug = getenv(SHNTOOL_DEBUG_ENV) ? 1 : 0;

  /* sanity check for modules */
  module_sanity_check();

  /* parse command line */
  parse_main(argc,argv);

  return 0;
}
