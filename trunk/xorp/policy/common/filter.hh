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

#ifndef __POLICY_COMMON_FILTER_HH__
#define __POLICY_COMMON_FILTER_HH__

#include <string>

namespace filter {



/**
 * There are three type of filters:
 *
 * IMPORT: deals with import filtering. Incoming routes from other routers and
 * possibly the rib.
 *
 * EXPORT_SOURCE_MATCH: a filter which tags routes that need to be
 * redistributed. This filter only modifies policytags.
 *
 * EXPORT: Filters outgoing routes from the routing protocols to other routers
 * and possibly the rib itself.
 */
enum Filter {
    IMPORT =		    1,
    EXPORT_SOURCEMATCH =    2,
    EXPORT =		    4
};


/**
 * @param f filter type to convert to human readable string.
 * @return string representation of filter name.
 */
std::string filter2str(const Filter& f);

} // namespace


#endif // __POLICY_COMMON_FILTER_HH__
