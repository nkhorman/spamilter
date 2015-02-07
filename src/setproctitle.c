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

#if (defined __linux)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if (defined __linux || defined __APPLE__)

#ifndef SPT_MAXTITLE
#define SPT_MAXTITLE 255
#endif

extern char **environ;

static struct
{
	// Original value.
	const char *arg0;

	// First enviroment variable.
	char* env0;

	// Title space available.
	char* base;
	char* end;

	// Pointer to original nul character within base.
	char *nul;

	// progname
	char *progname;
} SPT;


#ifndef SPT_MIN
#define SPT_MIN(a, b) (((a) < (b))? (a) : (b))   
#endif

static inline size_t spt_min(size_t a, size_t b)
{
	return SPT_MIN(a, b);
}

static char **spt_find_argv_from_env(int argc, char *arg0)
{
	int i;
	char **buf = NULL;
	char *ptr;
	char *limit;

	if (!(buf = (char **)malloc((argc + 1) * sizeof(char *))))
		return NULL;

	buf[argc] = NULL;

	// Walk back from environ until you find argc-1 null-terminated strings.
	// Don't look for argv[0] as it's probably not preceded by 0.
	ptr = SPT.env0;
	limit = ptr - 8192; // TODO: empiric limit: should use MAX_ARG
	--ptr;

	for (i = argc - 1; i >= 1; --i)
	{
		if (*ptr)
			return NULL; // leak the malloc above

		--ptr;
		while (*ptr && ptr > limit)
			--ptr;

		if (ptr <= limit)
			return NULL; // leak the malloc above

		buf[i] = (ptr + 1);
	}

	// The first arg has not a zero in front. But what we have is reliable
	// enough (modulo its encoding). Check if it is exactly what found.
	//
	// The check is known to fail on OS X with locale C if there are
	// non-ascii characters in the executable path.
	ptr -= strlen(arg0);

	if (ptr <= limit)
		return NULL; // leak the malloc above

	if (strcmp(ptr, arg0))
		return NULL; // leak the malloc above

	buf[0] = ptr;

	return buf;
}

void spt_init(int argc, char *arg0)
{
	// Store a pointer to the first enviroment variable since go
	// will overwrite enviroment.
	SPT.env0 = environ[0];

	{
		char **argv = spt_find_argv_from_env(argc, arg0);
		char **envp = &SPT.env0;
		char *base, *end, *nul, *tmp;
		int i;

		if (!(base = argv[0]))
			return;

		nul = &base[strlen(base)];
		end = nul + 1;

		for (i = 0; i < argc || (i >= argc && argv[i]); i++)
		{
			if (!argv[i] || argv[i] < end)
				continue;

			end = argv[i] + strlen(argv[i]) + 1;
		}

		for (i = 0; envp[i]; i++)
		{
			if (envp[i] < end)
				continue;

			end = envp[i] + strlen(envp[i]) + 1;
		}

		if (!(SPT.arg0 = strdup(argv[0])))
			return;

		SPT.progname = basename(strdup(argv[0]));

		memset(base, 0, end - base);

		SPT.nul  = nul;
		SPT.base = base;
		SPT.end  = end;
	}

	return;
}

void setproctitle(const char *fmt, ...)
{
	char buf[SPT_MAXTITLE + 1]; // Use buffer in case argv[0] is passed.
	va_list ap;
	char *nul;
	int len,maxlen;

	if (!SPT.base)
		return;

	if (fmt)
	{
		char *tmp = NULL;

		maxlen = SPT_MAXTITLE - (strlen(SPT.progname) * 2 + 5);
		va_start(ap, fmt);
		vasprintf(&tmp, fmt, ap);
		va_end(ap);

		// clip the title so it fits in the buffer, with the decorations
		tmp[spt_min(strlen(tmp), maxlen)] = '\0';

		// decorate the title so as to not let the consumer lie about the real applicaiton name
		len = snprintf(buf, sizeof buf, "%s: %s (%s)", SPT.progname, tmp, SPT.progname);
		free(tmp);
	}
	else
		len = snprintf(buf, sizeof buf, "%s", SPT.arg0);

	if (len <= 0)
		return;

	len = spt_min(len, spt_min(sizeof buf, SPT.end - SPT.base) - 1);
	memcpy(SPT.base, buf, len);
	nul = &SPT.base[len];

	if (nul < SPT.nul)
		*SPT.nul = '.';
	else if (nul == SPT.nul && &nul[1] < SPT.end)
	{
		*SPT.nul = ' ';
		*++nul = '\0';
	}
}
#endif

#ifdef _UNIT_TEST
int main(int argc, char **argv)
{
	spt_init(argc,argv[0]);

	setproctitle("This is a test");

	system("ps ax | grep test | grep -v grep");

	return 0;
}
#endif
