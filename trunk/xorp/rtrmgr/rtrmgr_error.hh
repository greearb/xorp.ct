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

// $XORP: xorp/rtrmgr/rtrmgr_error.hh,v 1.1 2004/01/13 00:17:09 pavlin Exp $

#ifndef __RTRMGR_RTRMGR_ERROR_HH__
#define __RTRMGR_RTRMGR_ERROR_HH__


#include "libxorp/exceptions.hh"


class InitError : public XorpReasonedException {
public:
    InitError(const char* file, size_t line, const string init_why = "")
	: XorpReasonedException("InitError", file, line, init_why) {}
};

class ParseError : public XorpReasonedException {
public:
    ParseError(const char* file, size_t line, const string& reason)
	: XorpReasonedException("ParseError", file, line, reason)
    {}
};

#endif // __RTRMGR_RTRMGR_ERROR_HH__
