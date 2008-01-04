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

#ident "$XORP: xorp/fea/pa_transaction.cc,v 1.13 2007/09/15 19:52:40 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_transaction.hh"
#include "pa_backend.hh"

#include "pa_backend_dummy.hh"
#include "pa_backend_ipfw2.hh"

/* ------------------------------------------------------------------------- */

void
PaTransactionManager::pre_commit(uint32_t tid)
{
    unset_error();

    const PaSnapshot4* ps4 = _ptm.create_snapshot4();
    const PaBackend::Snapshot4Base* bps4 = _pbp->create_snapshot4();
#ifdef notyet
    const PaSnapshot6* ps6 = _ptm.create_snapshot6();
    const PaBackend::Snapshot6Base* bps6 = _pbp->create_snapshot6();
    _pa_transactions.insert(PaTransactionDB::value_type(
	tid, PaTransaction(ps4, ps6, bps4, bps6)));
#else
    _pa_transactions.insert(PaTransactionDB::value_type(
	tid, PaTransaction(ps4, bps4)));
#endif
}

void
PaTransactionManager::post_commit(uint32_t tid)
{
    PaTransactionDB::iterator i = _pa_transactions.find(tid);
    if (i == _pa_transactions.end()) {
	XLOG_ERROR("PaTransaction ID %u is missing snapshots: %s",
		   XORP_UINT_CAST(tid), "not found in PaTransactionDB");
	return;
    }
    PaTransaction& pat = i->second;

    //
    // Check if the transaction succeeded. Back out changes using
    // snapshots of the affected subsystems, if necessary.
    //
    if (!error().empty()) {
	if (pat.snap4() != NULL)
	    pat.snap4()->restore(_ptm);
	if (pat.bsnap4() != NULL)
	    _pbp->restore_snapshot4(pat.bsnap4());
#ifdef notyet
	if (pat.snap6() != NULL)
	    pat.snap6()->restore(_ptm);
	if (pat.bsnap6() != NULL)
	    _pbp->restore_snapshot6(pat.bsnap6());
#endif
    }
    // XXX: The destructor for PaTransaction should delete the snapshots.
    _pa_transactions.erase(i);
}

void
PaTransactionManager::operation_result(bool success,
					const TransactionOperation& op)
{
    if (success)
	return;

    const PaTransactionOperation* fto =
	dynamic_cast<const PaTransactionOperation*>(&op);

    if (fto == 0) {
	//
	// Getting here is programmer error.
	//
	XLOG_ERROR("PaTransaction commit error, \"%s\" is not a "
		   "PaTransactionOperation",
		   op.str().c_str());
	return;
    }

    //
    // Only print the error message for the first failed operation.
    // Flush the transaction if an error occurred, thus halting it.
    //
    if (set_unset_error(fto->str()) == XORP_OK) {
#ifdef notyet
	flush(fto->tid);
#endif
	XLOG_ERROR("PaTransaction commit failed on %s",
		   fto->str().c_str());
    }
}

/* ------------------------------------------------------------------------- */

//
// Attempt to instantiate a back-end ACL provider by name.
//
// Must not be called during a commit; we are implicitly
// serialized by way of the FEA being single-threaded and
// event driven. In any other parallelism scheme,
// a lock must be held.
//
int
PaTransactionManager::set_backend(const char* name)
{
    if (name == NULL)
	return (XORP_ERROR);

    PaBackend* nbp = NULL;

    // Attempt to construct the new backend.
    try {
	if (strcmp(name, "dummy") == 0) {
	    nbp = new PaDummyBackend();
#ifdef notyet
#ifdef HAVE_PACKETFILTER_IPF
	} else if (strcmp(name, "ipf") == 0) {
	    nbp = new PaIpfBackend();
#endif
#endif

#ifdef HAVE_PACKETFILTER_IPFW2
	} else if (strcmp(name, "ipfw2") == 0) {
	    nbp = new PaIpfw2Backend();
#endif

#ifdef notyet
#ifdef HAVE_PACKETFILTER_NF
	// Linux Netfilter, also known as 'iptables'.
	} else if ((strcmp(name, "netfilter") == 0) ||
		   (strcmp(name, "iptables") == 0)) {
	    nbp = new PaNfBackend();
#endif
#endif

#ifdef notyet
#ifdef HAVE_PACKETFILTER_PF
	} else if (strcmp(name, "pf") == 0) {
	    nbp = new PaPfBackend();
#endif
#endif

	} else {
	    return (XORP_ERROR);
	}
    } catch (PaInvalidBackendException) {
	return (XORP_ERROR);
    }

    XLOG_ASSERT(nbp != NULL);

    // Free the old backend if we had one.
    if (_pbp != NULL)
	delete _pbp;
    _pbp = nbp;

    // Push XORP's picture of the ACLs down to the new backend.
    const PaSnapshot4* ps4 = _ptm.create_snapshot4();
    _pbp->push_entries4(ps4);
    delete ps4;
#ifdef notyet
    const PaSnapshot6* ps6 = _ptm.create_snapshot6();
    _pbp->push_entries6(ps6);
    delete ps6;
#endif

    return (XORP_OK);
}

/* ------------------------------------------------------------------------- */

PaTransactionManager::PaTransaction::PaTransaction(
    const PaSnapshot4* sn4, const PaBackend::Snapshot4Base* bsn4)
    : _snap4(sn4), _bsnap4(bsn4)
{
}

PaTransactionManager::PaTransaction::~PaTransaction()
{
    if (_snap4 != NULL)
	delete _snap4;

    if (_bsnap4 != NULL)
	delete _bsnap4;

#ifdef notyet
    if (_snap6 != NULL)
	delete _snap6;

    if (_bsnap4 != NULL)
	delete _bsnap6;
#endif
}

/* ------------------------------------------------------------------------- */
