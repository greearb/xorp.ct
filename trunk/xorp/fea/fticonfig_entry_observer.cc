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

#ident "$XORP: xorp/fea/fticonfig_entry_observer.cc,v 1.4 2004/08/17 02:20:06 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_entry_observer.hh"


//
// Observe single-entry information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would specify the particular entry that
// has changed.
//


FtiConfigEntryObserver::FtiConfigEntryObserver(FtiConfig& ftic)
    : _is_running(false),
      _ftic(ftic),
      _is_primary(true)
{
    
}

FtiConfigEntryObserver::~FtiConfigEntryObserver()
{
    
}

void
FtiConfigEntryObserver::register_ftic_primary()
{
    _ftic.register_ftic_entry_observer_primary(this);
}

void
FtiConfigEntryObserver::register_ftic_secondary()
{
    _ftic.register_ftic_entry_observer_secondary(this);
}
