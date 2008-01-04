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

#ident "$XORP: xorp/fea/pa_table.cc,v 1.6 2007/09/15 19:52:40 pavlin Exp $"

#include <vector>

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "pa_entry.hh"
#include "pa_table.hh"

/* --------------------------------------------------------------------- */
/* Constructor/destructor */

PaTableManager::PaTableManager()
{
}

PaTableManager::~PaTableManager()
{
}

/* --------------------------------------------------------------------- */
/* IPv4 ACL management */

/*
 * Add an IPv4 ACL entry to XORP's tables.
 */
int
PaTableManager::add_entry4(const PaEntry4& entry)
{
    _pal4.push_back(entry);
    return (XORP_OK);
}

/*
 * Delete an IPv4 ACL entry from XORP's tables.
 * Note that only the rule-match part of the rule is compared,
 * i.e. all fields except the rule action.
 */
int
PaTableManager::delete_entry4(const PaEntry4& entry)
{
    for (PaTable4::iterator ti = _pal4.begin(); ti != _pal4.end(); ti++) {
	if (entry.match(*ti)) {
	    (void)_pal4.erase(ti);
	    return (XORP_OK);
	}
    }
    return (XORP_ERROR);
}

/*
 * Delete an IPv4 ACL entry from XORP's tables.
 */
int
PaTableManager::delete_all_entries4()
{
    _pal4.clear();
    return (XORP_OK);
}

/*
 * Create a snapshot of XORP's current IPv4 ACLs.
 */
const PaSnapshot4*
PaTableManager::create_snapshot4() const
{
    return (new PaSnapshot4(*this));
}

/*
 * Restore XORP's current IPv6 ACLs from a previous snapshot.
 */
int
PaTableManager::restore_snapshot4(const PaSnapshot4* snap)
{
    return (snap->restore(*this));
}

PaSnapshot4::PaSnapshot4(const PaTableManager& parent)
{
    this->_pal4 = parent._pal4;
}

int
PaSnapshot4::restore(PaTableManager& parent) const
{
    // Assignment from vector<PaEntry4> to same type
    // will DTRT according to STL Container model documentation,
    // i.e. make a complete identical copy.
    parent._pal4 = this->_pal4;
    return (XORP_OK);
}

/* --------------------------------------------------------------------- */
/* IPv6 ACL management */

/*
 * Add an IPv6 ACL entry to XORP's tables.
 */
int
PaTableManager::add_entry6(const PaEntry6& entry)
{
#ifdef notyet
    _pal6.push_back(entry);
    return (XORP_OK);
#else
    UNUSED(entry);
    return (XORP_ERROR);
#endif
}

/*
 * Delete an IPv6 ACL entry from XORP's tables.
 */
int
PaTableManager::delete_entry6(const PaEntry6& entry)
{
#ifdef notyet
    for (PaTable6::iterator ti = _pal6.begin(); ti != _pal6.end(); ti++) {
	if (entry.match(*ti)) {
	    (void)_pal6.erase(ti);
	    return (XORP_OK);
	}
    }
    return (XORP_ERROR);
#else
    UNUSED(entry);
    return (XORP_ERROR);
#endif
}

/*
 * Delete an IPv6 ACL entry from XORP's tables.
 */
int
PaTableManager::delete_all_entries6()
{
#ifdef notyet
    _pal6.clear();
    return (XORP_OK);
#else
    return (XORP_ERROR);
#endif
}

/*
 * Create a snapshot of XORP's current IPv6 ACLs.
 */
const PaSnapshot6*
PaTableManager::create_snapshot6() const
{
#ifdef notyet
    return (new PaSnapshot6(*this));
#else
    return NULL;
#endif
}

/*
 * Restore XORP's current IPv6 ACLs from a previous snapshot.
 */
int
PaTableManager::restore_snapshot6(const PaSnapshot6* snap)
{
#ifdef notyet
    return (snap->restore(*this));
#else
    UNUSED(snap);
    return (XORP_ERROR);
#endif
}

PaSnapshot6::PaSnapshot6(const PaTableManager& parent)
{
#ifdef notyet
    this->_pal6 = parent._pal6;
#else
    UNUSED(parent);
#endif
}

int
PaSnapshot6::restore(PaTableManager& parent) const
{
#ifdef notyet
    parent._pal6 = this->_pal6;
    return (XORP_OK);
#else
    UNUSED(parent);
    return (XORP_ERROR);
#endif
}
