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

// $XORP: xorp/fea/fticonfig_table_get.hh,v 1.2 2003/05/10 00:06:39 pavlin Exp $

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
     * Receive data.
     * 
     * @param data the buffer with the data.
     * @param n_bytes the number of bytes in the @param data buffer.
     */
    virtual void receive_data(const uint8_t* data, size_t n_bytes) = 0;
    
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

    int sock(int family);
    
protected:
    bool parse_buffer_rtm(int family, list<FteX>& fte_list, const uint8_t *buf,
			  size_t buf_bytes);
    bool parse_buffer_nlm(int family, list<FteX>& fte_list, const uint8_t* buf,
			  size_t buf_bytes);
    
    int	_s4;
    int _s6;
    
private:
    FtiConfig&	_ftic;
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
     * Receive data.
     * 
     * @param data the buffer with the data.
     * @param n_bytes the number of bytes in the @param data buffer.
     */
    virtual void receive_data(const uint8_t* data, size_t n_bytes);
    
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
     * Receive data.
     * 
     * @param data the buffer with the data.
     * @param n_bytes the number of bytes in the @param data buffer.
     */
    virtual void receive_data(const uint8_t* data, size_t n_bytes);
    
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
				 public NetlinkSocket,
				 public NetlinkSocketObserver {
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
     * Data has pop-up.
     * 
     * @param data the buffer with the data.
     * @param n_bytes the number of bytes in the @param data buffer.
     */
    virtual void nlsock_data(const uint8_t* data, size_t n_bytes);
    
    /**
     * Receive data.
     * 
     * @param data the buffer with the data.
     * @param n_bytes the number of bytes in the @param data buffer.
     */
    virtual void receive_data(const uint8_t* data, size_t n_bytes);
    
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
    
    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of netlink socket data to
					// cache so table fetch via netlink
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached netlink socket data.
};

#endif // __FEA_FTICONFIG_TABLE_GET_HH__
