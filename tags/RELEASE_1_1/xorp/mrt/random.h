/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2001
 * YOID Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $XORP: xorp/mrt/random.h,v 1.9 2002/12/09 11:49:08 pavlin Exp $
 */


#ifndef __MRT_RANDOM_H__
#define __MRT_RANDOM_H__


/*
 * Random-number generator front-end.
 */


#include <stdlib.h>
#include <sys/types.h>


/*
 * Constants definitions
 */
#if (!defined(TRUE)) || (!defined(FALSE))
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define FALSE (0)
#define TRUE (!FALSE)
#ifndef __cplusplus
typedef enum { true = TRUE, false = FALSE } bool;
#endif
typedef bool bool_t;
#endif /* TRUE, FALSE */

/*
 * Structures, typedefs and macros
 */
/*
 * The 'random' API. Note the similarity with random(3), except for RANDOM().
 */
#define SRANDOM(seed)			my_srandom(seed)
#define RANDOM(max_value)		my_random(max_value)
#define INITSTATE(seed, state, n)	my_initstate(seed, state, n)
#define SETSTATE(state)			my_setstate(state)

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS
extern void		my_srandom(unsigned long seed);
extern unsigned long	my_random(unsigned long max_value);
extern char *		my_initstate(unsigned long seed, char *state, long n);
extern char *		my_setstate(char *state);
__END_DECLS

#endif /* __MRT_RANDOM_H__ */
