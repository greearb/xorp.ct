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

#ident "$XORP: xorp/pim/pim_config.cc,v 1.8 2003/03/10 23:20:47 hodson Exp $"


//
// TODO: a temporary solution for various MLD6IGMP configuration
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mld6igmp_node.hh"
#include "mld6igmp_vif.hh"


int
Mld6igmpNode::set_vif_proto_version(const string& vif_name, int proto_version)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL)
	return (XORP_ERROR);
    
    if (mld6igmp_vif->set_proto_version(proto_version) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
Mld6igmpNode::reset_vif_proto_version(const string& vif_name)
{
    Mld6igmpVif *mld6igmp_vif = vif_find_by_name(vif_name);
    
    if (mld6igmp_vif == NULL)
	return (XORP_ERROR);
    
    mld6igmp_vif->set_proto_version(mld6igmp_vif->proto_version_default());
    
    return (XORP_OK);
}
