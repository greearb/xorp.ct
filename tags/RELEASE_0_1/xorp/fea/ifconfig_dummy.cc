// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_dummy.cc,v 1.30 2002/12/09 18:28:57 hodson Exp $"

#include "iftree.hh"
#include "ifconfig.hh"
#include "ifconfig_dummy.hh"

bool
IfConfigDummy::push_config(const IfTree& config)
{
    report_updates(config);
    _config = config;

    return true;
}

const IfTree&
IfConfigDummy::pull_config(IfTree&)
{
    return _config;
}
