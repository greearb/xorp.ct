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

#ident "$XORP: xorp/pim/pim_config.cc,v 1.3 2003/01/13 20:11:21 pavlin Exp $"


//
// TODO: a temporary solution for various MFEA configuration
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_node.hh"


int
MfeaNode::set_mrib_table_default_metric_preference(uint32_t metric_preference)
{
    mrib_table_default_metric_preference().set(metric_preference);
    
    return (XORP_OK);
}

int
MfeaNode::reset_mrib_table_default_metric_preference()
{
    mrib_table_default_metric_preference().reset();
    
    return (XORP_OK);
}

int
MfeaNode::set_mrib_table_default_metric(uint32_t metric)
{
    mrib_table_default_metric().set(metric);
    
    return (XORP_OK);
}

int
MfeaNode::reset_mrib_table_default_metric()
{
    mrib_table_default_metric().reset();
    
    return (XORP_OK);
}
