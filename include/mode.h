/*  mode.h - mode module definitions
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
 * $Id: mode.h,v 1.6 2004/03/23 09:47:13 jason Exp $
 */

#ifndef __MODE_H__
#define __MODE_H__

typedef struct {
  char  *name;                         /* mode name, specified on command line */
  char  *alias;                        /* alternate name that invokes this mode */
  char  *description;                  /* one line description of this mode */
  void (*run_main)(int,char **,int);   /* main() function for this mode */
} mode_module;

#endif
