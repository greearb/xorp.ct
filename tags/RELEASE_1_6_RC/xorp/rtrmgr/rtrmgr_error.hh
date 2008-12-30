// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/rtrmgr/rtrmgr_error.hh,v 1.8 2008/07/23 05:11:43 pavlin Exp $

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
