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
#include "bgp4_mib_1657_bgpidentifier.hh"

/** Initializes the bgp_identifier_scalar module */
void
init_bgp4_mib_1657_bgpidentifier (void)
{
    static oid bgpIdentifier_oid[] = { 1,3,6,1,2,1,15,4, 0 };

    DEBUGMSGTL((BgpMib::the_instance().name(),
	"Initializing bgpIdentifier...\n"));

    netsnmp_register_read_only_instance(netsnmp_create_handler_registration
                                        ("bgpIdentifier",
                                         get_bgpIdentifier,
                                         bgpIdentifier_oid,
                                         OID_LENGTH(bgpIdentifier_oid),
                                         HANDLER_CAN_RONLY));
}

void get_bgp_id_done(const XrlError& e, const IPv4* bgp_id, 
	             netsnmp_delegated_cache* cache)
{

    DEBUGMSGTL((BgpMib::the_instance().name(), "get_bgp_id_done called\n"));

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

    uint32_t raw_ip = bgp_id->addr();

    snmp_set_var_typed_value(requests->requestvb, ASN_IPADDRESS,
	(unsigned char *) &raw_ip, sizeof(raw_ip));

    return;
}


int
get_bgpIdentifier(netsnmp_mib_handler * handler,
                          netsnmp_handler_registration * reginfo,
                          netsnmp_agent_request_info *  reqinfo,
                          netsnmp_request_info *requests)
{
    DEBUGMSGTL((BgpMib::the_instance().name(), "get_bgpIdentifier called\n"));
    BgpMib& bgp_mib = BgpMib::the_instance();
    BgpMib::GetBgpIdCB cb_id;
    netsnmp_delegated_cache* req_cache = netsnmp_create_delegated_cache
	(handler, reginfo, reqinfo, requests, NULL);
    cb_id = callback(get_bgp_id_done, req_cache);
    bgp_mib.send_get_bgp_id("bgp", cb_id);

    requests->delegated = 1;
    return SNMP_ERR_NOERROR;
}
