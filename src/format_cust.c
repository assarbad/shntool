/*  format_cust.c - custom output format module
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
 * $Id: format_cust.c,v 1.6 2004/03/29 08:47:11 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "fileio.h"
#include "misc.h"
#include "shntool.h"

#define CUSTOM_EXT "custom"
#define OUTFILESTR "%f"

static char *output_args[MAX_CHILD_ARGS];
static int num_output_args = 0;
static char *ext = CUSTOM_EXT;

static FILE *open_for_output(char *,int *);
static int is_our_file(char *);
static void get_arguments(int,char **,int *);

format_module format_cust = {
  "cust",
  "custom output format module",
  CUSTOM_EXT,
  0,
  0,
  NULL,
  NULL,
  is_our_file,
  NULL,
  open_for_output,
  NULL,
  get_arguments
};

static void usage(char *errmsg)
{
  fprintf(stderr,"'cust' output format usage:\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  { program arg1 arg2 ... argN }\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"To specify an extension (e.g. 'abc'):\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  ext=abc { program arg1 arg2 ... argN }\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"NOTE: One of the arguments must contain the file name placeholder '%s',\n",OUTFILESTR);
  fprintf(stderr,"      which %s will replace with the output filename.\n",progname);
  fprintf(stderr,"\n");
  fprintf(stderr,"Error parsing argument list, see below:\n");
  fprintf(stderr,"\n");

  st_error("cust format: %s",errmsg);
}

static void get_arguments(int argc,char **argv,int *argpos)
{
  int found_terminator = 0,
      found_filename = 0,
      found_program = 0;

  (*argpos)++;

  if (*argpos >= argc) {
    usage("missing '{'");
  }

  if (argv[*argpos][0] == 'e' && argv[*argpos][1] == 'x' &&
      argv[*argpos][2] == 't' && argv[*argpos][3] == '=') {
    ext = argv[*argpos]+4;
    (*argpos)++;
  }

  if (strcmp(argv[*argpos],"{")) {
    usage("missing '{'");
  }
  (*argpos)++;

  while (0 == found_terminator) {
    if (*argpos >= argc) {
      usage("missing '}'");
    }

    if (0 == strcmp(argv[*argpos],"}")) {
      found_terminator = 1;
      break;
    }

    if (argv[*argpos][0] == 'e' && argv[*argpos][1] == 'x' &&
        argv[*argpos][2] == 't' && argv[*argpos][3] == '=') {
      ext = argv[*argpos]+4;
    }
    else {
      if (num_output_args >= MAX_CHILD_ARGS) {
        st_error("too many custom format arguments given - maximum is %d",MAX_CHILD_ARGS);
      }

      if (strstr(argv[*argpos],OUTFILESTR)) {
        found_filename = 1;
      }

      if (0 == num_output_args && 0 == found_filename) {
        found_program = 1;
      }

      output_args[num_output_args] = argv[*argpos];
      num_output_args++;
    }

    (*argpos)++;
  }

  if (0 == found_program) {
    usage("missing program name");
  }

  if (0 == found_filename) {
    usage("missing file name placeholder");
  }
}

static FILE *open_for_output(char *filename,int *pid)
{
  FILE *input,*output;
  char newfilename[FILENAME_SIZE];
  char newarg[FILENAME_SIZE];
  int i;

  for (i=0;i<num_output_args;i++) {
    if (strstr(output_args[i],OUTFILESTR)) {
      /* build new filename with proper extension, if specified */
      strcpy(newfilename,filename);
      newfilename[FILENAME_SIZE-1] = 0;
      newfilename[strlen(newfilename)-strlen(CUSTOM_EXT)] = 0;
      strcat(newfilename,ext);
      newfilename[FILENAME_SIZE-1] = 0;

      /* build new argument, replacing %f with output filename */
      strcpy(newarg,output_args[i]);
      *(strstr(newarg,OUTFILESTR)) = 0;
      strcat(newarg,newfilename);
      strcat(newarg,strstr(output_args[i],OUTFILESTR) + strlen(OUTFILESTR));

      arg_add(newarg);
    }
    else {
      arg_add(output_args[i]);
    }
  }

  *pid = spawn_output(&input,&output);

  if (input)
    fclose(input);

  return output;
}

static int is_our_file(char *filename)
{
  return 0;
}
