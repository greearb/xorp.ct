/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ident "$XORP: xorp/libxorp/utility.c,v 1.12 2008/10/01 21:49:32 pavlin Exp $"


/*
 * Misc. utilities.
 */

#include "libxorp_module.h"

#include "libxorp/xorp.h"

#include <ctype.h>

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

/*
 * Function to return C-string representation of a boolean: "true" of "false".
 */
const char *
bool_c_str(int v)
{
    if (v)
	return ("true");
    else
	return ("false");
}
