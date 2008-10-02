// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/test_xrl_sockets4_udp.cc,v 1.25 2008/07/23 05:10:12 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"
#include "libxorp/xorpfd.hh"

#include <set>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/sockutil.hh"
#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/socket4_xif.hh"
#include "xrl/targets/test_socket4_base.hh"


static const uint8_t FILLER_VALUE = 0xe7;

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
// UDP client

class TestSocket4UDP : public XrlTestSocket4TargetBase {
public:
    TestSocket4UDP(EventLoop& e, const string& fea_target_name,
		   IPv4 finder_host, uint16_t finder_port)
	: _e(e), _fea_target_name(fea_target_name),
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

    uint32_t bytes_received() const	{ return _b_rcv; }
    uint32_t bytes_sent() const		{ return _b_snd; }
    uint32_t packets_received() const	{ return _p_rcv; }
    uint32_t packets_sent() const	{ return _p_snd; }
    uint32_t xrl_errors() const		{ return _x_err; }
    bool closed() const			{ return _closed; }

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
		    _fea_target_name.c_str(), addr.str().c_str(), port);
	bool s = c.send_udp_open_and_bind(
	    _fea_target_name.c_str(), _r->instance_name(), addr, port,
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
	bool s = c.send_close(_fea_target_name.c_str(), _sockid,
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
	_t_snd = _e.new_periodic_ms(ipg_ms,
				    callback(this,
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
	bool s = c.send_send_to(_fea_target_name.c_str(), _sockid, host, port,
				d, callback(this, &TestSocket4UDP::send_data_cb));
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
				const string&	if_name,
				const string&	vif_name,
				const IPv4&	src_host,
				const uint32_t&	src_port,
				const vector<uint8_t>&	data)
    {
	UNUSED(if_name);
	UNUSED(vif_name);

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
    socket4_user_0_1_inbound_connect_event(const string&	/* sockid */,
					   const IPv4&		/* src_host */,
					   const uint32_t&	/* src_port */,
					   const string&	/* new_sockid */,
					   bool&		/* accept */)
    {
	return XrlCmdError::COMMAND_FAILED("Not implemented");
    }

    XrlCmdError
    socket4_user_0_1_outgoing_connect_event(const string&	/* sockid */)
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
    socket4_user_0_1_disconnect_event(const string&	sockid)
    {
	XLOG_ASSERT(sockid == _sockid);
	verbose_log("disconnect event\n");
	_closed = true;
	return XrlCmdError::OKAY();
    }

private:
    EventLoop&	_e;
    XrlRouter*	_r;
    string	_fea_target_name;
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
create_test_socket(EventLoop*		pe,
		   string		fea_target_name,
		   IPv4			finder_host,
		   uint16_t		finder_port,
		   TestSocket4UDP**	ppu)
{
    verbose_log("Creating TestSocket4UDP instance.\n");
    *ppu = new TestSocket4UDP(*pe, fea_target_name, finder_host, finder_port);
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

// ----------------------------------------------------------------------------
// Test Main

int
test_main(IPv4 finder_host, uint16_t finder_port)
{
    EventLoop e;
    const string fea_target_name = "fea";
    vector<XorpTimer> ev;	// Vector for timed events
    bool eflag(false);		// Error flag set by timed events

    if (verbose())
	ev.push_back(e.new_periodic_ms(100, callback(ticker)));

    //
    // Create udp1 socket and bind to port 5000.
    //
    TestSocket4UDP* udp1;
    ev.push_back(e.new_oneoff_after_ms(1000,
				       callback(create_test_socket,
						&e, fea_target_name,
						finder_host, finder_port,
						&udp1)));

    ev.push_back(e.new_oneoff_after_ms(1500,
				       callback(bind_test_socket, &udp1,
						IPv4::LOOPBACK(), uint16_t(5000),
						&eflag)));

    //
    // Create udp2 socket and bind to port 5001.
    //
    TestSocket4UDP* udp2;
    ev.push_back(e.new_oneoff_after_ms(2000,
				       callback(create_test_socket,
						&e, fea_target_name,
						finder_host, finder_port,
						&udp2)));

    ev.push_back(e.new_oneoff_after_ms(2500,
				       callback(bind_test_socket, &udp2,
						IPv4::LOOPBACK(), uint16_t(5001),
						&eflag)));

    //
    // Send packets from udp2/5001 to udp1/5000
    //
    ev.push_back(e.new_oneoff_after_ms(3000,
				       callback(start_sending, &udp2,
						IPv4::LOOPBACK(), uint16_t(5000))));

    ev.push_back(e.new_oneoff_after_ms(10000,
				       callback(stop_sending, &udp2)));

    //
    // Send packets from udp1/5000 to udp2/5001
    //
    ev.push_back(e.new_oneoff_after_ms(10000,
				       callback(start_sending, &udp1,
						IPv4::LOOPBACK(), uint16_t(5001))));

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
    ev.push_back(e.new_oneoff_after_ms(15000,
				       callback(verify_closed, &udp2,
						false, &eflag)));

    //
    // Close udp2
    //
    ev.push_back(e.new_oneoff_after_ms(15500, callback(close_test_socket,
						       &udp2)));

    //
    // Check udp2 is closed.
    //
    ev.push_back(e.new_oneoff_after_ms(16000,
				       callback(verify_closed, &udp2,
						true, &eflag)));

    //
    // Destroy udp1 and udp2.
    //
    ev.push_back(e.new_oneoff_after_ms(19998,
				       callback(destroy_test_socket, &udp1)));

    ev.push_back(e.new_oneoff_after_ms(19998,
				       callback(destroy_test_socket, &udp2)));

    //
    // Force exit.
    //
    bool finish(false);
    ev.push_back(e.set_flag_after_ms(26000, &finish));

    while (eflag == false && finish == false) {
	e.run();
    }

    if (eflag)
	return 1;

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

    if (r != 0)
	return (1);
    return (0);
}
