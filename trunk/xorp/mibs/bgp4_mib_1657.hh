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


#ifndef __MIBS_BGP4_MIB_1657_HH__
#define __MIBS_BGP4_MIB_1657_HH__ 

#include "config.h"
#include <queue>
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_args.hh"
#include "xrl/interfaces/bgp_xif.hh"
#include "bgp4_mib_xrl_target.hh"


/* dlopen functions */
extern "C" {
/**
 * This is the first and only function that gets called when the SNMP
 * agent loads the bgp_mib_1657 module.  This function is responsible for
 * instantiating the @ref BgpMib object and for registering the scalars and
 * tables that are implemented by this MIB.
 *
 * @short BGP MIB initialization function called by the SNMP agent
 */
void            init_bgp4_mib_1657 (void);
/**
 * This is the function called by the SNMP agent immediately before unloading
 * the bgp_mib_1657 module.  After this function is called, the shared object
 * will be unloaded and therefore inaccessible.  This function unregisters any
 * alarms and file descriptors to prevent callbacks to non-accessible
 * functions.
 *
 * @short BGP MIB clean up function called by the SNMP agent before unloading
 */
void          deinit_bgp4_mib_1657 (void);
}




/**
 * This is the top level class for the BGP MIB tree.  It is a singleton class
 * that implements an @ref XrlBgpV0p2Client for BGP, and contains an 
 * @ref XrlStdRouter and an @ref XrlBgpMibTarget.
 *
 * @short Top level class for the BGP MIB tree 
 *
 */
class BgpMib : public XrlBgpV0p2Client
{
public:
    /**
     * @return the one and only instance in this class
     */
    static BgpMib& the_instance();
    /**
     * @return the name that will be used to identify this mib in the logfile
     */
    static const char * name()    { return XORP_MODULE_NAME; };
    
    ~BgpMib();

private:
    BgpMib();
    XrlStdRouter _xrl_router;
    XrlBgpMibTarget _xrl_target;

    const string  _name;

    static BgpMib _bgp_mib;
};

#endif    /* __MIBS_BGP4_MIB_1657_HH__ */                      
