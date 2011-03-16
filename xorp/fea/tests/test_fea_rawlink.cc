// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/xorpfd.hh"



#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/sockutil.hh"
#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/fea_rawlink_xif.hh"
#include "xrl/targets/test_fea_rawlink_base.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

static const uint8_t FILLER_VALUE = 0xe7;
static const uint16_t ETHER_TYPE = 0x9000;   // ETHERTYPE_LOOPBACK in BSD

// Some globals are necessary (insufficient bound callback slots)
static string g_fea_target_name;
static IPv4 g_finder_host;
static uint16_t g_finder_port;

// ---------------------------------------------------------------------------
// Verbose output control

static bool s_verbose = false;

bool verbose()           { return s_verbose; }
void set_verbose(bool v) { s_verbose = v; }

#define verbose_log(x...)                                                     \
do {                                                                          \
    if (verbose()) {                                                          \
        printf("From %s:%d: ", __FILE__, __LINE__);                           \
        printf(x);                                                            \
	fflush(stdout);							      \
    }                                                                         \
} while(0)

#define verbose_err(x...)                                                     \
do {                                                                          \
    printf("From %s:%d: ", __FILE__, __LINE__);                               \
    printf(x);                                                                \
    fflush(stdout);							      \
} while(0)



// ----------------------------------------------------------------------------
// Raw link client

class TestRawLink : public XrlTestFeaRawlinkTargetBase {
public:
    TestRawLink(EventLoop& e, const string& fea_target_name,
		IPv4 finder_host, uint16_t finder_port,
		const string& ifname,
		const string& vifname,
		const Mac& src,
		bool is_sender)
	: _e(e),
	  _fea_target_name(fea_target_name),
	  _ifname(ifname), _vifname(vifname),
	  _src(src), _is_sender(is_sender),
	  _p_snd(0), _p_rcv(0), _b_snd(0), _b_rcv(0), _x_err(0),
	 _unregistered(false)
    {
	_r = new XrlStdRouter(_e, "test_fea_rawlink",
			      finder_host, finder_port);
	set_command_map(_r);
	_r->finalize();
    }

    ~TestRawLink()
    {
	set_command_map(0);
	delete _r;
	_r = 0;
    }

    uint32_t bytes_received() const	{ return _b_rcv; }
    uint32_t bytes_sent() const		{ return _b_snd; }
    uint32_t packets_received() const	{ return _p_rcv; }
    uint32_t packets_sent() const	{ return _p_snd; }
    uint32_t xrl_errors() const		{ return _x_err; }
    bool unregistered() const		{ return _unregistered; }

    /**
     * Bind to interface.
     *
     * This is an asynchronous request.  If the request is
     * successfully queued for dispatch then true is returned.
     *
     * Subsequently, if the request is successful _sockid is set to a valid
     * socket identifier, and if unsuccessful the number of xrl errors is
     * bumped by one.
     */
    bool
    register_link()
    {
	XrlRawLinkV0p1Client c(_r);
	verbose_log("Sending register request (\"%s\", %s/%s/%x)\n",
		    _fea_target_name.c_str(),
		    _ifname.c_str(),
		    _vifname.c_str(),
		    XORP_UINT_CAST(ETHER_TYPE));
	bool s = c.send_register_receiver(
		_fea_target_name.c_str(),
		_r->instance_name(),
		_ifname,
		_vifname,
		ETHER_TYPE,
		"",
		false,
		callback(this, &TestRawLink::register_cb));
	return s;
    }

    /**
     * Request closure of link.
     *
     * This is an asychronous request.  If the request is successfully
     * queued for dispatch then true is returned.
     *
     * On success _sockid is cleared.  On failure the number xrl errors is
     * bumped by one.
     */
    bool
    unregister_link()
    {
	XrlRawLinkV0p1Client c(_r);
	verbose_log("Sending unregister request (\"%s\", %s/%s/%x)\n",
		    _fea_target_name.c_str(),
		    _ifname.c_str(),
		    _vifname.c_str(),
		    XORP_UINT_CAST(ETHER_TYPE));
	bool s = c.send_unregister_receiver(
		_fea_target_name.c_str(),
		_r->instance_name(),
		_ifname,
		_vifname,
		ETHER_TYPE,
		"",
		callback(this, &TestRawLink::unregister_cb));
	return s;
    }

