
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "bgp4_mib_module.h"
#include "xorpevents.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_1657_bgpversion.hh"
#include "bgp4_mib_1657_bgppeertable.hh"


// Local variables
static const char * mib_mod_name = XORP_MODULE_NAME;
static PeriodicTimerCallback ptcb;
static XorpTimer * pXtbgp;

// Local functions
static bool bgp4_mib_callback() {
    static int i;
    DEBUGMSGTL((mib_mod_name, "Counting...%d\n", i));
    if (-1 == ++i) return false;
    return true;
}

void
init_bgp4_mib_1657 (void)
{
    DEBUGMSGTL((mib_mod_name, "Initializing...\n"));

    // this is to ensure the constructor of BgpMib gets called as soon as
    // we load this MIB
    BgpMib::the_instance();

    init_bgp4_mib_1657_bgpversion();
    init_bgp4_mib_1657_bgppeertable();
    DEBUGMSGTL((mib_mod_name, "Creating periodic event...\n"));
    ptcb = callback(bgp4_mib_callback);
    pXtbgp = new XorpTimer;
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance(); 
    DEBUGMSGTL((mib_mod_name, "Exporting events...\n"));
    eventloop.export_events();

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
    DEBUGMSGTL((mib_mod_name, "Unloading...\n"));
    delete pXtbgp;
    pXtbgp = NULL;
}

BgpMib *  BgpMib::_bgpMib = NULL;


BgpMib& 
BgpMib::the_instance() 
{
    if (!_bgpMib) {
	_bgpMib = new BgpMib;
	DEBUGMSGTL((mib_mod_name, "BgpMib created\n"));
    }
    return *_bgpMib;
}

BgpMib::BgpMib() 
    : XrlBgpV0p2Client(&_xrl_router), 
      _xrl_router(SnmpEventLoop::the_instance(),"bgp4_mib"),
      _xrl_target(&_xrl_router, *this)

{
}

BgpMib::~BgpMib()
{
    if (_bgpMib) delete _bgpMib;
    DEBUGMSGTL((mib_mod_name, "BgpMib destroyed\n"));
}

