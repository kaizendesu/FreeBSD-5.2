/*-
 * Copyright (c) 2003 Tim J. Robbins. All rights reserved.
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* f*rune() are obsolete in FreeBSD 6 -- use ANSI functions instead. */
#define	OBSOLETE_IN_6

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)frune.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */
#include <sys/param.h>
__FBSDID("$FreeBSD$");

#include <limits.h>
#include <rune.h>
#include <stddef.h>
#include <stdio.h>
#include <wchar.h>

__warn_references(fgetrune, "warning: fgetrune() is deprecated. See fgetrune(3).");
long
fgetrune(fp)
	FILE *fp;
{
	wint_t ch;

	if ((ch = fgetwc(fp)) == WEOF)
		return (feof(fp) ? EOF : _INVALID_RUNE);
	return ((long)ch);
}

__warn_references(fungetrune, "warning: fungetrune() is deprecated. See fungetrune(3).");
int
fungetrune(r, fp)
	rune_t r;
	FILE* fp;
{

	return (ungetwc((wint_t)r, fp) == WEOF ? EOF : 0);
}

__warn_references(fputrune, "warning: fputrune() is deprecated. See fputrune(3).");
int
fputrune(r, fp)
	rune_t r;
	FILE *fp;
{

	return (fputwc((wchar_t)r, fp) == WEOF ? EOF : 0);
}
