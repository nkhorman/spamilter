/*--------------------------------------------------------------------*
 *
 * Modified by;
 *	Neal Horman - http://www.wanlink.com
 *	portions Copyright (c) 2015 Neal Horman. All Rights Reserved
 *
 * From;
 *	https://github.com/ErikDubbelboer/gspt
 *
 *--------------------------------------------------------------------*/

/* ==========================================================================
 * setproctitle.h - Linux/Darwin setproctitle.
 * --------------------------------------------------------------------------
 * Copyright (C) 2010  William Ahern
 * Copyright (C) 2013  Salvatore Sanfilippo
 * Copyright (C) 2013  Stam He
 * Copyright (C) 2013  Erik Dubbelboer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ==========================================================================
 */

#if (defined __linux || defined __APPLE__)

#if (defined __linux)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdarg.h> // va_list va_start va_end 
#include <stdlib.h> // malloc(3) setenv(3) clearenv(3) setproctitle(3) getprogname(3)
#include <stdio.h>  // vsnprintf(3) snprintf(3)

#define HAVE_SETPROCTITLE_REPLACEMENT 1

int spt_init(int argc, char *arg0);
void setproctitle(const char *fmt, ...);

#endif
