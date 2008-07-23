// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/libxorp/popen.hh,v 1.9 2008/01/04 03:16:38 pavlin Exp $

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
