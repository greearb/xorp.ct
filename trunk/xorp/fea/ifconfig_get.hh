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

// $XORP: xorp/fea/ifconfig_get.hh,v 1.7 2003/08/21 23:47:54 fred Exp $

#ifndef __FEA_IFCONFIG_GET_HH__
#define __FEA_IFCONFIG_GET_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "netlink_socket.hh"

class IfConfig;
class IfTree;
struct ifaddrs;

class IfConfigGet {
public:
    IfConfigGet(IfConfig& ifc);
    
    virtual ~IfConfigGet();
    
    IfConfig&	ifc() { return _ifc; }
    
    virtual void register_ifc();
    
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

    virtual bool pull_config(IfTree& config) = 0;
    
    static string iff_flags(uint32_t flags);
    int sock(int family);

    bool parse_buffer_ifaddrs(IfTree& it, const ifaddrs **ifap);
    bool parse_buffer_rtm(IfTree& it, const uint8_t *buf, size_t buf_bytes);
    bool parse_buffer_ifreq(IfTree& it, int family, const uint8_t *buf,
			    size_t buf_bytes);
    bool parse_buffer_nlm(IfTree& it, const uint8_t *buf, size_t buf_bytes);
    
protected:
    int	_s4;
    int _s6;
    
private:
    IfConfig&	_ifc;
};

class IfConfigGetDummy : public IfConfigGet {
public:
    IfConfigGetDummy(IfConfig& ifc);
    virtual ~IfConfigGetDummy();

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
    
    virtual bool pull_config(IfTree& config);
    
private:
    
};

class IfConfigGetGetifaddrs : public IfConfigGet {
public:
    IfConfigGetGetifaddrs(IfConfig& ifc);
    virtual ~IfConfigGetGetifaddrs();

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
    
    virtual bool pull_config(IfTree& config);
    
    virtual bool read_config(IfTree& it);
    
private:
    
};

class IfConfigGetSysctl : public IfConfigGet {
public:
    IfConfigGetSysctl(IfConfig& ifc);
    virtual ~IfConfigGetSysctl();

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
    
    virtual bool pull_config(IfTree& config);
    virtual bool read_config(IfTree& it);
    
private:
    
};

class IfConfigGetIoctl : public IfConfigGet {
public:
    IfConfigGetIoctl(IfConfig& ifc);
    virtual ~IfConfigGetIoctl();
    
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
    
    virtual bool read_config(IfTree& it);
    virtual bool pull_config(IfTree& config);
    
private:
};

class IfConfigGetProcLinux : public IfConfigGet {
public:
    IfConfigGetProcLinux(IfConfig& ifc);
    virtual ~IfConfigGetProcLinux();
    
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
    
    virtual bool read_config(IfTree& it);
    virtual bool pull_config(IfTree& config);
    
private:
};

class IfConfigGetNetlink : public IfConfigGet,
			   public NetlinkSocket4,
			   public NetlinkSocket6,
			   public NetlinkSocketObserver {
public:
    IfConfigGetNetlink(IfConfig& ifc);
    virtual ~IfConfigGetNetlink();
    
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
    
    virtual bool read_config(IfTree& it);
    virtual bool pull_config(IfTree& config);
    
    /**
     * Data has pop-up.
     * 
     * @param data the buffer with the data.
     * @param nbytes the number of bytes in the @ref data buffer.
     */
    virtual void nlsock_data(const uint8_t* data, size_t nbytes);
    
private:
    
    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of netlink socket data to
					// cache so route lookup via netlink
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached netlink socket data.
};

#endif // __FEA_IFCONFIG_GET_HH__
