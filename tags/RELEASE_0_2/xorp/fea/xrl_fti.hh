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

// $XORP: xorp/fea/xrl_fti.hh,v 1.2 2002/12/14 23:42:51 hodson Exp $

#ifndef __FEA_XRL_FTI_HH__
#define __FEA_XRL_FTI_HH__

#include "libxipc/xrl_router.hh"
#include "fti_transaction.hh"

/**
 * Helper class for helping with Fti transactions via an Xrl
 * interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the FtiTransactionManager
 * class.
 */
class XrlFtiTransactionManager
{
public:
    /**
     * Constructor
     *
     * @param e the EventLoop.
     * @param fti the ForwardingTableInterface object.
     * @param max_ops the maximum number of operations pending.
     */
    XrlFtiTransactionManager(EventLoop&	e,
			     Fti&	fti,
			     uint32_t	max_ops = 200)
	: _ftm(e, fti), _max_ops(max_ops)
    {}

    XrlCmdError start_transaction(uint32_t& tid);

    XrlCmdError commit_transaction(uint32_t tid);

    XrlCmdError abort_transaction(uint32_t tid);

    XrlCmdError add(uint32_t tid, const FtiTransactionManager::Operation& op);

    inline Fti& fti() { return _ftm.fti(); }

protected:
    FtiTransactionManager  _ftm;
    uint32_t		   _max_ops;	// Maximum operations in a transaction
};

#endif // __FEA_XRL_FTI_HH__
