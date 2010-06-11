// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/policy/common/policy_exception.hh,v 1.8 2008/10/02 21:58:07 bms Exp $

#ifndef __POLICY_COMMON_POLICY_EXCEPTION_HH__
#define __POLICY_COMMON_POLICY_EXCEPTION_HH__


#include "libxorp/exceptions.hh"


/**
 * @short Base class for all policy exceptions.
 *
 * All policy exceptions have a string representing the error.
 */
/**
 * @short Base class for all policy exceptions.
 *
 * All policy exceptions have a string representing the error.
 */
class PolicyException : public XorpReasonedException {
public:
    /**
     * @param reason the error message
     */
    PolicyException(const char* file, size_t line, 
			const string& init_why = "")   
      : XorpReasonedException("PolicyException", file, line, init_why) {} 

    PolicyException(const char* type, const char* file, size_t line, 
			const string& init_why = "")   
      : XorpReasonedException(type, file, line, init_why) {} 
    virtual ~PolicyException() {}
};


#endif // __POLICY_COMMON_POLICY_EXCEPTION_HH__
