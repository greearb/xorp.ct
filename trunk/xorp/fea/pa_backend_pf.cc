// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/pa_backend_pf.cc,v 1.6 2007/09/15 21:37:38 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/stat.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NET_PFVAR_H
#include <net/pfvar.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"
#include "pa_backend_pf.hh"

/* ------------------------------------------------------------------------- */
/* Constructor and destructor. */

const char* PaPfBackend::_pfdevname = "/dev/pf";

PaPfBackend::PaPfBackend() throw(PaInvalidBackendException)
    : _fd(-1)
{
#ifndef HAVE_PACKETFILTER_PF
    throw PaInvalidBackendException();
#else

    // Attempt to probe for PF with an operation which doesn't
    // change any of the state.

    // Open the pf device.
    _fd = open(_pfdevname, O_RDWR);
    if (_fd == -1) {
	XLOG_ERROR("Could not open PF device.");
	throw PaInvalidBackendException();
    }

    // Initialize rulesets to a known state.
#endif
}

PaPfBackend::~PaPfBackend()
{
#ifdef HAVE_PACKETFILTER_PF
    if (_fd != -1) {
	::close(_fd);
    }
#endif
}

/* ------------------------------------------------------------------------- */
/* General back-end methods. */

const char*
PaPfBackend::get_name() const
{
    return ("pf");
}

const char*
PaPfBackend::get_version() const
{
    return ("0.1");
}

/* ------------------------------------------------------------------------- */
/* IPv4 ACL back-end methods. */

int
PaPfBackend::push_entries4(const PaSnapshot4* snap)
{
#ifndef HAVE_PACKETFILTER_PF
    UNUSED(snap);
    return (XORP_ERROR);
#else
    XLOG_ASSERT(snap != NULL);

    const PaSnapTable4& table = snap->data();
    uint32_t rulebuf[MAX_PF_RULE_WORDS];
    uint32_t size_used;
    int ret_value;
    uint32_t ticket;

    ret_value = XORP_ERROR;

    // Begin a pf transaction.
    ticket = start_transaction();
    if (ticket == -1)
	return (ret_value);

    // Transcribe all the rules contained within the snapshot,
    // pushing each one into the inactive ruleset.
    for (PaSnapTable4::const_iterator i = table.begin();
	 i != table.end(); i++) {
	ret_value = transcribe_and_push_rule4(*i, ticket);
	if (ret_value != XORP_OK)
	    break;
    }

    // Attempt to commit the transaction, thus flipping
    // the active and inactive rulesets.
    if (ret_value == XORP_OK)
	ret_value = commit_transaction(ticket);

    return (ret_value);
#endif
}

