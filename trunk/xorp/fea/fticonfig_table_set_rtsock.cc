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

#ident "$XORP: xorp/fea/fticonfig_table_set_rtsock.cc,v 1.5 2004/06/02 22:52:38 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_set.hh"


//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to set the information is routing sockets.
//


FtiConfigTableSetRtsock::FtiConfigTableSetRtsock(FtiConfig& ftic)
    : FtiConfigTableSet(ftic)
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic();
#endif
}

FtiConfigTableSetRtsock::~FtiConfigTableSetRtsock()
{
    stop();
}

int
FtiConfigTableSetRtsock::start()
{
    delete_all_entries4();
    delete_all_entries6();

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigTableSetRtsock::stop()
{
    if (! _is_running)
	return (XORP_OK);

    delete_all_entries4();
    delete_all_entries6();

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigTableSetRtsock::set_table4(const list<Fte4>& fte_list)
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
FtiConfigTableSetRtsock::delete_all_entries4()
{
    list<Fte4> fte_list;
    list<Fte4>::const_iterator iter;
    
    // Get the list of all entries
    ftic().get_table4(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	if (fte.xorp_route())
	    ftic().delete_entry4(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetRtsock::set_table6(const list<Fte6>& fte_list)
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
FtiConfigTableSetRtsock::delete_all_entries6()
{
    list<Fte6> fte_list;
    list<Fte6>::const_iterator iter;
    
    // Get the list of all entries
    ftic().get_table6(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	if (fte.xorp_route())
	    ftic().delete_entry6(fte);
    }
    
    return true;
}
