// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_COMMON_POLICY_EXCEPTION_HH__
#define __POLICY_COMMON_POLICY_EXCEPTION_HH__

#include <string>


/**
 * @short Base class for all policy exceptions.
 *
 * All policy exceptions have a string representing the error.
 */
class PolicyException {
public:
    /**
     * @param reason the error message
     */
    PolicyException(const std::string& reason) : _reason(reason) {}
    virtual ~PolicyException() {}

    /**
     * @return a human readable and possibly understandable error message
     */
     const std::string& str() const { return _reason; }

private:
    std::string _reason;
};


#endif // __POLICY_COMMON_POLICY_EXCEPTION_HH__
