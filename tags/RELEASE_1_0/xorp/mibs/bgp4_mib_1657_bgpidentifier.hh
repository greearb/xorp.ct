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


#ifndef __MIBS_BGP4_MIB_1657_BGPIDENTIFIER_HH__
#define __MIBS_BGP4_MIB_1657_BGPIDENTIFIER_HH__

void init_bgp4_mib_1657_bgpidentifier(void);

/* callback functions that will be called from snmpd, thus the
   C linkage */
extern "C" {

Netsnmp_Node_Handler get_bgpIdentifier;

}

#endif // __MIBS_BGP4_MIB_1657_BGPIDENTIFIER_HH__
