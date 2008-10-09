// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/devnotes/template.hh,v 1.10 2008/07/23 05:09:59 pavlin Exp $

#ifndef __VRRP_VRRP_EXCEPTION_HH__
#define __VRRP_VRRP_EXCEPTION_HH__

#include "libxorp/exceptions.hh"

class VRRPException : public XorpReasonedException {
public:
    VRRPException(const char* file, size_t line, const string& why = "")
	: XorpReasonedException("VRRPException", file, line, why) {}
};

#endif // __VRRP_VRRP_EXCEPTION_HH__
