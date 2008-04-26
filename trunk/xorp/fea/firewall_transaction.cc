// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008 International Computer Science Institute
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

#ident "$XORP$"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "firewall_transaction.hh"

void
FirewallTransactionManager::pre_commit(uint32_t tid)
{
    reset_error();
    _tid_exec = tid;
}

void
FirewallTransactionManager::operation_result(bool success,
					     const TransactionOperation& op)
{
    if (success)
	return;

    const FirewallTransactionOperation* fto;
    fto = dynamic_cast<const FirewallTransactionOperation*>(&op);
    XLOG_ASSERT(fto != NULL);

    if (_first_error.empty()) {
	_first_error = c_format("Failed executing: \"%s\": %s",
				fto->str().c_str(),
				fto->error_reason().c_str());
	flush(_tid_exec);
    }
}
