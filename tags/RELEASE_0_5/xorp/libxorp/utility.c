/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/* Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/utility.c,v 1.1 2003/11/06 01:21:38 pavlin Exp $"


/*
 * Misc. utilities.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "libxorp_module.h"
#include "config.h"
#include "utility.h"

/*
 * Exported variables
 */

/*
 * Local constants definitions
 */

/*
 * Local structures, typedefs and macros
 */

/*
 * Local variables
 */

/*
 * Local functions prototypes
 */


/*
 * Various ctype(3) wrappers that work properly even if the value of the int
 * argument is not representable as an unsigned char and doesn't have the
 * value of EOF.
 */
int
xorp_isalnum(int c)
{
    return isascii(c) && isalnum(c);
}

int
xorp_isalpha(int c)
{
    return isascii(c) && isalpha(c);
}

/*
 * TODO: for now comment-out xorp_isblank(), because isblank(3) is introduced
 * with ISO C99, and may not always be available on the system.
 */
#if 0
int
xorp_isblank(int c)
{
    return isascii(c) && isblank(c);
}
#endif /* 0 */

int
xorp_iscntrl(int c)
{
    return isascii(c) && iscntrl(c);
}

int
xorp_isdigit(int c)
{
    return isascii(c) && isdigit(c);
}

int
xorp_isgraph(int c)
{
    return isascii(c) && isgraph(c);
}

int
xorp_islower(int c)
{
    return isascii(c) && islower(c);
}

int
xorp_isprint(int c)
{
    return isascii(c) && isprint(c);
}

int
xorp_ispunct(int c)
{
    return isascii(c) && ispunct(c);
}

int
xorp_isspace(int c)
{
    return isascii(c) && isspace(c);
}

int
xorp_isupper(int c)
{
    return isascii(c) && isupper(c);
}

int
xorp_isxdigit(int c)
{
    return isascii(c) && isxdigit(c);
}

int
xorp_tolower(int c)
{
    if (isascii(c))
	return tolower(c);
    else
	return c;
}

int
xorp_toupper(int c)
{
    if (isascii(c))
	return toupper(c);
    else
	return c;
}
