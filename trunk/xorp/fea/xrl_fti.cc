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

#ident "$XORP: xorp/fea/xrl_fti.cc,v 1.2 2003/03/10 23:20:17 hodson Exp $"

#include "xrl_fti.hh"

static const char* FTI_MAX_OPS_HIT =
"Resource limit on number of operations in a transaction hit.";

static const char* FTI_MAX_TRANSACTIONS_HIT =
"Resource limit on number of pending transactions hit.";

static const char* FTI_BAD_ID =
"Expired or invalid transaction id presented.";

XrlCmdError
XrlFtiTransactionManager::start_transaction(uint32_t& tid)
{
    if (_ftm.start(tid))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(FTI_MAX_TRANSACTIONS_HIT);
}

XrlCmdError
XrlFtiTransactionManager::commit_transaction(uint32_t tid)
{
    if (_ftm.commit(tid)) {
	const string& errmsg = _ftm.error();
	if (errmsg.empty())
	    return XrlCmdError::OKAY();
	return XrlCmdError::COMMAND_FAILED(errmsg);
    }
    return XrlCmdError::COMMAND_FAILED(FTI_BAD_ID);
}

XrlCmdError
XrlFtiTransactionManager::abort_transaction(uint32_t tid)
{
    if (_ftm.abort(tid))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(FTI_BAD_ID);
}

XrlCmdError
XrlFtiTransactionManager::add(uint32_t tid,
			      const FtiTransactionManager::Operation& op)
{
    uint32_t n_ops;

    if (_ftm.retrieve_size(tid, n_ops) == false)
	return XrlCmdError::COMMAND_FAILED(FTI_BAD_ID);

    if (_max_ops <= n_ops)
	return XrlCmdError::COMMAND_FAILED(FTI_MAX_OPS_HIT);

    if (_ftm.add(tid, op))
	return XrlCmdError::OKAY();

    //
    // In theory, resource shortage is the only thing that could get us
    // here.
    //
    return XrlCmdError::COMMAND_FAILED("Unknown resource shortage");
}
