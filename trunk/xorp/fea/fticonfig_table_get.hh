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

// $XORP: xorp/fea/fticonfig_table_get.hh,v 1.12 2004/08/17 02:20:07 pavlin Exp $

#ifndef __FEA_FTICONFIG_TABLE_GET_HH__
#define __FEA_FTICONFIG_TABLE_GET_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "fte.hh"


class IPv4;
class IPv6;
class FtiConfig;

class FtiConfigTableGet {
public:
    FtiConfigTableGet(FtiConfig& ftic);
    
    virtual ~FtiConfigTableGet();
    
    FtiConfig&	ftic() { return _ftic; }
    
    virtual void register_ftic_primary();
    virtual void register_ftic_secondary();
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }

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
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table4(list<Fte4>& fte_list) = 0;

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table6(list<Fte6>& fte_list) = 0;

    /**
     * Parse information about routing table information received from
     * the underlying system.
     * 
     * The information to parse is in RTM format
     * (e.g., obtained by routing sockets or by sysctl(3) mechanism).
     * 
     * @param family the address family to consider only ((e.g., AF_INET
     * or AF_INET6 for IPv4 and IPv6 respectively).
     * @param fte_list the list with the Fte entries to store the result.
     * @param buf the buffer with the data to parse.
     * @param buf_bytes the size of the data in the buffer.
     * @param is_rtm_get_only if true, consider only the RTM_GET entries.
     * @return true on success, otherwise false.
     * @see FteX.
     */
    bool parse_buffer_rtm(int family, list<FteX>& fte_list, const uint8_t *buf,
			  size_t buf_bytes, bool is_rtm_get_only);

    /**
     * Parse information about routing table information received from
     * the underlying system.
     * 
     * The information to parse is in NETLINK format
     * (e.g., obtained by netlink(7) sockets mechanism).
     * 
     * @param family the address family to consider only ((e.g., AF_INET
     * or AF_INET6 for IPv4 and IPv6 respectively).
     * @param fte_list the list with the Fte entries to store the result.
     * @param buf the buffer with the data to parse.
     * @param buf_bytes the size of the data in the buffer.
     * @param is_nlm_get_only if true, consider only the entries obtained
     * by RTM_GETROUTE.
     * @return true on success, otherwise false.
     * @see FteX.
     */
    bool parse_buffer_nlm(int family, list<FteX>& fte_list, const uint8_t* buf,
			  size_t buf_bytes, bool is_nlm_get_only);
    
protected:
    int sock(int family);

    int	_s4;
    int _s6;

    // Misc other state
    bool	_is_running;

private:
    FtiConfig&	_ftic;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class FtiConfigTableGetDummy : public FtiConfigTableGet {
public:
    FtiConfigTableGetDummy(FtiConfig& ftic);
    virtual ~FtiConfigTableGetDummy();

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
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table4(list<Fte4>& fte_list);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table6(list<Fte6>& fte_list);
    
private:
    
};

class FtiConfigTableGetSysctl : public FtiConfigTableGet {
public:
    FtiConfigTableGetSysctl(FtiConfig& ftic);
    virtual ~FtiConfigTableGetSysctl();

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
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table4(list<Fte4>& fte_list);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table6(list<Fte6>& fte_list);
    
private:
    bool get_table(int family, list<FteX>& fte_list);
    
};

class FtiConfigTableGetNetlink : public FtiConfigTableGet,
				 public NetlinkSocket4,
				 public NetlinkSocket6 {
public:
    FtiConfigTableGetNetlink(FtiConfig& ftic);
    virtual ~FtiConfigTableGetNetlink();

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
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table4(list<Fte4>& fte_list);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table6(list<Fte6>& fte_list);
    
private:
    bool get_table(int family, list<FteX>& fte_list);

    NetlinkSocketReader	_ns_reader;
};

#endif // __FEA_FTICONFIG_TABLE_GET_HH__
