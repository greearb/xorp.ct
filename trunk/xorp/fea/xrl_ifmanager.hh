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

// $XORP: xorp/fea/xrl_ifmanager.hh,v 1.14 2007/04/23 22:14:11 pavlin Exp $

#ifndef __FEA_XRL_IFMANAGER_HH__
#define __FEA_XRL_IFMANAGER_HH__

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"
#include "libxipc/xrl_router.hh"
#include "ifmanager_transaction.hh"

/**
 * Helper class for helping with Interface configuration transactions
 * via an Xrl interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the IfConfig class.
 */
class XrlInterfaceManager {
public:
    typedef InterfaceTransactionManager::Operation Operation;

    /**
     * Constructor
     *
     * @param eventloop the EventLoop.
     * @param ifconfig the IfConfig object.
     * @param max_ops the maximum number of operations pending.
     */
    XrlInterfaceManager(EventLoop&	eventloop,
			IfConfig&	ifconfig,
			uint32_t	max_ops = 200)
	: _itm(eventloop, 5000, 10), _ifconfig(ifconfig), _max_ops(max_ops)
    {}

    /**
     * get the status of the interface manager.
     *
     * @param reason the human-readable reason for any failure.
     * @return the status of the interface manager.
     */
    ProcessStatus status(string& reason) const;

    //
    // Transaction related methods
    //
    XrlCmdError start_transaction(uint32_t& tid);

    XrlCmdError commit_transaction(uint32_t tid);

    XrlCmdError abort_transaction(uint32_t tid);

    XrlCmdError add(uint32_t tid, const Operation& op);

    //
    // Miscellaneous helper methods
    //
    inline IfTree& iftree() const	{ return _ifconfig.local_config(); }

    inline IfTree& old_iftree() const	{ return _ifconfig.old_local_config(); }

    inline IfConfig& ifconfig() const	{ return _ifconfig; }

protected:

protected:
    InterfaceTransactionManager	_itm;
    IfConfig&			_ifconfig;
    uint32_t			_max_ops;
};

#endif // __FEA_XRL_IFMANAGER_HH__