// Clear all entries by committing an empty transaction.
int
PaPfBackend::delete_all_entries4()
{
#ifndef HAVE_PACKETFILTER_PF
    return (XORP_ERROR);
#else
    struct pfioc_trans trans;
    struct pfioc_trans_e trans_e;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;

    if ((0 < ioctl(_fd, DIOCXBEGIN, &trans)) ||
	(0 < ioctl(_fd, DIOCXCOMMIT, &trans))) {
	XLOG_ERROR("Failed to delete all entries from PF "
		   "firewall: %s", strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#endif
}

const PaBackend::Snapshot4Base*
PaPfBackend::create_snapshot4()
{
#ifndef HAVE_PACKETFILTER_PF
    return (NULL);
#else

    struct pfioc_rule pr;
    struct pfioc_rule *ppr;
    struct pfioc_rule *ppvec;
    int i;

    // Get a ticket and find out how many rules there are.
    memset(&pr, 0, sizeof(pr));
    if (0 < ioctl(_fd, DIOCGETRULES, &pr)) {
	XLOG_ERROR("Cannot snapshot PF rule state: DIOCGETRULES: %s",
		   strerror(errno));
	return (NULL);
    }

    // XXX: deal with pr.nr == 0 by creating an empty snapshot.
    if (pr.nr == 0) {
	PaPfBackend::Snapshot4* snap = new PaPfBackend::Snapshot4(*this);
	return (snap);
    }

    ppvec = calloc(pr.nr, sizeof(*ppvec));
    if (ppvec == NULL) {
	XLOG_ERROR("Cannot allocate PF rule state: calloc: %s",
		    strerror(errno));
	return (NULL);
    }

    ppr = ppvec;
    for (i = 0; i < pr.nr; i++) {
	memset(ppr, 0, sizeof(*ppr));
	ppr->ticket = pr.ticket;
	if (0 < ioctl(_fd, DIOCGETRULE, ppr)) {
	    XLOG_ERROR("Cannot snapshot PF rule state: DIOCGETRULE: %s",
		       strerror(errno));
	    break;
	}
	ppr++;
    }

    if (i != pr.nr) {
	free(ppvec);
	return (NULL);
    }

    // Create a snapshot of the current PF rule set.
    // Use a private constructor.
    PaPfBackend::Snapshot4* snap = new PaPfBackend::Snapshot4(*this,
							      pr.nr, ppr);

    return (snap);
#endif
}

int
PaPfBackend::restore_snapshot4(const PaBackend::Snapshot4Base* snap4)
{
#ifndef HAVE_PACKETFILTER_PF
    UNUSED(snap4);
    return (XORP_ERROR);
#else
    // Simply check if the snapshot is an instance of our derived snapshot.
    // If it is not, we received it in error, and it is of no interest to us.
    const PaPfBackend::Snapshot4* dsnap4 =
	dynamic_cast<const PaPfBackend::Snapshot4*>(snap4);
    XLOG_ASSERT(dsnap4 != NULL);

    // Begin a pf transaction.
    uint32_t ticket = start_transaction();
    if (ticket == -1)
	return (XORP_ERROR);

    // Push the rules back.
    struct pfioc_rule *ppr = dsnap4._rulebuf;
    for (i = 0; i < dsnap4._nrules; i++) {
	if (0 < ioctl(_fd, DIOCADDRULE, ppr)) {
	    XLOG_ERROR("Failed to add rule to a PF firewall "
		       "transaction: %s", strerror(errno));
	    return (XORP_ERROR);
	}
	ppr++;
    }

    // Commit.
    return (commit_transaction(ticket));
#endif
}

#ifdef HAVE_PACKETFILTER_PF
/* ------------------------------------------------------------------------- */
/* Private utility methods used for manipulating PF itself */

// Enable or disable the pf firewall.
int
PaPfBackend::set_pf_enabled(bool enable)
{
    int ret_value = XORP_OK;

    if (enable) {
	if (0 > ioctl(_fd, DIOCSTART)) {
	    if (errno == EEXIST) {
		XLOG_ERROR("PF already enabled");
#ifdef __FreeBSD__
	    } else if (errno == ESRCH) {
		XLOG_ERROR("pfil_hooks registration failed");
		ret_value = XORP_ERROR;
#endif
	    } else {
		XLOG_ERROR("DIOCSTART: %s", strerror(errno));
		ret_value = XORP_ERROR;
	    }
	}
    } else {
	if (0 > ioctl(_fd, DIOCSTOP)) {
	    if (errno == ENOENT) {
		XLOG_ERROR("PF already disabled");
	    } else {
		XLOG_ERROR("DIOCSTOP: %s", strerror(errno));
		ret_value = XORP_ERROR;
	    }
	}
    }
    return (retval);
}

int
PaPfBackend::start_transaction()
{
    struct pfioc_trans trans;
    struct pfioc_trans_e trans_e;
    int result;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;

    if (0 < ioctl(_fd, DIOCXBEGIN, &trans)) {
	XLOG_ERROR("Failed to begin transaction for adding rules"
		   "to PF firewall: %s", strerror(errno));
	return (-1);
    }

    // XXX: Do we *need* a 'pool ticket' ?

    return (trans_e.ticket);
}

int
PaPfBackend::abort_transaction(uint32_t ticket)
{
    struct pfioc_trans trans;
    struct pfioc_trans_e trans_e;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;
    trans_e.ticket = ticket;

    if (0 < ioctl(_fd, DIOCXROLLBACK, &trans)) {
	XLOG_ERROR("Failed to abort transaction for adding rules"
		   "to PF firewall: %s", strerror(errno));
	return (false);
    }
}

int
PaPfBackend::commit_transaction(uint32_t ticket)
{
    struct pfioc_trans trans;
    struct pfioc_trans_e trans_e;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;
    trans_e.ticket = ticket;

    if (0 < ioctl(_fd, DIOCXCOMMIT, &trans)) {
	XLOG_ERROR("Failed to commit transaction for adding rules"
		   "to PF firewall: %s", strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
PaPfBackend::transcribe_and_add_rule4(const PaEntry4& entry, uint32_t ticket)
{
    struct pfioc_rule pfr;

    memset(&pfr, 0, sizeof(pfr));
    pfr.ticket = ticket;
    //pfr.pool_ticket = pticket;    // XXX: needed?

    //
    // Transcribe the XORP rule to a PF rule.
    //
    pfr.rule.src.type = PF_ADDR_ADDRMASK;
    pfr.rule.src.iflags = 0;	// PFI_AFLAG_NETWORK ?
    entry.src().masked_addr().copy_out(&pfr.rule.src.a.addr.addr.v4);
    entry.src().netmask().copy_out(&pfr.rule.src.a.addr.mask.v4);
    pfr.rule.src.port[0] = entry().sport(); // XXX

    pfr.rule.dst.type = PF_ADDR_ADDRMASK;
    pfr.rule.dst.iflags = 0;	// PFI_AFLAG_NETWORK ?
    entry.dst().masked_addr().copy_out(&pfr.rule.dst.a.addr.addr.v4);
    entry.dst().netmask().copy_out(&pfr.rule.dst.a.addr.mask.v4);
    pfr.rule.dst.port[1] = entry().dport(); // XXX

    if (entry.action() == PaAction(ACTION_PASS)) {
	pfr.rule.action = PF_PASS;
    } else if (entry.action() == PaAction(ACTION_DROP)) {
	pfr.rule.action = PF_DROP;
    } else {
	XLOG_UNREACHABLE();
    }

    pfr.rule.direction = PF_INOUT;
    pfr.rule.af = AF_INET;
    pfr.rule.proto = entry.proto();

    if (0 < ioctl(_fd, DIOCADDRULE, &pfr)) {
	XLOG_ERROR("Failed to add rule to a PF firewall "
		   "transaction: %s", strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/* ------------------------------------------------------------------------- */
/* Snapshot scoped classes (Memento pattern) */

// Cannot be copied or assigned from base class.
PaPfBackend::Snapshot4::Snapshot4(const PaBackend::Snapshot4Base& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(snap4),
    _parent(NULL), _nrules(0), _rulebuf(NULL)
{
    throw PaInvalidSnapshotException();
}

// May be copied or assigned from own class, but
// requires deep copy of rule buffer.
PaPfBackend::Snapshot4::Snapshot4(const PaPfBackend::Snapshot4& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(snap4),
      _parent(snap4._parent), _nrules(snap4._nrules)
{
    if (snap4._rulebuf == NULL) {
	_rulebuf = NULL;
    } else {
	struct pfioc_rule *ppr = calloc(_nrules, sizeof(*ppr));
	if (ppr == NULL)
	    throw PaInvalidSnapshotException();
	memcpy(ppr, snap4._rulebuf, (_nrules * sizeof(*ppr)));
    }
}

// This constructor is marked private and used internally
// by the parent class.
PaPfBackend::Snapshot4::Snapshot4(PaPfBackend& parent, int nrules,
				  struct pfioc_rule *rulebuf)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(),
      _parent(&parent), _nrules(nrules), _rulebuf(rulebuf)
{
}

// Deep free.
PaPfBackend::Snapshot4::~Snapshot4()
{
    if (_rulebuf != NULL)
	free(_rulebuf);
}

/* ------------------------------------------------------------------------- */
#endif // HAVE_PACKETFILTER_PF
