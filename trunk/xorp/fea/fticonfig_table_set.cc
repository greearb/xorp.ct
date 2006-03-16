// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_table_set.cc,v 1.9 2005/03/25 02:53:04 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_set.hh"


//
// Set whole-table information into the unicast forwarding table.
//


FtiConfigTableSet::FtiConfigTableSet(FtiConfig& ftic)
    : _is_running(false),
      _ftic(ftic),
      _in_configuration(false),
      _is_primary(true)
{
    
}

FtiConfigTableSet::~FtiConfigTableSet()
{
    
}

void
FtiConfigTableSet::register_ftic_primary()
{
    _ftic.register_ftic_table_set_primary(this);
}

void
FtiConfigTableSet::register_ftic_secondary()
{
    _ftic.register_ftic_table_set_secondary(this);

    //
    // XXX: push the current config into the new secondary
    //
    if (_is_running) {
	list<Fte4> fte_list4;

	if (_ftic.get_table4(fte_list4) == true) {
	    if (set_table4(fte_list4) != true) {
		XLOG_ERROR("Cannot push the current IPv4 forwarding table "
			   "into a new secondary mechanism for setting the "
			   "forwarding table");
	    }
	}

#ifdef HAVE_IPV6
	list<Fte6> fte_list6;

	if (_ftic.get_table6(fte_list6) == true) {
	    if (set_table6(fte_list6) != true) {
		XLOG_ERROR("Cannot push the current IPv6 forwarding table "
			   "into a new secondary mechanism for setting the "
			   "forwarding table");
	    }
	}
#endif // HAVE_IPV6
    }
}
