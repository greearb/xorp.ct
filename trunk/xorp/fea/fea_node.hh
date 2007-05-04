// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

// $XORP: xorp/fea/fea_node.hh,v 1.4 2007/04/27 01:10:27 pavlin Exp $


#ifndef __FEA_FEA_NODE_HH__
#define __FEA_FEA_NODE_HH__


//
// FEA (Forwarding Engine Abstraction) node implementation.
//

#include "libxorp/profile.hh"

#include "fibconfig.hh"
#include "ifconfig.hh"
#include "nexthop_port_mapper.hh"
#include "pa_table.hh"
#include "pa_transaction.hh"

class EventLoop;

/**
 * @short The FEA (Forwarding Engine Abstraction) node class.
 * 
 * There should be one node per FEA instance.
 */
class FeaNode {
public:
    /**
     * Constructor for a given event loop.
     *
     * @param eventloop the event loop to use.
     */
    FeaNode(EventLoop& eventloop);

    /**
     * Destructor
     */
    virtual	~FeaNode();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
     *
     * Gracefully shutdown the FEA.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		shutdown();

    /**
     * Test whether the service is running.
     *
     * @return true if the service is still running, otherwise false.
     */
    bool	is_running() const;

    /**
     * Setup the unit to behave as dummy (for testing purpose).
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_dummy();

    /**
     * Test if running in dummy mode.
     * 
     * @return true if running in dummy mode, otherwise false.
     */
    bool	is_dummy() const { return _is_dummy; }

    /**
     * Get the event loop this service is added to.
     * 
     * @return the event loop this service is added to.
     */
    EventLoop& eventloop() { return (_eventloop); }

    /**
     * Get the Profile instance.
     *
     * @return a reference to the Profile instance.
     * @see Profile.
     */
    Profile& profile() { return (_profile); }

    /**
     * Get the IfConfig instance.
     *
     * @return a reference to the IfConfig instance.
     * @see IfConfig.
     */
    IfConfig& ifconfig() { return (_ifconfig); }

    /**
     * Get the FibConfig instance.
     *
     * @return a reference to the FibConfig instance.
     * @see FibConfig.
     */
    FibConfig& fibconfig() { return (_fibconfig); }

    /**
     * Get the PaTransactionManager instance.
     *
     * @return a reference to the PaTransactionManager instance.
     * @see PaTransactionManager.
     */
    PaTransactionManager& pa_transaction_manager() { return (_pa_transaction_manager); }

private:
    EventLoop&	_eventloop;	// The event loop to use
    bool	_is_running;	// True if the service is running
    bool	_is_dummy;	// True if running in dummy node
    Profile	_profile;	// Profile entity
    NexthopPortMapper		_nexthop_port_mapper;	// Next-hop port mapper

    IfConfig			_ifconfig;

    FibConfig			_fibconfig;

    PaTableManager		_pa_table_manager;
    PaTransactionManager	_pa_transaction_manager;
};

#endif // __FEA_FEA_NODE_HH__
