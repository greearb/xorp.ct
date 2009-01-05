// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/fea/firewall_transaction.cc,v 1.3 2008/10/02 21:56:46 bms Exp $"

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
