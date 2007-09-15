// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/pa_backend_nf.cc,v 1.3 2007/09/15 19:52:40 pavlin Exp $"

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
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef HAVE_PACKETFILTER_NF
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#endif

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"
#include "pa_backend_nf.hh"

/* ------------------------------------------------------------------------- */
/* Constructor and destructor. */

PaNfBackend::PaNfBackend() throw(PaInvalidBackendException)
    : _fd4(-1),
      _fd6(-1)
{
#ifndef HAVE_PACKETFILTER_NF
    throw PaInvalidBackendException();
#else

    _fd4 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (_fd4 == -1) {
	XLOG_ERROR("Could not open raw IPv4 socket.");
	throw PaInvalidBackendException();
    }

    // num_entries and size used for retrieving chain, possibly racy.
    struct ipt_getinfo info4;
    socklen_t size;

    memset(info4, 0, sizeof(info4));
    strncpy(info4.name, "FILTER", IPT_TABLE_MAXNAMELEN);
    if (getsockopt(_fd4, TC_IPPROTO, SO_GET_INFO, &info4, &size) < 0) {
        XLOG_ERROR("Error retrieving Netfilter info: getsockopt "
		   "SO_GET_INFO: %s", strerror(errno));
	throw PaInvalidBackendException();
    }

    // XXX: TODO: IPv6 socket.

#endif
}

PaNfBackend::~PaNfBackend()
{
#ifdef HAVE_PACKETFILTER_NF
    if (_fd4 != -1) {
	::close(_fd4);
    }
    // XXX: TODO: IPv6 socket.
#endif
}

/* ------------------------------------------------------------------------- */
/* General back-end methods. */

const char*
PaNfBackend::get_name() const
{
    return ("netfilter");
}

const char*
PaNfBackend::get_version() const
{
    return ("0.1");
}

/* ------------------------------------------------------------------------- */
/* IPv4 ACL back-end methods. */

int
PaNfBackend::push_entries4(const PaSnapshot4* snap)
{
#ifndef HAVE_PACKETFILTER_NF
    UNUSED(snap);
    return (XORP_ERROR);
#else
    XLOG_ASSERT(snap != NULL);

    const PaSnapTable4& table = snap->data();
    int ret_value;
    struct ipt_entry *ipt;
    char *buf;
    ssize_t size;

    // XXX: TODO: determine size of buffer needed
    // by doing a first pass through the XORP
    // intermediate ruleset.
#ifdef notyet
    tsize = table.size() * sizeof(struct ipt_entry);
    ipt = malloc(tsize);
    if (ipt == NULL) {
	XLOG_ERROR("malloc: %s", strerror(errno));
	return (XORP_ERROR);
    }
#endif

    ret_value = XORP_ERROR;

    // Transcribe all the rules contained within the snapshot
    // into Netfilter format.
    pipt = ipt;
    for (PaSnapTable4::const_iterator i = table.begin();
	 i != table.end(); i++) {
	ret_value = transcribe_rule4(*i, pipt);
	if (ret_value != XORP_OK)
	    break;
	++pipt;
    }

    // Push the new ruleset to Netfilter.
    ret_value = push_rules4(tsize, table.size(), ipt);

    return (ret_value);
#endif
}

// Clear all entries by committing an empty transaction.
int
PaNfBackend::delete_all_entries4()
{
#ifndef HAVE_PACKETFILTER_NF
    return (XORP_ERROR);
#else
    return (push_rules4(0, 0, NULL));
#endif
}

