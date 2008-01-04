/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 International Computer Science Institute
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
 * $XORP: xorp/libxorp/gai_strerror.c,v 1.6 2007/02/16 22:46:19 pavlin Exp $
 */

#include "xorp.h"

#ifdef HOST_OS_WINDOWS

#include "win_io.h"

/*
 * gai_strerror() is actually implemented as an inline on Windows; it is
 * not present in the Winsock2 DLLs. The 'real' inline functions are missing
 * from the MinGW copy of the PSDK, presumably for licensing reasons.
 *
 * There is an accompanying kludge to purge the namespace for this function
 * to be visible in <libxorp/xorp_osdep_end.h>.
 *
 * This now simply calls the win_strerror() utility routine in win_io.c.
 */

char *
gai_strerror(int ecode)
{
     return (win_strerror(ecode));
}

#endif /* HOST_OS_WINDOWS */