    /**
     * Start sending packets.
     *
     * @param host address to send data to.
     * @param port to send data to.
     * @param bytes size of each packet.
     * @param ipg_ms interpacket gap in milliseconds.
     */
    void
    start_sending(const Mac& dest, uint32_t bytes, uint32_t ipg_ms)
    {
	_t_snd = _e.new_periodic_ms(ipg_ms,
				    callback(this,
					     &TestRawLink::send_data,
					     dest, bytes));
    }

    /**
     * Stop sending packets.
     */
    void
    stop_sending()
    {
	_t_snd.unschedule();
    }

protected:
    bool
    send_data(Mac dest, uint32_t bytes)
    {
	vector<uint8_t> d(bytes, FILLER_VALUE);

	XrlRawLinkV0p1Client c(_r);
	bool s = c.send_send(_fea_target_name.c_str(), _ifname,
			     _vifname, _src, dest, ETHER_TYPE,
			     d, callback(this, &TestRawLink::send_data_cb));
	if (s) {
	    verbose_log("Sent %u(%u) bytes to %s\n",
			XORP_UINT_CAST(bytes), XORP_UINT_CAST(d.size()),
			dest.str().c_str());
	    _b_snd += bytes;
	    _p_snd += 1;
	    return true;
	}
	verbose_err("Send failed.");
	return false;
    }

    void
    send_data_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    stop_sending();
	    _x_err ++;
	}
    }

    void
    register_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err ++;
	    return;
	}
    }

    void
    unregister_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err ++;
	}
	_unregistered = true;
    }

