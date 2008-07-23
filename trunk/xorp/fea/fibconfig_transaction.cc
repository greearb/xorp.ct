// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/fibconfig_transaction.cc,v 1.6 2008/04/11 18:38:07 pavlin Exp $"

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
