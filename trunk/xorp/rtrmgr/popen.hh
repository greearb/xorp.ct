// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/popen.hh,v 1.2 2003/03/10 23:21:00 hodson Exp $

#ifndef __RTRMGR_POPEN_HH__
#define __RTRMGR_POPEN_HH__

void popen2(const string& command, FILE *& outstream, FILE *& errstream);
int pclose2(FILE *iop_out);

#endif // __RTRMGR_POPEN_HH__
