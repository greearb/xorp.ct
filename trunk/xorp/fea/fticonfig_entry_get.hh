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

// $XORP: xorp/fea/fticonfig_entry_get.hh,v 1.10 2004/03/16 21:45:19 pavlin Exp $

#ifndef __FEA_FTICONFIG_ENTRY_GET_HH__
#define __FEA_FTICONFIG_ENTRY_GET_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "fte.hh"
#include "netlink_socket.hh"
#include "routing_socket.hh"


class IPv4;
class IPv6;
class FtiConfig;

class FtiConfigEntryGet {
public:
    FtiConfigEntryGet(FtiConfig& ftic);
    
    virtual ~FtiConfigEntryGet();
    
    FtiConfig&	ftic() { return _ftic; }
    
    virtual void register_ftic();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start() = 0;
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop() = 0;
    
    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return bool on success, otherwise false.
     */
    virtual bool lookup_route4(const IPv4& dst, Fte4& fte) = 0;

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry4(const IPv4Net& dst, Fte4& fte) = 0;

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route6(const IPv6& dst, Fte6& fte) = 0;

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry6(const IPv6Net& dst, Fte6& fte) = 0;

    /**
     * Parse information about routing entry information received from
     * the underlying system.
     * 
     * The information to parse is in RTM format
     * (e.g., obtained by routing sockets or by sysctl(3) mechanism).
     * 
     * @param fte the Fte storage to store the parsed information.
     * @param buf the buffer with the data to parse.
     * @param buf_bytes buf_bytes the size of the data in the buffer.
     * @param is_rtm_get_only if true, consider only the RTM_GET entries.
     * @return true on success, otherwise false.
     * @see FteX.
     */
    bool parse_buffer_rtm(FteX& fte, const uint8_t *buf, size_t buf_bytes,
			  bool is_rtm_get_only);

    /**
     * Parse information about routing entry information received from
     * the underlying system.
     * 
     * The information to parse is in NETLINK format
     * (e.g., obtained by netlink(7) sockets mechanism).
     * 
     * @param fte the Fte storage to store the parsed information.
     * @param buf the buffer with the data to parse.
     * @param buf_bytes buf_bytes the size of the data in the buffer.
     * @param is_nlm_get_only if true, consider only the entries obtained
     * by RTM_GETROUTE.
     * @return true on success, otherwise false.
     * @see FteX.
     */
    bool parse_buffer_nlm(FteX& fte, const uint8_t *buf, size_t buf_bytes,
			  bool is_nlm_get_only);

protected:
    int sock(int family);
    
    int	_s4;
    int _s6;
    
private:
    FtiConfig&	_ftic;
};

class FtiConfigEntryGetDummy : public FtiConfigEntryGet {
public:
    FtiConfigEntryGetDummy(FtiConfig& ftic);
    virtual ~FtiConfigEntryGetDummy();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry4(const IPv4Net& dst, Fte4& fte);

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry6(const IPv6Net& dst, Fte6& fte);

private:
};

class FtiConfigEntryGetRtsock : public FtiConfigEntryGet,
				public RoutingSocket {
public:
    FtiConfigEntryGetRtsock(FtiConfig& ftic);
    virtual ~FtiConfigEntryGetRtsock();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry4(const IPv4Net& dst, Fte4& fte);

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry6(const IPv6Net& dst, Fte6& fte);

private:
    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route(const IPvX& dst, FteX& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry(const IPvXNet& dst, FteX& fte);

    RoutingSocketReader _rs_reader;
};

class FtiConfigEntryGetNetlink : public FtiConfigEntryGet,
				 public NetlinkSocket4,
				 public NetlinkSocket6 {
public:
    FtiConfigEntryGetNetlink(FtiConfig& ftic);
    virtual ~FtiConfigEntryGetNetlink();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry4(const IPv4Net& dst, Fte4& fte);

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry6(const IPv6Net& dst, Fte6& fte);

private:
    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route(const IPvX& dst, FteX& fte);

    NetlinkSocketReader	_ns_reader;
};

#endif // __FEA_FTICONFIG_ENTRY_GET_HH__
