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

// $XORP: xorp/fea/pa_table.hh,v 1.4 2007/02/16 22:45:49 pavlin Exp $

#ifndef __FEA_PA_TABLE_HH__
#define __FEA_PA_TABLE_HH__

#include <vector>
#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

#include "pa_entry.hh"

/* ------------------------------------------------------------------------- */

// Exceptions which can be thrown by PaTableManager and member classes.
class PaInvalidSnapshotException {};

// Forward declaration.
class PaTableManager;

// Representation of rule tables inside snapshots.
typedef vector<PaEntry4> PaSnapTable4;
typedef vector<PaEntry6> PaSnapTable6;

/*
 * Container classes for snapshots of packet filtering state.
 * This is transparent for the following reasons:
 *
 * 1. External entities may need to inspect their contents e.g. for
 *    dumping current ACLs.
 *
 * 2. Internal entities may also need to inspect their contents.
 *    An example would be PaBackend, further down in the subsystem's
 *    Chain of Responsibility, which needs to be able to translate
 *    the top-half's picture of packet ACLs into those of the back-end.
 *
 * Because the 'friend' attribute is not inheritable or transitive, the
 * contents of PaSnapshots need to be exposed to these clients also.
 *
 * Attempting to restore a snapshot after modifying the contents violates
 * the interface contract laid down here, so all accessors are const.
 *
 * Snapshots are implemented so that transactions on the ACLs may be rolled
 * back without knowledge of the internal representation. They may not be
 * marshaled elsewhere *directly*; they are only intended to be used with
 * the PaTableManager.
 *
 * Whilst modifying the 'parent' PaTableManager from within PaSnapshot may
 * seem odd, it avoids the use of additional friend statements, as
 * snapshots know about PaTableManager's internal ACL representation.
 */
class PaSnapshot4 {
public:
    PaSnapshot4(const PaTableManager& parent);
    bool restore(PaTableManager& parent) const;
    const PaSnapTable4& data() const { return _pal4; }
protected:
    PaSnapTable4	_pal4;
};

class PaSnapshot6 {
public:
    PaSnapshot6(const PaTableManager& parent);
    bool restore(PaTableManager& parent) const;
    const PaSnapTable6& data() const { return _pal6; }
protected:
    PaSnapTable6	_pal6;
};

/* --------------------------------------------------------------------- */

/*
 * Container class for XORP's packet filtering ACLs.
 *
 * All accesses internal to the FEA's ACL support go through this class.
 * This means that the internal representation of the tables are hidden
 * from internal clients inside the FEA.
 */
class PaTableManager {
protected:
    // PaTableManager's internal representation of ACL tables.
    //
    // This happens to be the same right now as what the Snapshots
    // expose. The copies only ever happen from within the Snapshots
    // themselves, so normally this is not a problem. If we need to
    // change either representation we can do the appropriate
    // insertion iterator magic there.
    typedef vector<PaEntry4> PaTable4;
#ifdef notyet
    typedef vector<PaEntry6> PaTable6;
#endif

    // Snapshot classes are able to see our internal representation.
    friend class PaSnapshot4;
#ifdef notyet
    friend class PaSnapshot6;
#endif

public:
    PaTableManager();
    virtual ~PaTableManager();

    /* --------------------------------------------------------------------- */
    /* IPv4 ACL management */

    /*
     * Add an IPv4 ACL entry to XORP's tables.
     */
    bool add_entry4(const PaEntry4& entry);

    /*
     * Delete an IPv4 ACL entry from XORP's tables.
     */
    bool delete_entry4(const PaEntry4& entry);

    /*
     * Delete an IPv4 ACL entry from XORP's tables.
     */
    bool delete_all_entries4();

    /*
     * Create a snapshot of XORP's current IPv4 ACLs.
     */
    const PaSnapshot4* create_snapshot4() const;

    /*
     * Restore XORP's current IPv4 ACLs from a previous snapshot.
     */
    bool restore_snapshot4(const PaSnapshot4* snap);

    /* --------------------------------------------------------------------- */
    /* IPv6 ACL management */

    /*
     * Add an IPv6 ACL entry to XORP's tables.
     */
    bool add_entry6(const PaEntry6& entry);

    /*
     * Delete an IPv6 ACL entry from XORP's tables.
     */
    bool delete_entry6(const PaEntry6& entry);

    /*
     * Delete an IPv6 ACL entry from XORP's tables.
     */
    bool delete_all_entries6();

    /*
     * Create a snapshot of XORP's current IPv6 ACLs.
     */
    const PaSnapshot6* create_snapshot6() const;

    /*
     * Restore XORP's current IPv6 ACLs from a previous snapshot.
     */
    bool restore_snapshot6(const PaSnapshot6* snap);

protected:
    PaTable4	_pal4;
#ifdef notyet
    PaTable6	_pal6;
#endif
};

/* ------------------------------------------------------------------------- */
#endif // __FEA_PA_TABLE_HH__
