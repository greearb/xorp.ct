
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "bgp4_mib_module.h"
#include "xorpevents.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_1657_bgpversion.hh"
#include "bgp4_mib_1657_bgppeertable.hh"


BgpMib BgpMib::_bgp_mib;

void
init_bgp4_mib_1657 (void)
{

    BgpMib & bgp_mib = BgpMib::the_instance();
    DEBUGMSGTL((bgp_mib.name(), "Initializing...\n"));

    init_bgp4_mib_1657_bgpversion();
    init_bgp4_mib_1657_bgppeertable();

    // NOTE:  these xlog calls are required by each mib module, since the
    // runtime linker seems to reset the values of xlog.c static variables
    // everytime a new mib module is loaded.  Only the last unloaded mib module
    // (xorp_if_mib_module) should do xlog cleanup.
     
    xlog_init("snmpd", NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW); 
    xlog_add_default_output();
    xlog_start();
}

void  
deinit_bgp4_mib_1657 (void)
{
    BgpMib & bgp_mib = BgpMib::the_instance();
    DEBUGMSGTL((bgp_mib.name(), "Unloading...\n"));
}



BgpMib& 
BgpMib::the_instance() 
{
    return _bgp_mib;
}

BgpMib::BgpMib() 
    : XrlBgpV0p2Client(&_xrl_router), 
      _xrl_router(SnmpEventLoop::the_instance(),"bgp4_mib"),
      _xrl_target(&_xrl_router, *this), _name(XORP_MODULE_NAME) 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "BgpMib created\n"));
}

BgpMib::~BgpMib() 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "BgpMib destroyed\n"));
}






