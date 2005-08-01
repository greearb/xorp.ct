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
 * $XORP$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xorp.h"

#ifdef HOST_OS_WINDOWS

/*
 * gai_strerror() is actually implemented as an inline on Windows; it is
 * not present in the Winsock2 DLLs. The 'real' inline functions are missing
 * from the MinGW copy of the PSDK, presumably for licensing reasons.
 *
 * There is an accompanying kludge to purge the namespace for this function
 * to be visible in <libxorp/xorp_osdep_end.h>.
 */

#define	GAI_STRERROR_BUFFER_SIZE	1024

char *
gai_strerror(int ecode)
{
    static char buff[GAI_STRERROR_BUFFER_SIZE + 1];
    DWORD dwMsgLen;

    dwMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|
			      FORMAT_MESSAGE_IGNORE_INSERTS|
			      FORMAT_MESSAGE_MAX_WIDTH_MASK,
			      NULL,
			      ecode,
			      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			      (LPSTR)buff,
			      GAI_STRERROR_BUFFER_SIZE,
			      NULL);
     return (buff);
}

#endif /* HOST_OS_WINDOWS */
