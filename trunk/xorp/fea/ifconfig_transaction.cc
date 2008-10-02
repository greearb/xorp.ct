// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/ifconfig_transaction.cc,v 1.5 2008/07/23 05:10:09 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig_transaction.hh"

void
IfConfigTransactionManager::pre_commit(uint32_t tid)
{
    reset_error();
    _tid_exec = tid;
}

void
IfConfigTransactionManager::operation_result(bool success,
					     const TransactionOperation& op)
{
    if (false == success && _first_error.empty()) {
	_first_error = c_format("Failed executing: \"%s\"", op.str().c_str());
	flush(_tid_exec);
    }
}
