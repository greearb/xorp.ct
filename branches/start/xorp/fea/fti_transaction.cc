// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fti_transaction.cc,v 1.9 2002/12/09 18:28:56 hodson Exp $"

#include "config.h"
#include "fea_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "fti_transaction.hh"

void
FtiTransactionManager::pre_commit(uint32_t /* tid */)
{
    unset_error();
    fti().start_configuration();
}

void
FtiTransactionManager::post_commit(uint32_t /* tid */)
{
    fti().end_configuration();
}

void
FtiTransactionManager::operation_result(bool success, TransactionOperation& op)
{
    if (success) {
	return;
    }

    FtiTransactionOperation* fto = dynamic_cast<FtiTransactionOperation*>(&op);
    if (fto == 0) {
	//
	// Getting here is programmer error.
	//
	XLOG_ERROR("FTI transaction commit error, \"%s\" is not an FTI "
		   "TransactionOperation",
		   op.str().c_str());
	return;
    }

    //
    // Record error and xlog first error only
    //
    if (set_unset_error(fto->str())) {
	XLOG_ERROR("FTI transaction commit failed on %s\n",
		   fto->str().c_str());
    }
}
