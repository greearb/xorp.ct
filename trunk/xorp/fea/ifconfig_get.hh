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

// $XORP: xorp/fea/ifconfig.hh,v 1.2 2003/03/10 23:20:15 hodson Exp $

#ifndef __FEA_IFCONFIG_GET_HH__
#define __FEA_IFCONFIG_GET_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"


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

    virtual int pull_config(IfTree& config) = 0;
    virtual void receive_data(const uint8_t* data, size_t n_bytes) = 0;
    
    static string iff_flags(uint32_t flags);
    int sock(int family);
    
protected:
    int parse_buffer_ifaddrs(IfTree& it, const ifaddrs **ifap);
    int parse_buffer_rtm(IfTree& it, const uint8_t *buf, size_t buf_bytes);
    int parse_buffer_ifreq(IfTree& it, int family, const uint8_t *buf,
			   size_t buf_bytes);
    
    int	_s4;
    int _s6;
    
private:
    IfConfig&	_ifc;
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
    
    virtual int pull_config(IfTree& config);
    virtual void receive_data(const uint8_t* data, size_t n_bytes);
    
    virtual int read_config(IfTree& it);
    
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
    
    virtual int pull_config(IfTree& config);
    
    virtual int read_config(IfTree& it);
    
    virtual void receive_data(const uint8_t* data, size_t n_bytes);
    
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
    
    virtual int read_config(IfTree& it);
    virtual int pull_config(IfTree& config);
    
    virtual void receive_data(const uint8_t* data, size_t n_bytes);
    
    int ioctl_socket(int family);
    
private:
};

#endif // __FEA_IFCONFIG_GET_HH__
