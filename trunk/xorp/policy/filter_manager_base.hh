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

#ifndef __POLICY_FILTER_MANAGER_BASE_HH__
#define __POLICY_FILTER_MANAGER_BASE_HH__

#include "code.hh"

/**
 * @short Base class for a FilterManager.
 *
 * A filter manager is the entitiy which interacts with the actual policy
 * filters in protocols and the rib.
 *
 * It is used by the configuration class to trigger updates
 *
 */
class FilterManagerBase {
public:
    virtual ~FilterManagerBase() {}

    /**
     * Update a specific policy filter.
     *
     * @param t The target to update [protocol/filter pair].
     *
     */
    virtual void update_filter(const Code::Target& t) = 0;

    /**
     * Commit all updates after msec milliseconds.
     *
     * @param msec Milliseconds after which all updates should be commited.
     *
     */
    virtual void flush_updates(uint32_t msec) = 0;
};

#endif // __POLICY_FILTER_MANAGER_BASE_HH__
