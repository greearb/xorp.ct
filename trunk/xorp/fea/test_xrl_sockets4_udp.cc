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

#ident "$XORP: xorp/fea/test_xrl_sockets4_udp.cc,v 1.3 2003/12/17 07:59:57 hodson Exp $"

#include <sysexits.h>

#include <set>

#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "libxipc/sockutil.hh"
#include "libxipc/xrl_std_router.hh"

#include "xrl_socket_server.hh"
#include "addr_table.hh"

#include "xrl/interfaces/socket4_xif.hh"
#include "xrl/targets/test_socket4_base.hh"

static const uint8_t FILLER_VALUE = 0xe7;

// ---------------------------------------------------------------------------
// Verbose output control

static bool s_verbose = false;

inline bool verbose()           { return s_verbose; }
inline void set_verbose(bool v) { s_verbose = v; }

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
// TestAddressTable definition and implementation

class TestAddressTable : public AddressTableBase {
public:
    void add_address(const IPv4& addr);
    void remove_address(const IPv4& addr);
    bool address_valid(const IPv6& addr) const;

    void add_address(const IPv6& addr);
    void remove_address(const IPv6& addr);
    bool address_valid(const IPv4& addr) const;

    uint32_t address_pif_index(const IPv4&) const { return 0; }
    uint32_t address_pif_index(const IPv6&) const { return 0; }

protected:
    set<IPv4> _v4s;
    set<IPv6> _v6s;
};

void
TestAddressTable::add_address(const IPv4& a)
{
    _v4s.insert(a);
}

void
TestAddressTable::remove_address(const IPv4& a)
{
    set<IPv4>::iterator i = _v4s.find(a);
    if (i != _v4s.end()) {
	_v4s.erase(i);
	invalidate_address(a, "invalidated");
    }
}

bool
TestAddressTable::address_valid(const IPv4& a) const
{
    return _v4s.find(a) != _v4s.end();
}

void
TestAddressTable::add_address(const IPv6& a)
{
    _v6s.insert(a);
}

void
TestAddressTable::remove_address(const IPv6& a)
{
    set<IPv6>::iterator i = _v6s.find(a);
    if (i != _v6s.end()) {
	_v6s.erase(i);
	invalidate_address(a, "invalidated");
    }
}

bool
TestAddressTable::address_valid(const IPv6& a) const
{
    return _v6s.find(a) != _v6s.end();
}


// ----------------------------------------------------------------------------
// UDP client

class TestSocket4UDP : public XrlTestSocket4TargetBase {
public:
    TestSocket4UDP(EventLoop& e, const string& ssname,
		   IPv4 finder_host, uint16_t finder_port)
	: _e(e), _ssname(ssname),
	  _p_snd(0), _p_rcv(0), _b_snd(0), _b_rcv(0), _x_err(0), _closed(false)
    {
	_r = new XrlStdRouter(_e, "test_xrl_socket", finder_host, finder_port);
	set_command_map(_r);
	_r->finalize();
    }

    ~TestSocket4UDP()
    {
	set_command_map(0);
	delete _r;
	_r = 0;
    }

    inline const uint32_t bytes_received() const	{ return _b_rcv; }
    inline const uint32_t bytes_sent() const		{ return _b_snd; }
    inline const uint32_t packets_received() const	{ return _p_rcv; }
    inline const uint32_t packets_sent() const		{ return _p_snd; }
    inline const uint32_t xrl_errors() const		{ return _x_err; }
    inline const bool	  closed() const		{ return _closed; }

