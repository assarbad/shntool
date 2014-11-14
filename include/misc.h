/*  misc.h - miscellaneous function definitions
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
 * $Id: misc.h,v 1.32 2004/04/07 05:09:44 jason Exp $
 */

#ifndef __MISC_H__
#define __MISC_H__

#include <stdio.h>
#include "wave.h"

#define NO_CHILD_PID   -740
#define MAX_CHILD_ARGS  256

extern int num_child_args;
extern char *child_args[];
extern int clobberflag;

typedef enum {
  CHILD_INPUT,
  CHILD_OUTPUT
} child_types;

typedef enum {
  CLOSE_SUCCESS,
  CLOSE_CHILD_ERROR_INPUT,
  CLOSE_CHILD_ERROR_OUTPUT
} close_retcodes;

enum {
  CLOBBER_ACTION_ASK,
  CLOBBER_ACTION_ALWAYS,
  CLOBBER_ACTION_NEVER
} clobber_action_types;

typedef enum {
  CLOBBER_FILE_NO,
  CLOBBER_FILE_YES
} clobber_result_types;

#define spawn_input(a,b)       spawn(a,b,CHILD_INPUT,NULL)
#define spawn_output(a,b)      spawn(a,b,CHILD_OUTPUT,NULL)
#define spawn_input_fd(a,b,c)  spawn(a,b,CHILD_INPUT,c)
#define spawn_output_fd(a,b,c) spawn(a,b,CHILD_OUTPUT,c)

#define close_input(a,b)  close_and_wait(a,b,CHILD_INPUT)
#define close_output(a,b) close_and_wait(a,b,CHILD_OUTPUT)

#define close_input_stream(a)  close_and_wait(a->input,a->input_pid,CHILD_INPUT)
#define close_output_stream(a) close_and_wait(a->output,a->output_pid,CHILD_OUTPUT)

#define open_input(a) open_input_internal(a,NULL,NULL)

/* run an external program, and sets up two-way communication with that process */
int spawn(FILE **,FILE **,int,FILE *);

/* close a file descriptor, and wait for a child process if necessary */
int close_and_wait(FILE *,int,int);

/* determines whether a filename contains a '.' in its base file name */
int filename_contains_a_dot(char *);

/* a simple menu system to alter the order in which given input files will be processed */
void alter_file_order(wave_info **,int);

/* generic version printing function - don't use this if you write your own mode modules (unless you want me to take credit for it :) */
void internal_version(void);

/* function to convert a file's length to m:ss or m:ss.ff format */
void length_to_str(wave_info *);

/* replacement snprintf */
void my_snprintf(char *,int,char *,...);

/* copies an arbitrary-length tag, without the NULL byte */
void tagcpy(char *,char *);

/* compares what was received to the expected tag */
int tagcmp(char *,char *);

/* functions for building argument lists in format modules */
void arg_add(char *);
void arg_reset();

/* initialization function for modes that create output formats via -o format */
format_module *output_format_init(int,char **,int *);

/* function to determine if two filenames point to the same file */
int files_are_identical(char *,char *);

/* function to open an input file for reading and skip past the ID3v2 tag, if one exists */
FILE *open_input_internal(char *,int *,wlong *);

/* function to open an output file for writing */
FILE *open_output(char *);

/* function to open an input stream and skip past the ID3v2 tag, if one exists */
int open_input_stream(wave_info *);

/* function to check if a file name is about to be clobbered, and if so, asks whether this is OK */
int clobber_check(char *);

/* function to remove a file if it exists, and the format module is not "cust" or "null" */
void remove_file(format_module *,char *);

/* function to determine whether odd-sized data chunks are NULL-padded to an even length */
int odd_sized_data_chunk_is_null_padded(wave_info *);

/* function to check whether the file descriptor begins with an ID3v2 tag */
unsigned long check_for_id3v2_tag(FILE *);

#endif
