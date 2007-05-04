// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_get.hh,v 1.35 2007/05/01 01:42:37 pavlin Exp $

#ifndef __FEA_IFCONFIG_GET_HH__
#define __FEA_IFCONFIG_GET_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "fea/forwarding_plane/control_socket/click_socket.hh"
#include "fea/forwarding_plane/control_socket/netlink_socket.hh"

class IfConfig;
class IfTree;

class IfConfigGet {
public:
    IfConfigGet(IfConfig& ifconfig)
	: _is_running(false),
	  _ifconfig(ifconfig),
	  _is_primary(true)
    {}
    virtual ~IfConfigGet() {}
    
    IfConfig&	ifconfig() { return _ifconfig; }
    
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }
    virtual bool is_running() const { return _is_running; }
    
    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config) = 0;
    
protected:
    // Misc other state
    bool	_is_running;

private:
    IfConfig&	_ifconfig;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class IfConfigGetDummy : public IfConfigGet {
public:
    IfConfigGetDummy(IfConfig& ifconfig);
    virtual ~IfConfigGetDummy();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    
};

class IfConfigGetGetifaddrs : public IfConfigGet {
public:
    IfConfigGetGetifaddrs(IfConfig& ifconfig);
    virtual ~IfConfigGetGetifaddrs();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in "struct ifaddrs" format
     * (e.g., obtained by getifaddrs(3) mechanism).
     * 
     * @param ifconfig the IfConfig instance.
     * @param it the IfTree storage to store the parsed information.
     * @param ifap a linked list of the network interfaces on the
     * local machine.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    static bool parse_buffer_getifaddrs(IfConfig& ifconfig, IfTree& it,
					const struct ifaddrs* ifap);

private:
    bool read_config(IfTree& it);
};

class IfConfigGetSysctl : public IfConfigGet {
public:
    IfConfigGetSysctl(IfConfig& ifconfig);
    virtual ~IfConfigGetSysctl();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in RTM format
     * (e.g., obtained by routing sockets or by sysctl(3) mechanism).
     * 
     * @param ifconfig the IfConfig instance.
     * @param it the IfTree storage to store the parsed information.
     * @param buffer the buffer with the data to parse.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    static bool parse_buffer_routing_socket(IfConfig& ifconfig, IfTree& it,
					    const vector<uint8_t>& buffer);

private:
    bool read_config(IfTree& it);
    static string iff_flags(uint32_t flags);
};

class IfConfigGetIoctl : public IfConfigGet {
public:
    IfConfigGetIoctl(IfConfig& ifconfig);
    virtual ~IfConfigGetIoctl();
    
    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in "struct ifreq" format
     * (e.g., obtained by ioctl(SIOCGIFCONF) mechanism).
     * 
     * @param ifconfig the IfConfig instance.
     * @param it the IfTree storage to store the parsed information.
     * @param family the address family to consider only (e.g., AF_INET
     * or AF_INET6 for IPv4 and IPv6 respectively).
     * @param buffer the buffer with the data to parse.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    static bool parse_buffer_ioctl(IfConfig& ifconfig, IfTree& it, int family,
				   const vector<uint8_t>& buffer);

private:
    bool read_config(IfTree& it);

    int _s4;
    int _s6;
};

class IfConfigGetProcLinux : public IfConfigGet {
public:
    IfConfigGetProcLinux(IfConfig& ifconfig);
    virtual ~IfConfigGetProcLinux();
    
    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);

    static const string PROC_LINUX_NET_DEVICES_FILE_V4;
    static const string PROC_LINUX_NET_DEVICES_FILE_V6;
};

class IfConfigGetNetlinkSocket : public IfConfigGet,
				 public NetlinkSocket {
public:
    IfConfigGetNetlinkSocket(IfConfig& ifconfig);
    virtual ~IfConfigGetNetlinkSocket();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);

    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in NETLINK format
     * (e.g., obtained by netlink(7) sockets mechanism).
     * 
     * @param ifconfig the IfConfig instance.
     * @param it the IfTree storage to store the parsed information.
     * @param buffer the buffer with the data to parse.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    static bool parse_buffer_netlink_socket(IfConfig& ifconfig, IfTree& it,
					    const vector<uint8_t>& buffer);
    
private:
    bool read_config(IfTree& it);

    NetlinkSocketReader	_ns_reader;
};

class IfConfigGetClick : public IfConfigGet,
			 public ClickSocket {
public:
    IfConfigGetClick(IfConfig& ifconfig);
    virtual ~IfConfigGetClick();
    
    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);

    ClickSocketReader	_cs_reader;
};

class IfConfigGetIPHelper : public IfConfigGet {
public:
    IfConfigGetIPHelper(IfConfig& ifconfig);
    virtual ~IfConfigGetIPHelper();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);
};

#endif // __FEA_IFCONFIG_GET_HH__
