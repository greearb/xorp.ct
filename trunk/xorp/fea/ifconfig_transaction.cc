// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_transaction.cc,v 1.3 2007/04/27 23:48:57 pavlin Exp $"

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
