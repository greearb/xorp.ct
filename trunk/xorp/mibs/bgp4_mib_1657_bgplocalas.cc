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


#include "bgp4_mib_module.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "xorpevents.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_1657_bgplocalas.hh"

/** Initializes the bgp_localas_scalar module */
void
init_bgp4_mib_1657_bgplocalas (void)
{
    static oid bgpLocalAs_oid[] = { 1,3,6,1,2,1,15,2, 0 };

    DEBUGMSGTL((BgpMib::the_instance().name(),"Initializing bgpLocalAs...\n"));

    netsnmp_register_read_only_instance(netsnmp_create_handler_registration
                                        ("bgpLocalAs",
                                         get_bgpLocalAs,
                                         bgpLocalAs_oid,
                                         OID_LENGTH(bgpLocalAs_oid),
                                         HANDLER_CAN_RONLY));
}

void get_local_as_done(const XrlError& e, const uint32_t* as, 
			       netsnmp_delegated_cache* cache)
{

    DEBUGMSGTL((BgpMib::the_instance().name(), "get_local_as_done called\n"));

    netsnmp_request_info *requests;
    netsnmp_agent_request_info *reqinfo;

    cache = netsnmp_handler_check_cache(cache);

    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    requests = cache->requests;

    if (e != XrlError::OKAY()) {
	DEBUGMSGTL((BgpMib::the_instance().name(), "XrlError: "));
	DEBUGMSGTL((BgpMib::the_instance().name(), e.error_msg()));
	DEBUGMSGTL((BgpMib::the_instance().name(), "\n"));
	netsnmp_set_request_error(reqinfo, requests, SNMP_NOSUCHINSTANCE);
	requests->delegated = 0;
	return;
    }

    DEBUGMSGTL((BgpMib::the_instance().name(),"continued delayed req, "
	"mode = %d\n", reqinfo->mode));

    // no longer delegated since we'll answer down below

    requests->delegated = 0;
    uint32_t local_as = *as;

    snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
	reinterpret_cast<unsigned char *>(&local_as), sizeof(uint32_t));

    return;
}


int
get_bgpLocalAs(netsnmp_mib_handler * handler,
                          netsnmp_handler_registration * reginfo,
                          netsnmp_agent_request_info *  reqinfo,
                          netsnmp_request_info *requests)
{
    DEBUGMSGTL((BgpMib::the_instance().name(), "get_bgpLocalAs called\n"));
    BgpMib& bgp_mib = BgpMib::the_instance();
    BgpMib::CB0 cb_localas;
    netsnmp_delegated_cache* req_cache = netsnmp_create_delegated_cache
	(handler, reginfo, reqinfo, requests, NULL);
    cb_localas = callback(get_local_as_done, req_cache);
    bgp_mib.send_get_local_as("bgp", cb_localas); 

    requests->delegated = 1;
    return SNMP_ERR_NOERROR;
}
