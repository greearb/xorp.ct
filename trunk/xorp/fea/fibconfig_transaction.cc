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

#ident "$XORP: xorp/fea/fibconfig_transaction.cc,v 1.7 2008/07/23 05:10:08 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fibconfig_transaction.hh"


int
FibConfigTransactionManager::set_error(const string& error)
{
    if (! _first_error.empty())
	return (XORP_ERROR);

    _first_error = error;
    return (XORP_OK);
}

void
FibConfigTransactionManager::pre_commit(uint32_t /* tid */)
{
    string error_msg;

    reset_error();

    if (fibconfig().start_configuration(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot start configuration: %s", error_msg.c_str());
	set_error(error_msg);
    }
}

void
FibConfigTransactionManager::post_commit(uint32_t /* tid */)
{
    string error_msg;

    if (fibconfig().end_configuration(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot end configuration: %s", error_msg.c_str());
	set_error(error_msg);
    }
}

void
FibConfigTransactionManager::operation_result(bool success,
					      const TransactionOperation& op)
{
    if (success)
	return;

    const FibConfigTransactionOperation* fto;
    fto = dynamic_cast<const FibConfigTransactionOperation*>(&op);
    XLOG_ASSERT(fto != NULL);

    //
    // Record error and xlog first error only
    //
    if (set_error(fto->str()) == XORP_OK) {
	XLOG_ERROR("FIB transaction commit failed on %s", fto->str().c_str());
    }
}
