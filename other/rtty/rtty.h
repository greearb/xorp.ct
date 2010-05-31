/* rtty.h - definitions for rtty package
 * vix 01nov91 [written]
 *
 * $Id$
 */

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#define ASSERT2(e, m1, m2)	if (!(e)) {int save_errno=errno;\
					   fprintf(stderr, "%s: %s: ",\
					           ProgName, m2);\
					   errno=save_errno;\
					   perror(m1); exit(1);}
#define ASSERT(e, m)		ASSERT2(e, m, "")
#define USAGE(x)		{ fprintf x;\
				  fprintf(stderr, "usage:  %s %s\n",\
					  ProgName, USAGE_STR);\
				  exit(1); }
#define TRUE 1
#define FALSE 0
#define min(a,b) ((a>b)?b:a)
#define max(a,b) ((a>b)?a:b)

#ifndef dprintf
# ifdef DEBUG
#  define dprintf if (Debug) fprintf
# else
#  define dprintf (void)
# endif
#endif

/* something in ULTRIX that we want to use if it's there */
#ifndef TAUTOFLOW
#define TAUTOFLOW 0
#endif