const PaBackend::Snapshot4Base*
PaNfBackend::create_snapshot4()
{
#ifndef HAVE_PACKETFILTER_NF
    return (NULL);
#else

    struct ipt_getinfo igi;
    socklen_t size;

    //
    // Determine the size of buffer and number of firewall rules
    // contained within the buffer.
    //
    strncpy(igi.name, "FILTER", IPT_TABLE_MAXNAMELEN);
    if (getsockopt(_fd4, TC_IPPROTO, SO_GET_INFO, &igi, &size) < 0) {
        XLOG_ERROR("Error retrieving Netfilter info: getsockopt "
		   "SO_GET_INFO: %s", strerror(errno));
	throw PaInvalidBackendException();
    }

    // Deal with num_entries == 0 quickly by creating an empty snapshot.
    if (igi.num_entries == 0) {
	PaNfBackend::Snapshot4* snap = new PaNfBackend::Snapshot4(*this);
	return (snap);
    }

    struct ipt_get_entries *ige;
    struct ipt_entry	   *ipt;
    unsigned int	    bufsize;

    bufsize = sizeof(struct ipt_get_entries) + igi.size;
    ige = malloc(bufsize);
    if (ige == NULL) {
	XLOG_ERROR("Cannot allocate NF rule state: malloc: %s",
		   strerror(errno));
	return (NULL);
    }

    //
    // Request the firewall rule list from the kernel.
    //
    strncpy(ige->name, "FILTER", IPT_TABLE_MAXNAMELEN);
    if (getsockopt(sockfd, TC_IPPROTO, SO_GET_ENTRIES, ige, &bufsize) < 0) {
	XLOG_ERROR("Cannot snapshot NF rule state: SO_GET_ENTRIES: %s",
		   strerror(errno));
	return (NULL);
    }

    // Create a snapshot of the current NF rule set using the
    // list we just extracted from the kernel. The snapshot wraps
    // the list we just allocated in the malloc() heap, so
    // do not free the pointer.
    PaNfBackend::Snapshot4* snap = new PaNfBackend::Snapshot4(*this,
							      igi.num_entries,
							      ige);

    return (snap);
#endif
}

int
PaNfBackend::restore_snapshot4(const PaBackend::Snapshot4Base* snap4)
{
#ifndef HAVE_PACKETFILTER_NF
    UNUSED(snap4);
    return (XORP_ERROR);
#else
    // Simply check if the snapshot is an instance of our derived snapshot.
    // If it is not, we received it in error, and it is of no interest to us.
    const PaNfBackend::Snapshot4* dsnap4 =
	dynamic_cast<const PaNfBackend::Snapshot4*>(snap4);
    XLOG_ASSERT(dsnap4 != NULL);

    // Push entire ruleset back to the kernel.
    return (push_rules4(dsnap4->_rulebuf->size,
    			dsnap4->_rulebuf->num_entries,
			dsnap4->_rulebuf->entrytable));
#endif
}

#ifdef HAVE_PACKETFILTER_NF
/* ------------------------------------------------------------------------- */
/* Private utility methods used for manipulating NF itself */

// Push a ruleset to the kernel, specified as a buffer containing
// a vector of variable-sized ipt_entry elements.
int
PaNfBackend::push_rules4(unsigned int size, unsigned int num_entries,
			 struct ipt_entry *rules)
{
    struct ipt_replace *ipr;
    unsigned int rsize;
    int ret_value;
    //
    // Allocate buffer for the ipt_replace message, fill
    // it out, and copy the rule list vector into the message.
    // If the caller didn't specify any rules, don't fill
    // them out -- assume caller is committing an empty list
    // to wipe out all firewall rules.
    //
    rsize = sizeof(*ipr);
    if (rules)
	rsize += size;
    ipr = malloc(rsize);
    if (ipr == NULL) {
        XLOG_ERROR("malloc: %s", strerror(errno));
        return (XORP_ERROR);
    }
    memset(ipr, 0, sizeof(ipr));
    strncpy(ipr->name, "FILTER", IPT_TABLE_MAXNAMELEN);
    if (rules) {
	memcpy(ipr->entries, rules, size);
        ipr->num_entries = num_entries;
	ipr->size = size;
        ipr->valid_hooks = NF_IP_FORWARD;
	ipr->hook_entry[NF_IP_FORWARD] = 0;
    }

    ret_value = XORP_OK;

    if (setsockopt(_fd4, IPPROTO_IP, IPT_SO_SET_REPLACE, ipr, rsize) < 0) {
	XLOG_ERROR("Failed to commit transaction for adding rules"
		   "to NF firewall: setsockopt SO_SET_REPLACE: %s",
		   strerror(errno));
	ret_value = XORP_ERROR;
    }

    free(ipr);
    return (ret_value);
}

