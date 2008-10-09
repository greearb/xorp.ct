// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/fea/fea_exception.hh,v 1.2 2008/10/09 18:03:48 abittau Exp $

#ifndef __FEA_FEA_EXCEPTION_HH__
#define __FEA_FEA_EXCEPTION_HH__

#include "libxorp/exceptions.hh"

class FeaException : public XorpReasonedException {
public:
    FeaException(const char* file, size_t line, const string& why = "")
        : XorpReasonedException("FeaException", file, line, why) {}
};

#endif // __FEA_FEA_EXCEPTION_HH__
