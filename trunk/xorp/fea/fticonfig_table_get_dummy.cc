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

#ident "$XORP: xorp/fea/fticonfig_table_get_sysctl.cc,v 1.1 2003/05/02 07:50:45 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is dummy (for testing purpose).
//


FtiConfigTableGetDummy::FtiConfigTableGetDummy(FtiConfig& ftic)
    : FtiConfigTableGet(ftic)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ftic();
#endif
}

FtiConfigTableGetDummy::~FtiConfigTableGetDummy()
{
    stop();
}

int
FtiConfigTableGetDummy::start()
{
    return (XORP_OK);
}
    
int
FtiConfigTableGetDummy::stop()
{
    return (XORP_OK);
}

void
FtiConfigTableGetDummy::receive_data(const uint8_t* data, size_t n_bytes)
{
    // TODO: use it if needed
    UNUSED(data);
    UNUSED(n_bytes);
}

int
FtiConfigTableGetDummy::get_table4(list<Fte4>& fte_list)
{
    Trie4::iterator ti;
    for (ti = ftic().trie4().begin(); ti != ftic().trie4().end(); ++ti) {
	const Fte4& fte = ti.payload();
	fte_list.push_back(fte);
    }
    
    return (XORP_OK);
}

int
FtiConfigTableGetDummy::get_table6(list<Fte6>& fte_list)
{
    Trie6::iterator ti;
    for (ti = ftic().trie6().begin(); ti != ftic().trie6().end(); ++ti) {
	const Fte6& fte = ti.payload();
	fte_list.push_back(fte);
    }
    
    return (XORP_OK);
}
