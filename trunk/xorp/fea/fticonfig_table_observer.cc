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

#ident "$XORP: xorp/fea/fticonfig_table_observer.cc,v 1.7 2004/10/21 00:10:25 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_observer.hh"


//
// Observe whole-table information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would NOT specify the particular entry that
// has changed.
//


FtiConfigTableObserver::FtiConfigTableObserver(FtiConfig& ftic)
    : _is_running(false),
      _ftic(ftic),
      _is_primary(true)
{
    
}

FtiConfigTableObserver::~FtiConfigTableObserver()
{
    
}

void
FtiConfigTableObserver::register_ftic_primary()
{
    _ftic.register_ftic_table_observer_primary(this);
}

void
FtiConfigTableObserver::register_ftic_secondary()
{
    _ftic.register_ftic_table_observer_secondary(this);
}

/**
 * Add a FIB table observer.
 * 
 * @param fib_table_observer the FIB table observer to add.
 */
void
FtiConfigTableObserver::add_fib_table_observer(
    FibTableObserverBase* fib_table_observer)
{
    if (find(_fib_table_observers.begin(),
	     _fib_table_observers.end(),
	     fib_table_observer)
	!= _fib_table_observers.end()) {
	return;		// XXX: we have already added that observer
    }

    _fib_table_observers.push_back(fib_table_observer);
}

/**
 * Delete a FIB table observer.
 * 
 * @param fib_table_observer the FIB table observer to delete.
 */
void
FtiConfigTableObserver::delete_fib_table_observer(
    FibTableObserverBase* fib_table_observer)
{
    list<FibTableObserverBase* >::iterator iter;

    iter = find(_fib_table_observers.begin(),
		_fib_table_observers.end(),
		fib_table_observer);
    if (iter != _fib_table_observers.end())
	_fib_table_observers.erase(iter);
}

/**
 * Propagate FIB changes to all FIB table observers.
 * 
 * @param fte_list the list with the FIB changes.
 */
void
FtiConfigTableObserver::propagate_fib_changes(const list<FteX>& fte_list)
{
    list<Fte4> fte_list4;
    list<Fte6> fte_list6;
    list<FteX>::const_iterator ftex_iter;

    if (fte_list.empty())
	return;

    // Copy the FteX list into Fte4 and Fte6 lists
    for (ftex_iter = fte_list.begin();
	 ftex_iter != fte_list.end();
	 ++ftex_iter) {
	const FteX& ftex = *ftex_iter;
	if (ftex.net().is_ipv4()) {
	    // IPv4 entry
	    Fte4 fte4 = Fte4(ftex.net().get_ipv4net(),
			     ftex.nexthop().get_ipv4(),
			     ftex.ifname(),
			     ftex.vifname(),
			     ftex.metric(),
			     ftex.admin_distance(),
			     ftex.xorp_route());
	    if (ftex.is_deleted())
		fte4.mark_deleted();
	    if (ftex.is_unresolved())
		fte4.mark_unresolved();
	    fte_list4.push_back(fte4);
	}

	if (ftex.net().is_ipv6()) {
	    // IPv6 entry
	    Fte6 fte6 = Fte6(ftex.net().get_ipv6net(),
			     ftex.nexthop().get_ipv6(),
			     ftex.ifname(),
			     ftex.vifname(),
			     ftex.metric(),
			     ftex.admin_distance(),
			     ftex.xorp_route());
	    if (ftex.is_deleted())
		fte6.mark_deleted();
	    if (ftex.is_unresolved())
		fte6.mark_unresolved();
	    fte_list6.push_back(fte6);
	}
    }

    // Inform all observers about the changes
    list<FibTableObserverBase* >::iterator iter;
    for (iter = _fib_table_observers.begin();
	 iter != _fib_table_observers.end();
	 ++iter) {
	FibTableObserverBase* fib_table_observer = *iter;
	if (! fte_list4.empty())
	    fib_table_observer->process_fib_changes(fte_list4);
	if (! fte_list6.empty())
	    fib_table_observer->process_fib_changes(fte_list6);
    }
}
