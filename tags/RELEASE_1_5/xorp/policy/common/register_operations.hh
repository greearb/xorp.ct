// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/register_operations.hh,v 1.6 2008/01/04 03:17:20 pavlin Exp $

#ifndef __POLICY_COMMON_REGISTER_OPERATIONS_HH__
#define __POLICY_COMMON_REGISTER_OPERATIONS_HH__

#include "element.hh"

/**
 * @short Do initial registration of dispatcher callbacks.
 *
 * The sole purpose of this class is to register the callbacks before the
 * dispatcher is actually used, and register them only once at startup.
 */
class RegisterOperations {
public:
    /**
     * Constructor which performs registrations
     *
     * In essence, this is where the grammar lives.
     */
    RegisterOperations();
};

namespace operations {

/**
 * Maybe be used to construct elements.  Also for casting! 
 *
 * @param type the string representation of typename.
 * @param arg the string representation of value.
 * @return element of wanted type representing arg.
 */
Element* ctr(const ElemStr& type, const Element& arg);

}

#endif // __POLICY_COMMON_REGISTER_OPERATIONS_HH__