    /**
     * Bind to interface and port.
     *
     * This is an asynchronous request.  If the request is
     * successfully queued for dispatch then true is returned.
     *
     * Subsequently, if the request is successful _sockid is set to a valid
     * socket identifier, and if unsuccessful the number of xrl errors is
     * bumped by one.
     */
    bool
    bind(IPv4 addr, uint16_t port)
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending bind request (\"%s\", %s/%u)\n",
		    _ssname.c_str(), addr.str().c_str(), port);
	bool s = c.send_udp_open_and_bind(
			_ssname.c_str(), _r->instance_name(), addr, port,
			callback(this, &TestSocket4UDP::bind_cb));
	return s;
    }

    /**
     * Request closure of socket.
     *
     * This is an asychronous request.  If the request is successfully
     * queued for dispatch then true is returned.
     *
     * On success _sockid is cleared.  On failure the number xrl errors is
     * bumped by one.
     */
    bool
    close()
    {
	XrlSocket4V0p1Client c(_r);
	bool s = c.send_close(_ssname.c_str(), _sockid,
			      callback(this, &TestSocket4UDP::close_cb));
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
    start_sending(IPv4 host, uint16_t port, uint32_t bytes, uint32_t ipg_ms)
    {
	_t_snd = _e.new_periodic(ipg_ms, callback(this,
						  &TestSocket4UDP::send_data,
						  host, port, bytes));
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
    send_data(IPv4 host, uint16_t port, uint32_t bytes)
    {
	vector<uint8_t> d(bytes, FILLER_VALUE);

	XrlSocket4V0p1Client c(_r);
	bool s = c.send_send_to(_ssname.c_str(), _sockid, host, port, d,
				callback(this, &TestSocket4UDP::send_data_cb));
	if (s) {
	    verbose_log("Sent %u bytes to %s/%u\n",
			bytes, host.str().c_str(), port);
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
    bind_cb(const XrlError& e, const string* psockid)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err ++;
	    return;
	}
	_sockid = *psockid;
    }

    void
    close_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err ++;
	}
	_sockid.erase();
	_closed = true;
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

    XrlCmdError
    socket4_user_0_1_recv_event(const string&	sockid,
				const IPv4&	src_host,
				const uint32_t&	src_port,
				const vector<uint8_t>&	data)
    {
	XLOG_ASSERT(sockid == _sockid);

	verbose_log("Received data from %s %u\n",
		    src_host.str().c_str(), src_port);
	_p_rcv += 1;
	for (size_t i = 0; i < data.size(); i++) {
	    if (data[i] != FILLER_VALUE) {
		return XrlCmdError::COMMAND_FAILED("Bad data received");
	    }
	}
	_b_rcv += data.size();
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_connect_event(const string&	/* sockid */,
				   const IPv4&		/* src_host */,
				   const uint32_t&	/* src_port */,
				   const string&	/* new_sockid */,
				   bool&		/* accept */)
    {
	return XrlCmdError::COMMAND_FAILED("Not implemented");
    }

    XrlCmdError
    socket4_user_0_1_error_event(const string&	sockid,
				 const string&	error,
				 const bool&	fatal)
    {
	XLOG_ASSERT(sockid == _sockid);
	verbose_log("error event: %s (fatal = %d)\n", error.c_str(), fatal);
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_close_event(const string&	sockid,
				 const string&	reason)
    {
	XLOG_ASSERT(sockid == _sockid);
	verbose_log("close event: %s\n", reason.c_str());
	_closed = true;
	return XrlCmdError::OKAY();
    }

private:
    EventLoop&	_e;
    XrlRouter*	_r;
    string	_ssname;
    string	_sockid;

    uint32_t	_p_snd;		// packets sent
    uint32_t	_p_rcv;		// packets received
    uint32_t	_b_snd;		// bytes sent
    uint32_t	_b_rcv;		// bytes received
    uint32_t	_x_err;		// xrl error count
    bool	_closed;	// close received

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
create_socket_server(EventLoop*		pe,
		     TestAddressTable*	ptat,
		     IPv4		addr,
		     uint16_t		port,
		     XrlSocketServer**	ppxss)
{
    XrlSocketServer* pxss = new XrlSocketServer(*pe, *ptat, addr, port);
    *ppxss = pxss;
    verbose_log("Created socket server.\n");
}

static void
destroy_socket_server(XrlSocketServer** ppxss)
{
    delete *ppxss;
    *ppxss = 0;
    verbose_log("Destroyed socket server.\n");
}

static void
start_socket_server(XrlSocketServer** ppxss)
{
    XrlSocketServer* pxss = *ppxss;
    pxss->startup();
    verbose_log("Starting socket server.\n");
}

static void
shutdown_socket_server(XrlSocketServer** ppxss)
{
    XrlSocketServer* pxss = *ppxss;
    pxss->shutdown();
    verbose_log("Shutting down socket server.\n");
}

static void
verify_socket_owner_count(XrlSocketServer** ppxss, int n, bool* eflag)
{
    XrlSocketServer* pxss = *ppxss;
    int m = pxss->socket_owner_count();
    if (m != n) {
	verbose_err("Socket owner count (%d) does not match expected (%d)\n",
		    m, n);
	*eflag = true;
    }
}

static void
verify_socket4_count(XrlSocketServer** ppxss, int n, bool* eflag)
{
    XrlSocketServer* pxss = *ppxss;
    int m =  pxss->ipv4_socket_count();
    if (m != n) {
	verbose_err("Socket4 count (%d) does not match expected (%d)\n", m, n);
	*eflag = true;
    }
}

static void
create_test_socket(EventLoop*		pe,
		   XrlSocketServer**	ppxss,
		   IPv4			finder_host,
		   uint16_t		finder_port,
		   TestSocket4UDP**	ppu)
{
    XrlSocketServer* pxss = *ppxss;
    verbose_log("Creating TestSocket4UDP instance.\n");
    *ppu = new TestSocket4UDP(*pe, pxss->instance_name(),
			      finder_host, finder_port);
}

static void
destroy_test_socket(TestSocket4UDP** ppu)
{
    delete *ppu;
    *ppu = 0;
}

static void
bind_test_socket(TestSocket4UDP** ppu, IPv4 addr, uint16_t port, bool *err)
{
    TestSocket4UDP* pu = *ppu;
    bool s = pu->bind(addr, port);
    if (s == false)
	*err = true;
}

static void
start_sending(TestSocket4UDP** ppu, IPv4 addr, uint16_t port)
{
    TestSocket4UDP* pu = *ppu;
    pu->start_sending(addr, port, 512, 200);
}

static void
stop_sending(TestSocket4UDP** ppu)
{
    TestSocket4UDP* pu = *ppu;
    pu->stop_sending();
}

static void
match_bytes_sent_received(TestSocket4UDP** pps /* sender */,
			  TestSocket4UDP** ppr /* receiver */,
			  bool* eflag)
{
    TestSocket4UDP* ps = *pps;
    TestSocket4UDP* pr = *ppr;

    if (ps->bytes_sent() != pr->bytes_received()) {
	verbose_err("Bytes sender sent do not match bytes receiver "
		    "received\n");
	*eflag = true;
    }
}

static void
close_test_socket(TestSocket4UDP** ppu)
{
    TestSocket4UDP* pu = *ppu;
    if (pu->close() == false) {
	verbose_err("Failed to send close socket request.\n");
    }
    verbose_log("Closing socket\n");
}

static void
verify_closed(TestSocket4UDP** ppu, bool closed, bool* eflag)
{
    TestSocket4UDP* pu = *ppu;
    if (pu->closed() != closed) {
	verbose_err("Socket close state (%d) does not matched expected.\n",
		    pu->closed());
	*eflag = true;
    }
}

static void
remove_address(TestAddressTable* ta, IPv4 addr)
{
    ta->remove_address(addr);
}

// ----------------------------------------------------------------------------
// Test Main

int
test_main(IPv4 finder_host, uint16_t finder_port)
{
    const IPv4 localhost("127.0.0.1");

    EventLoop e;
    TestAddressTable tat;

    tat.add_address(localhost);

    vector<XorpTimer> ev;	// Vector for timed events
    bool eflag(false);		// Error flag set by timed events

    if (verbose())
	ev.push_back(e.new_periodic(100, callback(ticker)));

    //
    // Create Socket server.
    //
    XrlSocketServer* xss;
    ev.push_back(e.new_oneoff_after_ms(0,
				       callback(create_socket_server,
						&e, &tat, finder_host,
						finder_port, &xss)));
    ev.push_back(e.new_oneoff_after_ms(500,
				       callback(start_socket_server,
						&xss)));

    //
    // Create udp1 socket and bind to port 5000.
    //
    TestSocket4UDP* udp1;
    ev.push_back(e.new_oneoff_after_ms(1000,
				       callback(create_test_socket,
						&e, &xss, finder_host,
						finder_port, &udp1)));

    ev.push_back(e.new_oneoff_after_ms(1500,
				       callback(bind_test_socket, &udp1,
						localhost, uint16_t(5000),
						&eflag)));

    //
    // Create udp2 socket and bind to port 5001.
    //
    TestSocket4UDP* udp2;
    ev.push_back(e.new_oneoff_after_ms(2000,
				       callback(create_test_socket,
						&e, &xss, finder_host,
						finder_port, &udp2)));

    ev.push_back(e.new_oneoff_after_ms(2500,
				       callback(bind_test_socket, &udp2,
						localhost, uint16_t(5001),
						&eflag)));

    //
    // Send packets from udp2/5001 to udp1/5000
    //
    ev.push_back(e.new_oneoff_after_ms(3000,
				       callback(start_sending, &udp2,
						localhost, uint16_t(5000))));

    ev.push_back(e.new_oneoff_after_ms(10000,
				       callback(stop_sending, &udp2)));

    //
    // Send packets from udp1/5000 to udp2/5001
    //
    ev.push_back(e.new_oneoff_after_ms(10000,
				       callback(start_sending, &udp1,
						localhost, uint16_t(5001))));

    ev.push_back(e.new_oneoff_after_ms(12000,
				       callback(stop_sending, &udp1)));

    //
    // Check bytes from udp2 to udp1 match at either end.
    //
    ev.push_back(e.new_oneoff_after_ms(13000,
				       callback(match_bytes_sent_received,
						&udp2, &udp1, &eflag)));

    //
    // Check bytes from udp1 to udp2 match at either end.
    //
    ev.push_back(e.new_oneoff_after_ms(13000,
				       callback(match_bytes_sent_received,
						&udp1, &udp2, &eflag)));

    //
    // Check udp1 is not closed
    //
    ev.push_back(e.new_oneoff_after_ms(13500,
				       callback(verify_closed, &udp1,
						false, &eflag)));
    //
    // Close udp1
    //
    ev.push_back(e.new_oneoff_after_ms(14000, callback(close_test_socket,
						       &udp1)));

    //
    // Check udp1 closed
    //
    ev.push_back(e.new_oneoff_after_ms(14500,
				       callback(verify_closed, &udp1,
						true, &eflag)));

    //
    // Check udp2 is not closed
    //
    ev.push_back(e.new_oneoff_after_ms(14800,
				       callback(verify_closed, &udp2,
						false, &eflag)));

    //
    // Invalidate localhost, which should cause udp2 to be closed by
    // server.
    //
    ev.push_back(e.new_oneoff_after_ms(15000,
				       callback(remove_address,
						&tat, localhost)));

    //
    // Check udp2 is closed.
    //
    ev.push_back(e.new_oneoff_after_ms(15500,
				       callback(verify_closed, &udp2,
						true, &eflag)));

    //
    // Check no sockets open
    //
    ev.push_back(e.new_oneoff_after_ms(16000,
				       callback(verify_socket4_count,
						&xss, 0, &eflag)));
    //
    // Destroy udp1 and udp2.
    //
    ev.push_back(e.new_oneoff_after_ms(19998,
				       callback(destroy_test_socket, &udp1)));

    ev.push_back(e.new_oneoff_after_ms(19998,
				       callback(destroy_test_socket, &udp2)));

    //
    // Check socket server is holding no resources.
    //
    ev.push_back(e.new_oneoff_after_ms(21000,
				       callback(verify_socket_owner_count,
						&xss, 0, &eflag)));

    ev.push_back(e.new_oneoff_after_ms(21000,
				       callback(verify_socket4_count,
						&xss, 0, &eflag)));
    //
    // Shutdown socket server.
    //
    ev.push_back(e.new_oneoff_after_ms(24000,
				       callback(shutdown_socket_server,
						&xss)));
    //
    // Destroy socket server.
    //
    ev.push_back(e.new_oneoff_after_ms(25000,
				       callback(destroy_socket_server,
						&xss)));

    //
    // Force exit.
    //
    bool finish(false);
    ev.push_back(e.set_flag_after_ms(26000, &finish));

    while (eflag == false && finish == false) {
	e.run();
    }

    if (eflag)
	return -1;

    return 0;
}



/* ------------------------------------------------------------------------- */

static void
usage(const char* progname)
{
    fprintf(stderr, "usage: %s [-F <host>:<port>] [-v]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -F <host>[:<port>]  "
            "Specify arguments for external Finder instance\n");
    fprintf(stderr, "  -v                  Verbose output\n");
    fprintf(stderr, "Runs remote UDP socket test using Xrls.\n");
    exit(EX_USAGE);
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
	finder_port = FINDER_DEFAULT_PORT;
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

    IPv4        finder_addr = FINDER_DEFAULT_HOST;
    uint16_t    finder_port = FINDER_DEFAULT_PORT;

    int r = 0;	// Return code

    int ch = 0;
    while ((ch = getopt(argc, argv, "F:v")) != -1) {
	switch (ch) {
	case 'F':
	    if (parse_finder_arg(optarg, finder_addr, finder_port) == false) {
		usage(argv[0]);
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

    try {
	if (r == 0) {
	    r = test_main(finder_addr, finder_port);
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

    return r;
}
