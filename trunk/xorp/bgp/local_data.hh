// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/bgp/local_data.hh,v 1.11 2005/11/15 11:43:58 mjh Exp $

#ifndef __BGP_LOCAL_DATA_HH__
#define __BGP_LOCAL_DATA_HH__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <list>

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
    LocalData() : _as(AsNum::AS_INVALID), _confed_id(AsNum::AS_INVALID)
    {}

    LocalData(const AsNum& as, const IPv4& id)
	: _as(as), _id(id), _confed_id(AsNum::AS_INVALID)
    {}

    /**
     * @return This routers AS number.
     */
    inline const AsNum& get_as() const {
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
    inline const IPv4& get_id() const {
	return _id;
    }

    /**
     * Set this routers ID.
     */
    void set_id(const IPv4& i) {
	_id = i;
    }

    /**
     * @return the confederation ID of this router if set.
     */
    inline const AsNum& get_confed_id() const {
	return _confed_id;
    }

    /**
     * Set this routers confederation ID.
     */
    inline void set_confed_id(const AsNum& confed_id) {
	_confed_id = confed_id;
    }

private:
    AsNum	_as;	                // This routers AS number.
    IPv4	_id;	                // This routers ID.
    AsNum	_confed_id;		// Confederation identifier.
};
#endif // __BGP_LOCAL_DATA_HH__
