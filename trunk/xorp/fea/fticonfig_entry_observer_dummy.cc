// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_entry_observer_dummy.cc,v 1.1 2003/05/10 00:06:39 pavlin Exp $"


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
// The mechanism to set the information is dummy (for testing purpose).
//


FtiConfigEntryObserverDummy::FtiConfigEntryObserverDummy(FtiConfig& ftic)
    : FtiConfigEntryObserver(ftic)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ftic();
#endif
}

FtiConfigEntryObserverDummy::~FtiConfigEntryObserverDummy()
{
    stop();
}

int
FtiConfigEntryObserverDummy::start()
{
    // TODO: XXX: PAVPAVPAV: implement it!
    
    return (XORP_OK);
}
    
int
FtiConfigEntryObserverDummy::stop()
{
    // TODO: XXX: PAVPAVPAV: implement it!
    
    return (XORP_OK);
}

void
FtiConfigEntryObserverDummy::receive_data(const uint8_t* data, size_t nbytes)
{
    // TODO: use it?
    UNUSED(data);
    UNUSED(nbytes);
}