protected:
    XrlCmdError
    common_0_1_get_target_name(string& name)
    {
	name = _r->instance_name();
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_get_version(string& version)
    {
	version = "0.1";
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_get_status(uint32_t& status, string& reason)
    {
	status = PROC_READY;
	reason = "";
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_shutdown()
    {
	return XrlCmdError::COMMAND_FAILED("Not supported");
    }

    /**
     *  Receive a raw link-level packet on an interface.
     *
     *  @param if_name the interface name the packet arrived on.
     *  @param vif_name the vif name the packet arrived on.
     *  @param src_address the MAC source address.
     *  @param dst_address the MAC destination address.
     *  @param ether_type the EtherType protocol number or the Destination SAP.
     *  It must be between 1536 and 65535 to specify the EtherType, or between
     *  1 and 255 to specify the Destination SAP for IEEE 802.2 LLC frames.
     *  @param payload the payload, everything after the MAC header.
     */
    XrlCmdError raw_link_client_0_1_recv(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const Mac&	src_address,
	const Mac&	dst_address,
	const uint32_t&	ether_type,
	const vector<uint8_t>&	payload)
    {
	if (_is_sender) {
	    // If we're sending over loopback, we will always
	    // see our own frames because both the source and
	    // destination endpoint addresses are the same.
	    // So keep RawLink instances strictly to
	    // sending and receiving.
	    return XrlCmdError::OKAY();
	}

	verbose_log("Received %u bytes from %s\n",
		    XORP_UINT_CAST(payload.size()),
		    src_address.str().c_str());
	_p_rcv += 1;

	XLOG_ASSERT(if_name == _ifname);
	XLOG_ASSERT(vif_name == _vifname);

	XLOG_ASSERT(dst_address == _src);
	XLOG_ASSERT(src_address == _src);
	XLOG_ASSERT(ether_type == ETHER_TYPE);

	// Now check the payload.
	for (size_t i = 0; i < payload.size(); i++) {
	    if (payload[i] != FILLER_VALUE) {
		return XrlCmdError::COMMAND_FAILED("Bad data received");
	    }
	}
	_b_rcv += payload.size();
	return XrlCmdError::OKAY();
    }

private:
    EventLoop&	_e;
    XrlRouter*	_r;
    string	_fea_target_name;

    string	_ifname;
    string	_vifname;

    Mac		_src;
    bool	_is_sender;

    uint32_t	_p_snd;		// packets sent
    uint32_t	_p_rcv;		// packets received
    uint32_t	_b_snd;		// bytes sent
    uint32_t	_b_rcv;		// bytes received
    uint32_t	_x_err;		// xrl error count
    bool	_unregistered;	// link unregistered

    XorpTimer	_t_snd;		// send timer
};


// ----------------------------------------------------------------------------
// Events

static bool
ticker()
{
    static const char x[] = { '.', 'o', '0', 'O', '0', 'o' };
    static char erase = '\0';
    static int p = 0;

    fprintf(stderr, "%c%c", erase, x[p]);
    p = (p + 1) % (sizeof(x));
    erase = '\b';
    return true;
}

static void
create_test_link(EventLoop*	pe,
		 string		ifname,
		 string		vifname,
		 Mac		src,
		 bool		is_sender,
		 TestRawLink**	ppu)
{
    verbose_log("Creating TestRawLink instance.\n");
    *ppu = new TestRawLink(*pe,
			   g_fea_target_name, g_finder_host, g_finder_port,
			   ifname, vifname, src, is_sender);
}

static void
destroy_test_link(TestRawLink** ppu)
{
    delete *ppu;
    *ppu = 0;
}

static void
register_test_link(TestRawLink** ppu, bool *err)
{
    TestRawLink* pu = *ppu;
    bool s = pu->register_link();
    if (s == false)
	*err = true;
}

static void
start_sending(TestRawLink** ppu, Mac addr)
{
    TestRawLink* pu = *ppu;
    pu->start_sending(addr, 512, 200);
}

static void
stop_sending(TestRawLink** ppu)
{
    TestRawLink* pu = *ppu;
    pu->stop_sending();
}

static void
match_bytes_sent_received(TestRawLink** pps /* sender */,
			  TestRawLink** ppr /* receiver */,
			  bool* eflag)
{
    TestRawLink* ps = *pps;
    TestRawLink* pr = *ppr;

    if (ps->bytes_sent() != pr->bytes_received()) {
	verbose_err("Bytes sender sent %u do not match bytes receiver "
		    "received %u\n",
		    XORP_UINT_CAST(ps->bytes_sent()),
		    XORP_UINT_CAST(pr->bytes_received()));
	*eflag = true;
    }
}

static void
match_bytes_not_sent(TestRawLink** pps, size_t count, bool* eflag)
{
    TestRawLink* ps = *pps;

    if (ps->bytes_sent() == count) {
	verbose_err("Bytes sender sent %u match expected %u\n",
		    XORP_UINT_CAST(ps->bytes_sent()),
		    XORP_UINT_CAST(count));
	*eflag = true;
    }
}

static void
match_bytes_received(TestRawLink** ppr, size_t count, bool* eflag)
{
    TestRawLink* pr = *ppr;

    if (pr->bytes_received() != count) {
	verbose_err("Bytes receiver received %u does not match expected %u\n",
		    XORP_UINT_CAST(pr->bytes_received()),
		    XORP_UINT_CAST(count));
	*eflag = true;
    }
}

static void
unregister_test_link(TestRawLink** ppu)
{
    TestRawLink* pu = *ppu;
    if (pu->unregister_link() == false) {
	verbose_err("Failed to send unregister link request.\n");
    }
    verbose_log("Unregistering link\n");
}

static void
verify_unregistered(TestRawLink** ppu, bool unregistered, bool* eflag)
{
    TestRawLink* pu = *ppu;
    if (pu->unregistered() != unregistered) {
	verbose_err("Link unregistered state (%d) does not match expected.\n",
		    pu->unregistered());
	*eflag = true;
    }
}

// ----------------------------------------------------------------------------
// Test Main

int
test_main(IPv4 finder_host, uint16_t finder_port,
	  string ifname, string vifname)
{
    EventLoop e;
    string fea_target_name = "fea";
    vector<XorpTimer> ev;	// Vector for timed events
    bool eflag(false);		// Error flag set by timed events
    Mac mac;

#ifndef HAVE_PCAP_H
    fprintf(stdout, "Test Skipped: No L2 I/O mechanism found: "
	    "HAVE_PCAP_H is not defined.\n");
    return 0;
#endif

    //
    // Wait for FEA bringup and pull the IfTree.
    //
    bool expired = false;
    IfMgrXrlMirror ifmgr(e, fea_target_name.c_str(), finder_host, finder_port);

    ifmgr.startup();
    XorpTimer t = e.set_flag_after_ms(3000, &expired);
    while (ifmgr.status() != SERVICE_RUNNING) {
	e.run();
	if (expired) {
	    verbose_err("ifmgr did not reach running state: "
			"state = %s note = \"%s\".\n",
			service_status_name(ifmgr.status()),
			ifmgr.status_note().c_str());
	    return -1;
	}
    }

    //
    // The interface must exist and be enabled. Retrieve its MAC address.
    //
    const IfMgrIfAtom* fi = ifmgr.iftree().find_interface(ifname);
    if (fi == NULL) {
	verbose_err("Could not find %s in iftree.\n", ifname.c_str());
	ifmgr.shutdown();
	return -1;
    }
    if (! fi->enabled()) {
	verbose_err("%s is disabled.\n", ifname.c_str());
	ifmgr.shutdown();
	return -1;
    }

    // For the Linux loopback interface the Mac is likely to be empty.
    mac = fi->mac();
    if (mac.is_zero()) {
	verbose_log("%s has an empty MAC address.\n", ifname.c_str());
	//ifmgr.shutdown();
	//return -1;
    }

    //
    // The VIF must exist and be enabled.
    //
    const IfMgrVifAtom* fv = ifmgr.iftree().find_vif(ifname, vifname);
    if (fv == NULL) {
	verbose_err("Could not find %s/%s in iftree.\n",
		    ifname.c_str(), vifname.c_str());
	ifmgr.shutdown();
	return -1;
    }
    if (! fv->enabled()) {
	verbose_err("%s/%s is disabled.\n", ifname.c_str(), vifname.c_str());
	ifmgr.shutdown();
	return -1;
    }

    if (verbose())
	ev.push_back(e.new_periodic_ms(100, callback(ticker)));

    //
    // NOTE WELL: callback() only supports up to 6 bound (stored)
    // arguments for solitary C++ functions, therefore we end up
    // using a few global variables here.
    //
    g_fea_target_name = fea_target_name;
    g_finder_host = finder_host;
    g_finder_port = finder_port;

    //
    // Create rl1 sender.
    //
    TestRawLink* rl1;
    ev.push_back(e.new_oneoff_after_ms(1000,
				       callback(create_test_link,
						&e,
						ifname, vifname,
						mac, true,
						&rl1)));

    //
    // Create rl2 listener link and register it.
    //
    TestRawLink* rl2;
    ev.push_back(e.new_oneoff_after_ms(2000,
				       callback(create_test_link,
						&e,
						ifname, vifname,
						mac, false,
						&rl2)));

    ev.push_back(e.new_oneoff_after_ms(2500,
				       callback(register_test_link, &rl2,
						&eflag)));

    //
    // Send packets from rl1 to rl2.
    //
    ev.push_back(e.new_oneoff_after_ms(10000,
				       callback(start_sending, &rl1, mac)));

    ev.push_back(e.new_oneoff_after_ms(12000,
				       callback(stop_sending, &rl1)));

    //
    // Check bytes from rl1 to rl2 match at either end.
    //
    ev.push_back(e.new_oneoff_after_ms(13000,
				       callback(match_bytes_sent_received,
						&rl1, &rl2, &eflag)));

    //
    // Check rl2 is not unregistered
    //
    ev.push_back(e.new_oneoff_after_ms(15000,
				       callback(verify_unregistered, &rl2,
						false, &eflag)));

    //
    // Unregister rl2
    //
    ev.push_back(e.new_oneoff_after_ms(15500, callback(unregister_test_link,
						       &rl2)));

    //
    // Check rl2 is unregistered.
    //
    ev.push_back(e.new_oneoff_after_ms(16000,
				       callback(verify_unregistered, &rl2,
						true, &eflag)));

    //
    // Force exit.
    //
    bool finish = false;
    ev.push_back(e.set_flag_after_ms(26000, &finish));

    while (eflag == false && finish == false) {
	e.run();
    }

    if (eflag) {
	ifmgr.shutdown();
	return 1;
    }

    //
    // Track the last send/receive counts. We need to run the
    // event loop above because everything is happening asynchronously,
    // and then run it again below.
    //
    size_t rl1_sent = rl1->bytes_sent();
    size_t rl2_received = rl2->bytes_received();

    ev.clear();

    //
    // Send packets from rl1 to rl2.
    //
    ev.push_back(e.new_oneoff_after_ms(4000,
				       callback(start_sending, &rl1, mac)));

    ev.push_back(e.new_oneoff_after_ms(6000,
				       callback(stop_sending, &rl1)));

    //
    // Check rl1 sent something (ie the count changed)
    //
    ev.push_back(e.new_oneoff_after_ms(7000,
				       callback(match_bytes_not_sent,
						&rl1, rl1_sent, &eflag)));

    //
    // Check rl2 did not receive any packets (ie the count did not change)
    //
    ev.push_back(e.new_oneoff_after_ms(7000,
				       callback(match_bytes_received,
						&rl2, rl2_received, &eflag)));

    //
    // Destroy rl1 and rl2.
    //
    ev.push_back(e.new_oneoff_after_ms(9998,
				       callback(destroy_test_link, &rl1)));

    ev.push_back(e.new_oneoff_after_ms(9998,
				       callback(destroy_test_link, &rl2)));

    //
    // Force exit.
    //
    finish = false;
    ev.push_back(e.set_flag_after_ms(12000, &finish));

    while (eflag == false && finish == false) {
	e.run();
    }

    ifmgr.shutdown();

    if (eflag)
	return 1;

    return 0;
}



/* ------------------------------------------------------------------------- */

static void
usage(const char* progname)
{
    fprintf(stderr, "usage: %s [-F <host>:<port>] [-v] <ifname> <vifname>\n",
	    progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -F <host>[:<port>]  "
            "Specify arguments for external Finder instance\n");
    fprintf(stderr, "  -v                  Verbose output\n");
    fprintf(stderr, "Runs rawlink test using Xrls over <ifname>/<vifname>.\n");
    exit(1);
}


static bool
parse_finder_arg(const char* host_colon_port,
		 IPv4&	     finder_addr,
		 uint16_t&   finder_port)
{
    string finder_host;

    const char* p = strstr(host_colon_port, ":");

    if (p) {
	finder_host = string(host_colon_port, p);
	finder_port = atoi(p + 1);
	if (finder_port == 0) {
	    fprintf(stderr, "Invalid port \"%s\"\n", p + 1);
	    return false;
	}
    } else {
	finder_host = string(host_colon_port);
	finder_port = FinderConstants::FINDER_DEFAULT_PORT();
    }

    try {
	finder_addr = IPv4(finder_host.c_str());
    } catch (const InvalidString& ) {
	// host string may need resolving
	in_addr ia;
	if (address_lookup(finder_host, ia) == false) {
	    fprintf(stderr, "Invalid host \"%s\"\n", finder_host.c_str());
	    return false;
	}
	finder_addr.copy_in(ia);
    }
    return true;
}

// ----------------------------------------------------------------------------
// Main

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    IPv4        finder_addr = FinderConstants::FINDER_DEFAULT_HOST();
    uint16_t    finder_port = FinderConstants::FINDER_DEFAULT_PORT();
    char* progname = argv[0];

    int r = 0;	// Return code
    int ch = 0;
    while ((ch = getopt(argc, argv, "F:v")) != -1) {
	switch (ch) {
	case 'F':
	    if (parse_finder_arg(optarg, finder_addr, finder_port) == false) {
		usage(progname);
		r = 1;
	    }
	    break;
	case 'v':
	    set_verbose(true);
	    break;
	default:
	    usage(argv[0]);
	    r = 1;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc != 2)
	usage(progname);

    string ifname(argv[0]);
    string vifname(argv[1]);

    try {
	if (r == 0) {
	    r = test_main(finder_addr, finder_port, ifname, vifname);
	}
    }
    catch (...) {
	xorp_catch_standard_exceptions();
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    if (r != 0)
	return (1);
    return (0);
}
