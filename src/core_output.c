/*  core_output.c - generic output functions
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
 * $Id: core_output.c,v 1.11 2004/03/29 08:47:11 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "shntool.h"
#include "misc.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static char msgbuf[BUF_SIZE];

static void print_prefix()
{
  if (1 == aliased || NULL == progmode)
    fprintf(stderr,"%s: ",progname);
  else
    fprintf(stderr,"%s [%s]: ",progname,progmode);
}

static void print_msgtype(char *msgtype,int line)
{
  int i;

  if (0 == line) {
    fprintf(stderr,"%s",msgtype);
  }
  else {
    for (i=0;i<strlen(msgtype);i++)
      fprintf(stderr," ");
  }
}

static void print_lines(char *msgtype,char *msg)
{
  int line = 0;
  char *head, *tail;

  head = tail = msgbuf;
  while (*head != '\0') {
    if (*head == '\n') {
      *head = '\0';

      print_prefix();
      print_msgtype(msgtype,line);
      fprintf(stderr,"%s\n",tail);

      tail = head + 1;
      line++;
    }
    head++;
  }

  print_prefix();
  print_msgtype(msgtype,line);
  fprintf(stderr,"%s\n",tail);
}

void st_error(char *msg, ...)
{
  va_list args;

  va_start(args,msg);

  my_vsnprintf(msgbuf,BUF_SIZE,msg,args);

  print_lines("error: ",msgbuf);

  va_end(args);

  exit(1);
}

void st_help(char *msg, ...)
{
  va_list args;

  va_start(args,msg);

  my_vsnprintf(msgbuf,BUF_SIZE,msg,args);

  print_lines("error: ",msgbuf);

  va_end(args);

  print_prefix();
  fprintf(stderr,"\n");
  print_prefix();
  fprintf(stderr,"type '%s -h' for help\n",fullprogname);

  exit(1);
}

void st_warning(char *msg, ...)
{
  va_list args;

  va_start(args,msg);

  my_vsnprintf(msgbuf,BUF_SIZE,msg,args);

  print_lines("warning: ",msgbuf);

  va_end(args);
}

void st_debug(char *msg, ...)
{
  va_list args;

  if (0 == shntool_debug)
    return;

  va_start(args,msg);

  my_vsnprintf(msgbuf,BUF_SIZE,msg,args);

  print_lines("debug: ",msgbuf);

  va_end(args);
}
