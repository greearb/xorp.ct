
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "xorpevents.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_1657_bgpversion.hh"


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
    init_bgp4_mib_1657_bgpversion();
    DEBUGMSGTL((mib_mod_name, "Creating periodic event...\n"));
    ptcb = callback(bgp4_mib_callback);
    pXtbgp = new XorpTimer;
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance(); 
    // *pXtbgp = eventloop.new_periodic (750, ptcb);
    DEBUGMSGTL((mib_mod_name, "Exporting events...\n"));
    eventloop.export_events();
}

void  
deinit_bgp4_mib_1657 (void)
{
    DEBUGMSGTL((mib_mod_name, "Unloading...\n"));
    delete pXtbgp;
    pXtbgp = NULL;
}

BgpMibXrlClient * BgpMibXrlClient::_bgpMib = NULL;

BgpMibXrlClient& BgpMibXrlClient::the_instance() 
{
    if (!_bgpMib) {
	_bgpMib = new BgpMibXrlClient;
	DEBUGMSGTL((mib_mod_name, "BgpMibXrlClient created\n"));
    }
    return *_bgpMib;
}

BgpMibXrlClient::BgpMibXrlClient() 
    : XrlBgpV0p2Client(&_xrl_rtr), 
      _xrl_rtr(SnmpEventLoop::the_instance(),"bgp mib")
{
}

BgpMibXrlClient::~BgpMibXrlClient()
{
    if (_bgpMib) delete _bgpMib;
    DEBUGMSGTL((mib_mod_name, "BgpMibXrlClient destroyed\n"));
}

