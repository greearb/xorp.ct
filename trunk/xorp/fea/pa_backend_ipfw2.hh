// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/fea/pa_backend_ipfw2.hh,v 1.5 2007/02/16 22:45:48 pavlin Exp $

#ifndef __FEA_PA_BACKEND_IPFW2_HH__
#define __FEA_PA_BACKEND_IPFW2_HH__

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"

#include <map>
#include <bitset>

/* ------------------------------------------------------------------------- */

/**
 * @short IPFW2 ACL backend interface.
 *
 * Concrete class defining a backend which drives IPFW2 on FreeBSD.
 */
class PaIpfw2Backend : public PaBackend {
    friend class Snapshot4;
public:
    PaIpfw2Backend() throw(PaInvalidBackendException);
    virtual ~PaIpfw2Backend();

#ifdef HAVE_PACKETFILTER_IPFW2
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
	friend class PaIpfw2Backend;
    public:
	Snapshot4(const Snapshot4& snap4)
	    throw(PaInvalidSnapshotException);
	Snapshot4(const PaBackend::Snapshot4Base& snap4)
	    throw(PaInvalidSnapshotException);
	virtual ~Snapshot4();
	uint8_t get_ruleset() const { return _ruleset; }
    private:
	Snapshot4(PaIpfw2Backend& parent, uint8_t ruleset)
	    throw(PaInvalidSnapshotException);

	PaIpfw2Backend*	_parent;
	uint8_t		_ruleset;
    };

    // Ruleset number constants. 31 is reserved for 'default rules' and
    // cannot have rules added to it, nor can it be enabled/disabled.
    enum { DEFAULT_RULESET = 0, TRANSCRIPT_RULESET = 1,
	   RESERVED_RULESET = 31, MAX_RULESETS = 32 };
    enum { MAX_IPFW2_RULE_WORDS = 255, IPFW_RULENUM_MAX = 65535 };

    // Enumeration defining encapsulated commands used to overload the
    // IP_FW_DEL socket option. Believe it or not these aren't defined
    // as constants anywhere in FreeBSD!!
    // Bits 24-31 are command opcode.
    // Bits 16-23 are destination ruleset index.
    // Bits 0-15 are source rule *or* source ruleset index.
    enum {
	CMD_DEL_RULE = 0,		// Delete a single rule
	CMD_DEL_RULES_WITH_SET = 1,	// Delete all rules in a set
	CMD_MOVE_RULE = 2,		// Move 1 rule to a ruleset
	CMD_MOVE_RULESET = 3,		// Move all rules in a set to new set
	CMD_SWAP_RULESETS = 4		// Swap two rulesets
    };

    // Types used for directly manipulating kernel rule tables.
    typedef vector<uint32_t> RuleBuf;
    typedef map<uint16_t, RuleBuf> RulesetDB;

    // IPv4 state snapshots are indexed by their IPFW2 'set number'.
    // These make it possible to freeze current state and copy them
    // to a free set which can then be manipulated independently
    // of the active set. IPFW2 allows more than one set to be active
    // at any given time, but XORP does not support this functionality.
    // XXX: This is a candidate for a simple array.
    typedef map<uint8_t, Snapshot4* >	Snapshot4DB;
    typedef bitset<MAX_RULESETS>	RulesetGroup;

#endif // HAVE_PACKETFILTER_IPFW2

public:
    /* --------------------------------------------------------------------- */
    /* General back-end methods. */

    const char* get_name() const;
    const char* get_version() const;

    /* --------------------------------------------------------------------- */
    /* IPv4 ACL back-end methods. */

    bool push_entries4(const PaSnapshot4* snap);
    bool delete_all_entries4();
    const PaBackend::Snapshot4Base* create_snapshot4();
    bool restore_snapshot4(const PaBackend::Snapshot4Base* snap);

#ifdef notyet
    /* --------------------------------------------------------------------- */
    /* IPv6 ACL back-end methods. */

    bool push_entries6(const PaSnapshot6* snap);
    bool delete_all_entries6();
    const PaBackend::Snapshot6Base* create_snapshot6() const;
    bool restore_snapshot6(const PaBackend::Snapshot6Base* snap);
#endif

#ifdef HAVE_PACKETFILTER_IPFW2
protected:
    /* --------------------------------------------------------------------- */
    /* Private back-end methods. */
    static bool get_autoinc_step(uint32_t& step);
    static bool set_autoinc_step(const uint32_t& step);

    int docmd4(int optname, void *optval, socklen_t optlen);

    int enable_disable_rulesets4(RulesetGroup& enable_group,
                                 RulesetGroup& disable_group);
    int enable_ruleset4(int index);
    int disable_ruleset4(int index);
    int move_ruleset4(int src_index, int dst_index);
    int swap_ruleset4(int first_index, int second_index);
    int flush_ruleset4(int index);
    int add_rule4(const int ruleset_index, const PaEntry4& entry);
    void copy_ruleset4(int src_index, int dst_index);
    int read_ruleset4(const int ruleset_index, RulesetDB& rulesetdb);
    void renumber_ruleset4(const int ruleset_index, RulesetDB& rulesetdb);
    int push_rulesetdb4(RulesetDB& rulesetdb);
    void transcribe_rule4(const PaEntry4& entry,
			  const int ruleset_index,
			  uint32_t rulebuf[MAX_IPFW2_RULE_WORDS],
			  uint32_t& size_used);
    int push_rule4(const int ruleset_index, uint32_t rulebuf[],
		   const uint32_t size_used);

    // XXX: For some reason this access isn't permitted even though
    // Snapshot4 is a friend of PaIpfw2Backend in this scope, which
    // makes no sense (under g++ 2.95).
public:
    Snapshot4** get_snapshotdb() { return _snapshot4db; }
#endif // HAVE_PACKETFILTER_IPFW2

protected:
    /* --------------------------------------------------------------------- */
#ifdef HAVE_PACKETFILTER_IPFW2
    // Holds mapping of IPFW2 state snapshots to set numbers.
    Snapshot4*		_snapshot4db[MAX_RULESETS];
#endif
    // Private IPv4 raw socket for talking to kernel.
    int			_s4;
};

/* ------------------------------------------------------------------------- */

#endif // __FEA_PA_BACKEND_IPFW2_HH__
