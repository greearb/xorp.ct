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

// $XORP: xorp/fea/xrl_rawsock4.hh,v 1.2 2002/12/14 23:42:51 hodson Exp $

#ifndef __FEA_XRL_RAWSOCK4_HH__
#define __FEA_XRL_RAWSOCK4_HH__

#include <map>
#include "libxorp/ref_ptr.hh"
#include "libxipc/xrl_router.hh"

#include "rawsock4.hh"

class InterfaceManager;
class XrlRouter;
class XrlRawSocketFilter;

/**
 * @short A class that manages raw sockets as used by the XORP Xrl Interface.
 *
 * The XrlRawSocket4Manager has two containers: a container for raw
 * sockets indexed by the protocol associated with the raw socket, and
 * a container for the filters associated with each xrl_target.  When
 * an Xrl Target registers for interest in a particular type of raw
 * packet a raw socket (FilterRawSocket4) is created if necessary,
 * then the relevent filter is created and associated with the
 * RawSocket.
 */

class XrlRawSocket4Manager
{
public:
    /**
     * Contructor for XrlRawSocket4Manager instances.
     *
     * May throw RawSocketException since a raw socket is contructed and
     * this requires root privelage.
     */
    XrlRawSocket4Manager(EventLoop& e, InterfaceManager& ifmgr, XrlRouter& xr)
	throw (RawSocketException);

    ~XrlRawSocket4Manager();

    XrlCmdError send(const string& vifname, const vector<uint8_t>& packet);

    XrlCmdError send(const IPv4&	    src_address,
		     const IPv4&	    dst_address,
		     const string&	    vifname,
		     const uint32_t&	    proto,
		     const uint32_t&	    ttl,
		     const uint32_t&	    tos,
		     const vector<uint8_t>& options,
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
    EventLoop&	      _e;
    InterfaceManager& _ifmgr;
    XrlRouter&	      _xrlrouter;

    FilterRawSocket4  _rs;

    // Collection of RawSocketFilters created by XrlRawSocketManager
    typedef multimap<string, XrlRawSocketFilter*> FilterBag;
    FilterBag _filters;

protected:
    void erase_filters(const FilterBag::iterator& begin,
		       const FilterBag::iterator& end);

};

#endif // __FEA_XRL_RAWSOCK4_HH__
