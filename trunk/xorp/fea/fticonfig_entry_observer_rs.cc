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

#ident "$XORP: xorp/fea/ifconfig_rtsock.cc,v 1.5 2003/03/10 23:20:15 hodson Exp $"


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
// The mechanism to set the information is routing sockets.
//


FtiConfigEntryObserverRs::FtiConfigEntryObserverRs(FtiConfig& ftic)
    : FtiConfigEntryObserver(ftic),
      RoutingSocket(ftic.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic();
#endif
}

FtiConfigEntryObserverRs::~FtiConfigEntryObserverRs()
{
    stop();
}

int
FtiConfigEntryObserverRs::start()
{
    return (RoutingSocket::start());
}
    
int
FtiConfigEntryObserverRs::stop()
{
    return (RoutingSocket::stop());
}

void
FtiConfigEntryObserverRs::rtsock_data(const uint8_t* data, size_t nbytes)
{
    ftic().ftic_entry_get().receive_data(data, nbytes);
}
