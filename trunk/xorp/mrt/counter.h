/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2003 International Computer Science Institute
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
 * $XORP: xorp/mrt/counter.h,v 1.1.1.1 2002/12/11 23:56:07 hodson Exp $
 */

#ifndef __MRT_COUNTER_H__
#define __MRT_COUNTER_H__


/*
 * A header file with various macros and definitions that implement
 * "counters" (a count-down, timer-like facility).
 */


#include <sys/types.h>


/*
 * Constants definitions
 */

/*
 * Structures, typedefs and macros
 */
#ifndef MAX
#define MAX(a, b) (((a) >= (b))? (a) : (b))
#define MIN(a, b) (((a) <= (b))? (a) : (b))
#endif /* MAX & MIN */

#ifndef MAX_VAL_SET
#define MAX_VAL_SET(a, val)						\
do {									\
	if ((a) < (val))						\
		(a) = (val);						\
} while (0)
#define MIN_VAL_SET(a, val)						\
do {									\
	if ((a) > (val))						\
		(a) = (val);						\
} while (0)
#endif /* MAX_VAL_SET & MIN_VAL_SET */

/*
 * XXX: max. counter value limited to 65535. Re-typedef for your own needs
 */
typedef uint16_t counter_t;
/*
 * The user code should use only those macros below to deal
 * with counters:
 *	COUNTER_RESET((counter_t)counter)
 *	COUNTER_ISSET((counter_t)counter)
 *	COUNTER_IS_SMALLER((counter_t)counter, int value)
 *	COUNTER_SET((counter_t)counter, int value)
 *	COUNTER_DECR((counter_t)counter, int value)
 */
#define COUNTER_FOREVER ((counter_t)~0)		/* Never decrement it! */
#define COUNTER_MAX	(COUNTER_FOREVER - 1)	/* Max. value of a counter */

#define COUNTER_RESET(counter)						\
do {									\
	(counter) = (counter_t)0;					\
} while (0)
#define COUNTER_ISSET(counter) ((counter) != (counter_t)0)

#define COUNTER_IS_SMALLER(counter, value) ((counter) < (counter_t)(value))

#define COUNTER_SET(counter, value)					\
do {									\
	(counter) = (counter_t)(value);					\
} while (0)

#define COUNTER_DECR(counter, value)					\
do {									\
	if ((counter) != COUNTER_FOREVER) {				\
		(counter) -= MIN((counter), (counter_t)(value));	\
	}								\
} while (0)

/* A macro to allocate (and zeroes) an array of size 's' elements */
#define COUNTER_ARRAY_ALLOC(p, s)					\
do {									\
	(p) = (counter_t *)malloc((s)*sizeof(*p));			\
	memset((p), 0, (s)*sizeof(*p));					\
} while (0)
#define COUNTER_ARRAY_FREE(p) (free((p)))

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

__END_DECLS

#endif /* __MRT_COUNTER_H__ */
