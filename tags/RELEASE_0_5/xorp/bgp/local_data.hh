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

// $XORP: xorp/bgp/local_data.hh,v 1.5 2003/03/10 23:19:59 hodson Exp $

#ifndef __BGP_LOCAL_DATA_HH__
#define __BGP_LOCAL_DATA_HH__

#include "config.h"
#include "bgp_module.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/asnum.hh"

/**
 * Data that applies to all BGP peerings.
 * Currently this is just our AS number and router ID.
 */
class LocalData {
public:
    LocalData() : _as(AsNum::AS_INVALID)
    {}

    LocalData(const AsNum& as, const IPv4& id)
	: _as(as), _id(id)
    {}

    /**
     * @return This routers AS number.
     */
    inline const AsNum& as() const {
	return _as;
    }

    /**
     * Set this routers AS number.
     */
    inline void set_as(const AsNum& a) {
	_as = a;
    }

    /**
     * @return This routers ID.
     */
    inline const IPv4& id() const {
	return _id;
    }

    /**
     * Set this routers ID.
     */
    void set_id(const IPv4& i) {
	_id = i;
    }

private:
    AsNum	_as;	// This routers AS number.
    IPv4	_id;	// This routers ID.
};
#endif // __BGP_LOCAL_DATA_HH__
