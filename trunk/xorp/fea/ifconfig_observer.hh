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

// $XORP: xorp/fea/ifconfig_observer.hh,v 1.1 2003/05/02 07:50:47 pavlin Exp $

#ifndef __FEA_IFCONFIG_OBSERVER_HH__
#define __FEA_IFCONFIG_OBSERVER_HH__

#include "netlink_socket.hh"
#include "routing_socket.hh"


class IfConfig;
class IfTree;

class IfConfigObserver {
public:
    IfConfigObserver(IfConfig& ifc);
    
    virtual ~IfConfigObserver();
    
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
    
private:
    IfConfig&	_ifc;
};

class IfConfigObserverRtsock : public IfConfigObserver,
			       public RoutingSocket,
			       public RoutingSocketObserver {
public:
    IfConfigObserverRtsock(IfConfig& ifc);
    virtual ~IfConfigObserverRtsock();

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
    
    void rtsock_data(const uint8_t* data, size_t nbytes);
    
private:
    
};


#endif // __FEA_IFCONFIG_OBSERVER_HH__
