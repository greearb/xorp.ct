// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ifndef __FEA_XRL_RAWSOCK6_HH__
#define __FEA_XRL_RAWSOCK6_HH__

#include <map>
#include "libxorp/ref_ptr.hh"
#include "libxipc/xrl_router.hh"

#include "rawsock6.hh"

class InterfaceManager;
class XrlRouter;
class XrlRawSocket6Filter;

/**
 * @short A class that manages raw sockets as used by the XORP Xrl Interface.
 *
 * The XrlRawSocket6Manager has two containers: a container for raw
 * sockets indexed by the protocol associated with the raw socket, and
 * a container for the filters associated with each xrl_target.  When
 * an Xrl Target registers for interest in a particular type of raw
 * packet a raw socket (FilterRawSocket6) is created if necessary,
 * then the relevent filter is created and associated with the
 * RawSocket.
 */

class XrlRawSocket6Manager
{
public:
    /**
     * Contructor for XrlRawSocket6Manager instances.
     *
     * May throw RawSocket6Exception since a raw socket is contructed and
     * this requires root privelage.
     */
    XrlRawSocket6Manager(EventLoop& eventloop, InterfaceManager& ifmgr,
			 XrlRouter& xr)
	throw (RawSocket6Exception);

    ~XrlRawSocket6Manager();

    XrlCmdError send(const IPv6& src,
		     const IPv6& dst,
		     const string& vifname,
		     uint8_t proto,
		     uint8_t tclass,
		     uint8_t hoplimit,
		     const vector<uint8_t>& hopopts,
		     const vector<uint8_t>& payload);

    XrlCmdError register_vif_receiver(const string&		tgt,
				      const string&		ifname,
				      const string&		vifname,
				      const uint32_t& 		proto);

    XrlCmdError unregister_vif_receiver(const string&		tgt,
					const string&		ifname,
					const string&		vifname,
					const uint32_t& 	proto);

    XrlRouter&	      router() { return _xrlrouter; }
    InterfaceManager& ifmgr()  { return _ifmgr; }

    /** Method to be called by Xrl sending filter invoker */
    void xrl_vif_send_handler(const XrlError& e, string tgt_name);

protected:
    EventLoop&	      _eventloop;
    InterfaceManager& _ifmgr;
    XrlRouter&	      _xrlrouter;

    // Collection of IPv6 raw sockets keyed by protocol.
    typedef map<uint8_t, FilterRawSocket6*> SocketTable6;
    SocketTable6 _sockets;

    // Collection of RawSocketFilters created by XrlRawSocketManager
    typedef multimap<string, XrlRawSocket6Filter*> FilterBag6;
    FilterBag6 _filters;

protected:
    void erase_filters(const FilterBag6::iterator& begin,
		       const FilterBag6::iterator& end);

};

#endif // __FEA_XRL_RAWSOCK6_HH__
