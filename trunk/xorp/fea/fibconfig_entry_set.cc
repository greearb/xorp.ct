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

#ident "$XORP: xorp/fea/fibconfig_entry_set.cc,v 1.1 2007/04/26 22:29:50 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fibconfig.hh"
#include "fibconfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//


FibConfigEntrySet::FibConfigEntrySet(FibConfig& fibconfig)
    : _is_running(false),
      _fibconfig(fibconfig),
      _in_configuration(false),
      _is_primary(true)
{
    
}

FibConfigEntrySet::~FibConfigEntrySet()
{
    
}

void
FibConfigEntrySet::register_fibconfig_primary()
{
    _fibconfig.register_fibconfig_entry_set_primary(this);
}

void
FibConfigEntrySet::register_fibconfig_secondary()
{
    _fibconfig.register_fibconfig_entry_set_secondary(this);

    //
    // XXX: push the current config into the new secondary
    //
    if (_is_running) {
	// XXX: nothing to do. 
	//
	// XXX: note that the forwarding table state in the secondary methods
	// is pushed by FibConfigTableSet::register_fibconfig_secondary(),
	// hence we don't need to do it here again. However, this is based on
	// the assumption that for each FibConfigEntrySet secondary method
	// there is a corresponding FibConfigTableSet secondary method.
	//
    }
}
