/*  core_fileio.c - file I/O functions
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
 * $Id: core_fileio.c,v 1.26 2004/04/03 20:03:05 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include "shntool.h"
#include "fileio.h"
#include "misc.h"

int read_n_bytes_internal(FILE *in,unsigned char *buf,int num,int verbose)
/* reads the specified number of bytes from the file descriptor 'in' into buf */
{
  int read;

  if ((read = fread(buf,1,num,in)) != num) {
    if (verbose) {
      fprintf(stderr,"\n");
      st_warning("tried to read %d bytes, but only read %d - possible truncated/corrupt file",num,read);
    }
  }

  return read;
}

int write_n_bytes_internal(FILE *out,unsigned char *buf,int num,int verbose)
/* writes the specified number of bytes from buf into the file descriptor 'out' */
{
  int wrote;

  if ((wrote = fwrite(buf,1,num,out)) != num) {
    if (verbose) {
      fprintf(stderr,"\n");
      st_warning("tried to write %d bytes, but only wrote %d - make sure that:\n"
                 "+ there is enough disk space\n"
                 "+ the specified output directory exists\n"
                 "+ you have permission to create files in the output directory\n"
                 "+ the output format's encoder program is installed correctly",num,wrote);
    }
  }

  return wrote;
}

unsigned long transfer_n_bytes_internal(FILE *in,FILE *out,unsigned long bytes,int verbose)
/* transfers 'bytes' bytes from file descriptor 'in' to file descriptor 'out' */
{
  unsigned char buf[XFER_SIZE];
  int bytes_to_xfer,
      actual_bytes_read,
      actual_bytes_written;
  unsigned long total_bytes_to_xfer = bytes,
                total_bytes_xfered = 0;

  while (total_bytes_to_xfer > 0) {
    bytes_to_xfer = min(total_bytes_to_xfer,XFER_SIZE);
    actual_bytes_read = read_n_bytes_internal(in,buf,bytes_to_xfer,verbose);
    actual_bytes_written = write_n_bytes_internal(out,buf,actual_bytes_read,verbose);
    total_bytes_xfered += (unsigned long)actual_bytes_written;
    if (actual_bytes_read != bytes_to_xfer || actual_bytes_written != bytes_to_xfer)
      break;
    total_bytes_to_xfer -= bytes_to_xfer;
  }

  return total_bytes_xfered;
}

int write_padding(FILE *out,int bytes)
/* writes the specified number of zero bytes to the file descriptor given */
{
  unsigned char silence[CD_BLOCK_SIZE];

  if (bytes >= CD_BLOCK_SIZE) {
    /* padding should always be less than the size of the block being padded */
    return 0;
  }

  memset((void *)silence,0,CD_BLOCK_SIZE);

  return write_n_bytes_internal(out,silence,bytes,1);
}

int read_value_long(FILE *file,unsigned long *be_val,unsigned long *le_val,unsigned char *tag_val)
/* reads an unsigned long in big- and/or little-endian format from a file descriptor */
{
  unsigned char buf[5];

  if (fread(buf, 1, 4, file) != 4)
    return 0;

  buf[4] = 0;

  if (be_val)
    *be_val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

  if (le_val)
    *le_val = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

  if (tag_val)
    tagcpy(tag_val,buf);

  return 1;
}

int read_value_short(FILE *file,unsigned short *be_val,unsigned short *le_val)
/* reads an unsigned short in big- and/or little-endian format from a file descriptor */
{
  unsigned char buf[2];

  if (fread(buf, 1, 2, file) != 2)
    return 0;

  if (be_val)
    *be_val = (buf[0] << 8) | buf[1];

  if (le_val)
    *le_val = (buf[1] << 8) | buf[0];

  return 1;
}
