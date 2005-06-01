/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2005 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/mrt/random.h,v 1.1.1.1 2002/12/11 23:56:07 hodson Exp $
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
extern char *		my_initstate(unsigned long seed, char *state,
				     size_t n);
extern char *		my_setstate(char *state);
__END_DECLS

#endif /* __MRT_RANDOM_H__ */