// Return 0 on failure, otherwise, size of rule required or transcribed.
ssize_t
PaNfBackend::transcribe_rule4(const PaEntry4& entry, struct ipt_entry *buf,
			      ssize_t remaining)
{
    //
    // Determine required size of the transcribed rule without filling
    // it out. We can use this as a sanity check.
    //
    ssize_t needed = sizeof(struct ipt_entry) +
	sizeof(struct ipt_standard_target);
    if (ipt->ip_proto == IPPROTO_TCP || ipt->ip_proto == IPPROTO_UDP)
	needed += sizeof(struct ipt_entry_match); // XXX
    //
    // If no buffer was passed to us to fill out, return the
    // required size of the rule.
    //
    if (buf == NULL)
	return (needed);
    if (remaining < needed)
	return (0);

    struct ipt_entry *ipt;
    struct ipt_entry_match *iem;
    struct ipt_standard_target *ist;

    ipt = (struct ipt_entry *)buf;
    memset(ipt, 0, needed);

    entry.src().masked_addr().copy_out(ipt->ip.src);
    entry.src().netmask().copy_out(ipt->ip.smsk);
    entry.dst().masked_addr().copy_out(ipt->ip.dst);
    entry.dst().netmask().copy_out(ipt->ip.dmsk);

    strncpy(ipt->ip.iniface, entry.ifname().c_str(), IFNAMSIZ);
    strncpy(ipt->ip.outiface, entry.ifname().c_str(), IFNAMSIZ);

    // XXX: I don't understand what iniface_mask and outiface_mask are
    // for, therefore I do not fill them out.
    // They could be for IP aliases (ethM:N).

    ipt->ip.proto = entry.proto() == IP_PROTO_ANY ? 0 : entry.proto();

    // implied by memset:
    //ipt->ip.flags = 0;
    //ipt->ip.invflags = 0;

#ifdef notyet
    // Fill out an additional match structure for tcp/udp ports.
    if (ipt->ip_proto == IPPROTO_TCP || ipt->ip_proto == IPPROTO_UDP) {
        struct ipt_entry_match *ipe;
	//
	// XXX: TODO: Fill out the optional 'port match' portion of the
	// firewall rule.
	//
    }
#endif

    // Fill out an IPTABLES standard target (accept or drop).
    // Standard targets are named by the empty string.
    strncpy(ist->target.u.user.name, IPT_STANDARD_TARGET, FUNCTION_MAXNAMELEN);
    if (entry.action() == PaAction(ACTION_PASS)) {
        ist->verdict = NF_ACCEPT;
    } else if (entry.action() == PaAction(ACTION_DROP)) {
        ist->verdict = NF_DROP;
    } else {
        XLOG_UNREACHABLE();
    }

    // XXX: TODO: Specify that this is for the FORWARD chain.
    // XXX: TODO: Fixup ipt->next_offset to point to next match.
    // XXX: TODO: Fixup ipt->target_offset to point to target.

    return (needed);
}

/* ------------------------------------------------------------------------- */
/* Snapshot scoped classes (Memento pattern) */

// Cannot be copied or assigned from base class.
PaNfBackend::Snapshot4::Snapshot4(const PaBackend::Snapshot4Base& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(snap4),
      _parent(NULL), _num_entries(0), _rulebuf(NULL)
{
    throw PaInvalidSnapshotException();
}

// May be copied or assigned from own class, but
// requires deep copy of rule buffer.
// The internal representation is simply the buffer returned
// by invoking the getsockopt() IPT_SO_GET_ENTRIES socket option,
// plus the number of entries returned by IPT_SO_GET_INFO.
PaNfBackend::Snapshot4::Snapshot4(const PaNfBackend::Snapshot4& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(snap4),
      _parent(snap4._parent)
{
    if (snap4._rulebuf == NULL) {
	_rulebuf = NULL;
    } else {
	ssize_t size = sizeof(*ige) + snap4._rulebuf->size;
	struct ipt_get_entries *ige = malloc(size);
	if (ige == NULL)
	    throw PaInvalidSnapshotException();
	memcpy(ige, &snap4._rulebuf, size);
	_rulebuf = ige;
	_num_entries = snap4._num_entries;
    }
}

// This constructor is marked private and used internally
// by the parent class, to attach an existing buffer.
PaNfBackend::Snapshot4::Snapshot4(PaNfBackend& parent,
				  unsigned int num_entries,
				  struct ipt_get_entries *rulebuf)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(),
      _parent(&parent), _num_entries(num_entries), _rulebuf(rulebuf)
{
}

// Deep free.
PaNfBackend::Snapshot4::~Snapshot4()
{
    if (_rulebuf != NULL)
	free(_rulebuf);
}

/* ------------------------------------------------------------------------- */
#endif // HAVE_PACKETFILTER_NF
