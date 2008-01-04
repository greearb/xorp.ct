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

// $XORP: xorp/fea/pa_backend_pf.hh,v 1.6 2007/09/15 21:37:38 pavlin Exp $

#ifndef __FEA_PA_BACKEND_PF_HH__
#define __FEA_PA_BACKEND_PF_HH__

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"

#include <map>
#include <bitset>

/* ------------------------------------------------------------------------- */

/**
 * @short PF ACL backend interface.
 *
 * Concrete class defining a backend which drives PF on [Free|Open|Net]BSD.
 *
 * PF is closer to IPF than IPFW2 in that it has a single inactive ruleset
 * which is used to install rules before being swapped in by its own
 * commit command.
 */
class PaPfBackend : public PaBackend {
    friend class Snapshot4;
public:
    PaPfBackend() throw(PaInvalidBackendException);
    virtual ~PaPfBackend();

#ifdef HAVE_PACKETFILTER_PF
protected:
    /* --------------------------------------------------------------------- */
    /*
     * @short State snapshot Memento classes.
     *
     * These are provider-specific and abstract. Attempting to instantiate
     * them directly will result in an exception being thrown.
     *
     * Be warned that they might not actually copy all the state in a form
     * which can be marshaled elsewhere. Each provider must implement
     * both of these classes and override the virtuals, and check that
     * snapshots passed to it are its own by using dynamic casts.
     */
    class Snapshot4 : public PaBackend::Snapshot4Base {
	friend class PaPfBackend;
    public:
	Snapshot4(const Snapshot4& snap4)
	    throw(PaInvalidSnapshotException);
	Snapshot4(const PaBackend::Snapshot4Base& snap4)
	    throw(PaInvalidSnapshotException);
	virtual ~Snapshot4();
	uint8_t get_ruleset() const { return _ruleset; }
    private:
	Snapshot4(PaPfBackend& parent, uint8_t ruleset)
	    throw(PaInvalidSnapshotException);

	PaPfBackend*	    _parent;
	int		    _nrules;
	struct pfioc_rule*  _rulebuf;
    };

    // Types used for directly manipulating kernel rule tables.
    typedef vector<uint32_t> RuleBuf;
    typedef map<uint16_t, RuleBuf> RulesetDB;

    // IPv4 state snapshots are indexed by their PF 'set number'.
    // These make it possible to freeze current state and copy them
    // to a free set which can then be manipulated independently
    // of the active set. PF allows more than one set to be active
    // at any given time, but XORP does not support this functionality.
    // XXX: This is a candidate for a simple array.
    typedef map<uint8_t, Snapshot4* >	Snapshot4DB;
    typedef bitset<MAX_RULESETS>	RulesetGroup;

#endif // HAVE_PACKETFILTER_PF

public:
    /* --------------------------------------------------------------------- */
    /* General back-end methods. */

    const char* get_name() const;
    const char* get_version() const;

    /* --------------------------------------------------------------------- */
    /* IPv4 ACL back-end methods. */

    int push_entries4(const PaSnapshot4* snap);
    int delete_all_entries4();
    const PaBackend::Snapshot4Base* create_snapshot4();
    int restore_snapshot4(const PaBackend::Snapshot4Base* snap);

#ifdef notyet
    /* --------------------------------------------------------------------- */
    /* IPv6 ACL back-end methods. */

    int push_entries6(const PaSnapshot6* snap);
    int delete_all_entries6();
    const PaBackend::Snapshot6Base* create_snapshot6() const;
    int restore_snapshot6(const PaBackend::Snapshot6Base* snap);
#endif

#ifdef HAVE_PACKETFILTER_PF
protected:
    /* --------------------------------------------------------------------- */
    /* Private back-end methods. */
    int set_pf_enabled(bool enable);
    uint32_t start_transaction();
    void abort_transaction(uint32_t ticket);
    int commit_transaction(uint32_t ticket);
    int transcribe_and_add_rule4(const PaEntry4& entry, uint32_t ticket);

    // XXX: For some reason this access isn't permitted even though
    // Snapshot4 is a friend of PaPfBackend in this scope, which
    // makes no sense (under g++ 2.95).
public:
    Snapshot4** get_snapshotdb() { return _snapshot4db; }
#endif // HAVE_PACKETFILTER_PF

protected:
    /* --------------------------------------------------------------------- */
#ifdef HAVE_PACKETFILTER_PF
    // Holds mapping of PF state snapshots to set numbers.
    Snapshot4*		_snapshot4db[MAX_RULESETS];
    // Open file descriptor pointing to the /dev/pf device.
    int			_fd;
private:
    static const char *_pfname;
#endif
};

/* ------------------------------------------------------------------------- */

#endif // __FEA_PA_BACKEND_PF_HH__
