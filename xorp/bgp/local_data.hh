// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/bgp/local_data.hh,v 1.25 2008/10/02 21:56:16 bms Exp $

#ifndef __BGP_LOCAL_DATA_HH__
#define __BGP_LOCAL_DATA_HH__

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/asnum.hh"



#include "damping.hh"


/**
 * Data that applies to all BGP peerings.
 * Currently this is just our AS number and router ID.
 */
class LocalData {
public:
    LocalData(EventLoop& eventloop) : _as(AsNum::AS_INVALID), 
				      _use_4byte_asnums(false),
				      _confed_id(AsNum::AS_INVALID),
				      _route_reflector(false),
				      _damping(eventloop),
				      _jitter(true)
    {}

//     LocalData(const AsNum& as, const IPv4& id)
// 	: _as(as), _id(id), _confed_id(AsNum::AS_INVALID)
//     {}

    /**
     * @return This routers AS number.
     */
    const AsNum& get_as() const { return _as; }

    /**
     * Set this routers AS number.
     */
    void set_as(const AsNum& a) { _as = a; }

    /**
     * @return true if we use 4 byte AS numbers.
     */
    inline bool use_4byte_asnums() const {
	return _use_4byte_asnums;
    }


    /**
     * Set whether to send 2 or 4 byte AS numbers 
     */
    inline void set_use_4byte_asnums(bool use_4byte_asnums) {
	_use_4byte_asnums = use_4byte_asnums;
    }


    /**
     * @return This routers ID.
     */
    const IPv4& get_id() const { return _id; }

    /**
     * Set this routers ID.
     */
    void set_id(const IPv4& i) {
	_id = i;
    }

    /**
     * @return the confederation ID of this router if set.
     */
    const AsNum& get_confed_id() const { return _confed_id; }

    /**
     * Set this routers confederation ID.
     */
    void set_confed_id(const AsNum& confed_id) { _confed_id = confed_id; }

    /**
     * @return the cluster ID of this router.
     */
    const IPv4& get_cluster_id() const {
	XLOG_ASSERT(_route_reflector);
	return _cluster_id;
    }

    /**
     * Set this routers cluster ID.
     */
    void set_cluster_id(const IPv4& cluster_id) { _cluster_id = cluster_id; }

    /**
     * Get the route reflection status.
     */
    const bool& get_route_reflector() const { return _route_reflector; }

    /**
     * Set the route reflection status.
     */
    void set_route_reflector(const bool route_reflector) {
	_route_reflector = route_reflector;
    }

    /**
     * Return all the route flap damping state.
     */
    Damping& get_damping() {
	return _damping;
    }

    void set_jitter(bool jitter) {
	_jitter = jitter;
    }

    bool get_jitter() const {
	return _jitter;
    }

private:
    AsNum	_as;	                // This routers AS number.
    bool        _use_4byte_asnums;      // Indicates to use 4byte AS numbers.
    IPv4	_id;	                // This routers ID.
    AsNum	_confed_id;		// Confederation identifier.
    IPv4	_cluster_id;		// Router reflector cluster ID
    bool	_route_reflector;	// True if this router is a
					// route reflector
    Damping 	_damping;		// Route Flap Damping parameters
    bool	_jitter;		// Jitter applied to timers.
};
#endif // __BGP_LOCAL_DATA_HH__
