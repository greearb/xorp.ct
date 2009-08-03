// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxorp/popen.hh,v 1.11 2008/10/02 21:57:32 bms Exp $

#ifndef __LIBXORP_POPEN_HH__
#define __LIBXORP_POPEN_HH__

#include<list>

pid_t	popen2(const string& command, const list<string>& arguments,
	       FILE *& outstream, FILE *& errstream,
	       bool redirect_stderr_to_stdout);
int	pclose2(FILE *iop_out, bool dont_wait);
int	popen2_mark_as_closed(pid_t pid, int wait_status);

#ifdef HOST_OS_WINDOWS
HANDLE	pgethandle(pid_t pid);
#endif

#endif // __LIBXORP_POPEN_HH__
