// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_table_set_dummy.cc,v 1.5 2004/08/17 02:20:09 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_set.hh"


//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to set the information is dummy (for testing purpose).
//


FtiConfigTableSetDummy::FtiConfigTableSetDummy(FtiConfig& ftic)
    : FtiConfigTableSet(ftic)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ftic_primary();
#endif
}

FtiConfigTableSetDummy::~FtiConfigTableSetDummy()
{
    stop();
}

int
FtiConfigTableSetDummy::start()
{
    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}
    
int
FtiConfigTableSetDummy::stop()
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigTableSetDummy::set_table4(const list<Fte4>& fte_list)
{
    list<Fte4>::const_iterator iter;

    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	ftic().add_entry4(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetDummy::delete_all_entries4()
{
    if (in_configuration() == false)
	return false;
    
    ftic().trie4().delete_all_nodes();
    
    return true;
}

bool
FtiConfigTableSetDummy::set_table6(const list<Fte6>& fte_list)
{
    list<Fte6>::const_iterator iter;
    
    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	ftic().add_entry6(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetDummy::delete_all_entries6()
{
    if (in_configuration() == false)
	return false;
    
    ftic().trie6().delete_all_nodes();
    
    return true;
}
