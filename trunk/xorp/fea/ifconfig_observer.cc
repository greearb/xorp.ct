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

#ident "$XORP: xorp/fea/ifconfig_observer.cc,v 1.3 2004/06/10 22:40:52 hodson Exp $"


#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"

#include "ifconfig.hh"
#include "ifconfig_observer.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//


IfConfigObserver::IfConfigObserver(IfConfig& ifc)
    : _is_running(false),
      _ifc(ifc)
{
    
}

IfConfigObserver::~IfConfigObserver()
{
    
}

void
IfConfigObserver::register_ifc_primary()
{
    _ifc.register_ifc_observer_primary(this);
}
