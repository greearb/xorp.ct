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

#ident "$XORP: xorp/mld6igmp/mld6igmp_group_record.cc,v 1.1 2006/06/07 00:01:54 pavlin Exp $"

//
// Multicast source record information used by IGMPv3 (RFC 3376) and
// MLDv2 (RFC 3810).
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mld6igmp_source_record.hh"
#include "mld6igmp_group_record.hh"



//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


/**
 * Mld6igmpSourceRecord::Mld6igmpSourceRecord:
 * @group_record: The group record this entry belongs to.
 * @source: The entry source address.
 * 
 * Return value: 
 **/
Mld6igmpSourceRecord::Mld6igmpSourceRecord(Mld6igmpGroupRecord& group_record,
					   const IPvX& source)
    : _group_record(group_record),
      _source(source)
{
    
}

/**
 * Mld6igmpSourceRecord::~Mld6igmpSourceRecord:
 * @: 
 * 
 * Mld6igmpSourceRecord destructor.
 **/
Mld6igmpSourceRecord::~Mld6igmpSourceRecord()
{

}
