// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007-2008 International Computer Science Institute
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


#define file_location() c_format("%s: %d", __FILE__, __LINE__)

//
// ----------------------------------------------------------------------------
// Scheduling Time class
class SchedulingTime {
private:
    int _time;
    int _increment;
	
public:
    SchedulingTime(int start_time = 0, int increment = 250) 
	: _time(start_time), _increment(increment) {}
	
    int now() { return _time; } 
    int next() { _time += _increment; return _time; }
    int next(int inc) { _time += inc; return _time; }
};

//
// ----------------------------------------------------------------------------
// TCP client/server commonalities class

class TestSocket4TCP : public XrlTestSocket4TargetBase {
public:
    TestSocket4TCP(EventLoop& e, const string& fea_target_name,
		   IPv4 finder_host, uint16_t finder_port) :
		   _e(e), _fea_target_name(fea_target_name), _p_snd(0),
		   _p_rcv(0), _b_snd(0), _b_rcv(0), _x_err(0) {
	_r = new XrlStdRouter(_e, "test_xrl_socket", finder_host, finder_port);
	set_command_map(_r);
	_r->finalize();
    }

    ~TestSocket4TCP()
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

    /**
     * Stop sending packets.
     */
    void
    stop_sending()
    {
	_t_snd.unschedule();
    }

    /**
     * Send the specified number of bytes of data through the given socket.
     *
     * On success, the number of sent bytes and packets is bumped up
     * accordingly, and true is returned. On failure the number of xrl
     * errors is bumped by one, and the sending timer stopped.
     */
    bool
    send_data(string& sockid, uint32_t bytes)
    {
	vector<uint8_t> data(bytes, FILLER_VALUE);
	XrlSocket4V0p1Client c(_r);
	if (c.send_send(_fea_target_name.c_str(), sockid, data,
		callback(this, &TestSocket4TCP::send_data_cb))) {
	    verbose_log("Sent %u bytes...\n", bytes);
	    _b_snd += bytes;
	    _p_snd += 1;
	    return true;
	}
	return false;
    }


protected:
    virtual bool send_data(uint32_t bytes) = 0;
    virtual void send_data_cb(const XrlError& e) = 0;
public:
    virtual void start_sending(uint32_t bytes, uint32_t ipg_ms) = 0;

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
    socket4_user_0_1_recv_event(const string&		sockid,
				const string&		if_name,
				const string&		vif_name,
				const IPv4&		src_host,
				const uint32_t&		src_port,
				const vector<uint8_t>&	data) = 0;

    XrlCmdError
    socket4_user_0_1_inbound_connect_event(const string& 	sockid,
					   const IPv4&		src_host,
					   const uint32_t&	src_port,
					   const string&	new_sockid,
					   bool&		accept) = 0;

    XrlCmdError
    socket4_user_0_1_outgoing_connect_event(const string& 	sockid) = 0;

    XrlCmdError
    socket4_user_0_1_error_event(const string& 		sockid, 
				 const string&		error,
				 const bool&		fatal) = 0;

    XrlCmdError
    socket4_user_0_1_disconnect_event(const string&	sockid) = 0;

    EventLoop&	_e;
    XrlRouter*	_r;
    string	_fea_target_name;

    uint32_t	_p_snd;		// packets sent
    uint32_t	_p_rcv;		// packets received
    uint32_t	_b_snd;		// bytes sent
    uint32_t	_b_rcv;		// bytes received
    uint32_t	_x_err;		// xrl error count

    XorpTimer	_t_snd;		// send timer
};

//
// ----------------------------------------------------------------------------
// TCP Server

class TestSocket4TCPServer : public TestSocket4TCP {

public:
    TestSocket4TCPServer(EventLoop& e, const string& fea_target_name,
			 IPv4 finder_host, uint16_t finder_port)
	: TestSocket4TCP(e, fea_target_name, finder_host, finder_port),
	  _server_closed(true), _client_closed(true) {}

    bool server_closed() const { return _server_closed; }
    bool client_closed() const { return _client_closed; }

    /**
     * Bind to interface and port.
     *
     * This is an asynchronous request.  If the request is
     * successfully queued for dispatch then true is returned.
     *
     * Subsequently, if the request is successful _server_sockid is set to a
     * valid socket identifier, and _server_closed set to false.
     * If unsuccessful the number of xrl errors is bumped by one.
     */
    bool
    bind(IPv4 addr, uint16_t port)
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending bind (%s:%u) request.\n",
		    addr.str().c_str(), port);
	return c.send_tcp_open_and_bind(
	    _fea_target_name.c_str(), _r->instance_name(), addr, port,
	    callback(this, &TestSocket4TCPServer::bind_cb));
    }

    /**
     * Listen on the server socket.
     *
     * This is an asynchronous request.  If the request is
     * successfully queued for dispatch then true is returned.
     */
    bool
    listen(uint32_t backlog)
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending listen request.\n");
	bool s = c.send_tcp_listen(
	    _fea_target_name.c_str(), _server_sockid, backlog,
	    callback(this, &TestSocket4TCPServer::listen_cb));
	return s;
    }

    /**
     * Request closure of the server socket.
     *
     * This is an asychronous request.  If the request is successfully
     * queued for dispatch then true is returned.
     *
     * On success _server_sockid is cleared.  On failure the number xrl
     * errors is bumped by one.
     */
    bool
    close_server()
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending close request for server socket.\n");
	return c.send_close(_fea_target_name.c_str(), _server_sockid,
	      callback(this, &TestSocket4TCPServer::close_server_cb));
    }

    /**
     * Request closure of the client socket (request a disconnect
     * of the client).
     *
     * This is an asychronous request.  If the request is successfully
     * queued for dispatch then true is returned.
     *
     * On success _client_sockid is cleared.  On failure the number xrl
     * errors is bumped by one.
     */
    bool
    close_client()
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending close request for server client socket.\n");
	return c.send_close(_fea_target_name.c_str(), _client_sockid,
	      callback(this, &TestSocket4TCPServer::close_client_cb));
    }

    /**
     * Send data to the connected client, through the client socket.
     *
     * For implementation, see TestSocket4TCP::send_data.
     */
    bool
    send_data(uint32_t bytes)
    {
	return TestSocket4TCP::send_data(_client_sockid, bytes);
    }

    /**
     * Start sending packets.
     *
     * @param bytes size of each packet.
     * @param ipg_ms interpacket gap in milliseconds.
     */
    void
    start_sending(uint32_t bytes, uint32_t ipg_ms)
    {
	_t_snd = _e.new_periodic_ms(ipg_ms,
	    callback(this, &TestSocket4TCPServer::send_data, bytes));
    }

protected:

    //
    // Callback functions
    //

    void
    bind_cb(const XrlError& e, const string* psockid)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err++;
	    return;
	}
	_server_sockid = *psockid;
	_server_closed = false;
    }

    void
    listen_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err++;
	    return;
	}
    }

    void
    close_server_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err++;
	}
	_server_sockid.erase();
	_server_closed = true;
    }

    void
    close_client_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err++;
	}
	_client_sockid.erase();
	_client_closed = true;
    }

    void
    send_data_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    stop_sending();
	    _x_err++;
	}
    }

    //
    // Socket User Interface functions
    //

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

	// should only be receiving data on the client socket
	XLOG_ASSERT(_client_sockid == sockid);
	verbose_log("Server received %u bytes from %s:%u\n",
		    XORP_UINT_CAST(data.size()), src_host.str().c_str(),
		    src_port);
	_p_rcv += 1;
	for (size_t i = 0; i < data.size(); i++) {
	    if (data[i] != FILLER_VALUE) {
		return XrlCmdError::COMMAND_FAILED("Bad data received.");
	    }
	}
	_b_rcv += data.size();
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_inbound_connect_event(const string&	sockid,
					   const IPv4&		src_host,
					   const uint32_t&	src_port,
					   const string&	new_sockid,
					   bool&		accept)
    {
	// should only be receiving connections from the server socket
	XLOG_ASSERT(sockid == _server_sockid);
	verbose_log("Accepting connection from %s:%d\n",
    			src_host.str().c_str(), src_port);
	accept = true; // Here we would decide whether to accept or reject.
	_client_sockid = new_sockid;
	_client_closed = false;
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_outgoing_connect_event(const string&	sockid)

    {
	// should only be originating connections from the client socket
	XLOG_ASSERT(sockid == _server_sockid);
	return XrlCmdError::COMMAND_FAILED("Not supported");
    }

    XrlCmdError
    socket4_user_0_1_error_event(const string&	sockid,
				 const string&	error,
				 const bool&	fatal)
    {
	// socket must be one of the two...
	XLOG_ASSERT(sockid == _server_sockid || sockid == _client_sockid);
	verbose_log("Server error event on %s socket: %s (fatal = %d)\n",
			(sockid == _server_sockid) ? "server" : "client",
			error.c_str(), fatal);
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_disconnect_event(const string&	sockid)
    {
	// socket must be one of the two...
	XLOG_ASSERT(sockid == _server_sockid || sockid == _client_sockid);
	verbose_log("Server disconnect event on %s socket\n",
	    (sockid == _server_sockid) ? "server" : "client");
	if (sockid == _server_sockid) {
	    _server_closed = true;
	} else {
	    _client_closed = true;
	}
	return XrlCmdError::OKAY();
    }




private:
    string 	_server_sockid;
    string	_client_sockid;
    bool 	_server_closed;	// flag indicating if server socket is closed
    bool	_client_closed;	// flag indicating if client socket is closed
};

//
// ----------------------------------------------------------------------------
// TCP Client

class TestSocket4TCPClient : public TestSocket4TCP {

public:
    TestSocket4TCPClient(EventLoop& e, const string& fea_target_name,
		   IPv4 finder_host, uint16_t finder_port)
	: TestSocket4TCP(e, fea_target_name, finder_host, finder_port),
	  _closed(true), _connected(false) {}

    bool closed() const { return _closed; }
    bool connected() const { return _connected; }

    /**
     * Bind to interface and port, and connect to remote address and port.
     *
     * Subsequently, if the request is successful _sockid is set to
     * a valid socket identifier, and _closed set to false.
     * If unsuccessful the number of xrl errors is bumped by one.
     */
    bool
    bind_connect(IPv4 local_addr, uint16_t local_port, IPv4 remote_addr,
		 uint16_t remote_port)
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending bind (%s/%u) and connect request (\"%s\", %s/%u)\n",
		    local_addr.str().c_str(), local_port,
		    _fea_target_name.c_str(),
		    remote_addr.str().c_str(), remote_port);
	return c.send_tcp_open_bind_connect(
	    _fea_target_name.c_str(), _r->instance_name(), local_addr,
	    local_port, remote_addr, remote_port,
	    callback(this, &TestSocket4TCPClient::bind_connect_cb));
    }

    /**
     * Request closure of the client socket.
     *
     * On success _sockid is cleared.  On failure the number xrl
     * errors is bumped by one.
     */
    bool
    close()
    {
	XrlSocket4V0p1Client c(_r);
	verbose_log("Sending close request for client socket.\n");
	return c.send_close(_fea_target_name.c_str(), _sockid,
	      callback(this, &TestSocket4TCPClient::close_cb));
    }

    /**
     * Send data to the server through the client socket.
     *
     * For implementation, see TestSocket4TCP::send_data.
     */
    bool
    send_data(uint32_t bytes)
    {
	return TestSocket4TCP::send_data(_sockid, bytes);
    }

    /**
     * Start sending packets.
     *
     * @param bytes size of each packet.
     * @param ipg_ms interpacket gap in milliseconds.
     */
    void
    start_sending(uint32_t bytes, uint32_t ipg_ms)
    {
	_t_snd = _e.new_periodic_ms(ipg_ms,
	    callback(this, &TestSocket4TCPClient::send_data, bytes));
    }

protected:

    //
    // Callback functions
    //

    void
    bind_connect_cb(const XrlError& e, const string* psockid)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err++;
	    return;
	}
	_sockid = *psockid;
	_closed = false;
    }

    void
    close_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    _x_err++;
	}
	_sockid.erase();
	_closed = true;
	_connected = false;
    }

    void
    send_data_cb(const XrlError& e)
    {
	if (e != XrlError::OKAY()) {
	    verbose_err("Xrl Error: %s\n", e.str().c_str());
	    stop_sending();
	    _x_err++;
	}
    }

    //
    // Socket User Interface functions
    //

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

	XLOG_ASSERT(_sockid == sockid);
	verbose_log("Client received %u bytes from %s:%u\n",
		    XORP_UINT_CAST(data.size()), src_host.str().c_str(),
		    src_port);
	_p_rcv += 1;
	for (size_t i = 0; i < data.size(); i++) {
	    if (data[i] != FILLER_VALUE) {
		return XrlCmdError::COMMAND_FAILED("Bad data received.");
	    }
	}
	_b_rcv += data.size();
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_inbound_connect_event(const string&    /* sockid */,
					   const IPv4&	    /* src_host */,
					   const uint32_t&  /* src_port */,
					   const string&    /* new_sockid */,
					   bool&	    /* accept */)
    {
	return XrlCmdError::COMMAND_FAILED("Not supported");
    }

    XrlCmdError
    socket4_user_0_1_outgoing_connect_event(const string&	sockid)
    {
	XLOG_ASSERT(_sockid == sockid);
	verbose_log("Client connected to server\n");
	_connected = true;
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_error_event(const string&	sockid,
				 const string&	error,
				 const bool&	fatal)
    {
	XLOG_ASSERT(sockid == _sockid);
	verbose_log("Client socket error: %s (fatal = %d)\n",
		    error.c_str(), fatal);
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    socket4_user_0_1_disconnect_event(const string&	sockid)
    {
	XLOG_ASSERT(sockid == _sockid);
	verbose_log("Client disconnect event\n");
	_closed = true;
	_connected = false;
	return XrlCmdError::OKAY();
    }	

private:
    string _sockid;	// socket used to connect to server
    bool _closed;	// flag indicating whether socket is closed
    bool _connected;	// flag indicating whether the socket is connected
};

//
// ----------------------------------------------------------------------------
// Main test setup functions

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

//
// ----------------------------------------------------------------------------
// Server functions

static void
create_server(EventLoop*	pe,
	      string		fea_target_name,
	      IPv4		finder_host,
	      uint16_t		finder_port,
	      TestSocket4TCP**	ppu)
{
    verbose_log("Creating TestSocket4TCPServer instance.\n");
    *ppu = new TestSocket4TCPServer(*pe, fea_target_name,
				    finder_host, finder_port);
}

static void
bind_server(TestSocket4TCP** ppu, IPv4 addr, uint16_t port, bool *err)
{
    TestSocket4TCPServer* pu = dynamic_cast<TestSocket4TCPServer*>(*ppu);
    bool s = pu->bind(addr, port);
    if (s == false)
	*err = true;
}

static void
listen_server(TestSocket4TCP** ppu, uint16_t backlog, bool *err)
{
    TestSocket4TCPServer* pu = dynamic_cast<TestSocket4TCPServer*>(*ppu);
    bool s = pu->listen(backlog);
    if (s == false)
	*err = true;
}

static void
close_server_socket(TestSocket4TCP** ppu)
{
    verbose_log("Closing server socket.\n");
    TestSocket4TCPServer* pu = dynamic_cast<TestSocket4TCPServer*>(*ppu);
    if (pu->close_server() == false) {
	verbose_err("Failed to send close server socket request.\n");
    }
}

static void
verify_server_closed(TestSocket4TCP** ppu, bool closed, bool* eflag,
		     string location)
{
    TestSocket4TCPServer* pu = dynamic_cast<TestSocket4TCPServer*>(*ppu);
    if (pu->server_closed() != closed) {
	verbose_err("Server socket close state (%s) "
		    "does not matched expected (%s) location %s\n",
		    bool_c_str(pu->server_closed()),
		    bool_c_str(closed),
		    location.c_str());
	*eflag = true;
    }
}

static void
close_server_client_socket(TestSocket4TCP** ppu)
{
    verbose_log("Closing server client socket.\n");
    TestSocket4TCPServer* pu = dynamic_cast<TestSocket4TCPServer*>(*ppu);
    if (pu->close_client() == false) {
	verbose_err("Failed to send server close client socket request.\n");
    }
}

static void
verify_server_client_closed(TestSocket4TCP** ppu, bool closed, bool* eflag,
			    string location)
{
    TestSocket4TCPServer* pu = dynamic_cast<TestSocket4TCPServer*>(*ppu);
    if (pu->client_closed() != closed) {
	verbose_err("Server client socket close state (%s) "
		    "does not matched expected (%s) location %s\n",
		    bool_c_str(pu->client_closed()),
		    bool_c_str(closed),
		    location.c_str());
	*eflag = true;
    }
}

//
// ----------------------------------------------------------------------------
// Client functions

static void
create_client(EventLoop*	pe,
	      string		fea_target_name,
	      IPv4		finder_host,
	      uint16_t		finder_port,
	      TestSocket4TCP**	ppu)
{
    verbose_log("Creating TestSocket4TCPClient instance.\n");
    *ppu = new TestSocket4TCPClient(*pe, fea_target_name,
				    finder_host, finder_port);
}

static void
bind_connect_client(TestSocket4TCP** ppu, IPv4 local_addr,
		    uint16_t local_port, IPv4 remote_addr,
		    uint16_t remote_port, bool *err)
{
    TestSocket4TCPClient* pu = dynamic_cast<TestSocket4TCPClient*>(*ppu);
    bool s = pu->bind_connect(local_addr, local_port, remote_addr,
			      remote_port);
    if (s == false)
	*err = true;
}


static void
close_client_socket(TestSocket4TCP** ppu)
{
    verbose_log("Closing client socket.\n");
    TestSocket4TCPClient* pu = dynamic_cast<TestSocket4TCPClient*>(*ppu);
    if (pu->close() == false) {
	verbose_err("Failed to send close client socket request.\n");
    }
}

static void
verify_client_closed(TestSocket4TCP** ppu, bool closed, bool* eflag,
		     string location)
{
    TestSocket4TCPClient* pu = dynamic_cast<TestSocket4TCPClient*>(*ppu);
    if (pu->closed() != closed) {
	verbose_err("Client socket close state (%s) "
		    "does not matched expected (%s) location %s\n",
		    bool_c_str(pu->closed()),
		    bool_c_str(closed),
		    location.c_str());
	*eflag = true;
    }
}

static void
verify_client_connected(TestSocket4TCP** ppu, bool connected, bool* eflag,
			string location)
{
    TestSocket4TCPClient* pu = dynamic_cast<TestSocket4TCPClient*>(*ppu);
    if (pu->connected() != connected) {
	verbose_err("Client socket connected state (%s) "
		    "does not matched expected (%s) location %s\n",
		    bool_c_str(pu->connected()),
		    bool_c_str(connected),
		    location.c_str());
	*eflag = true;
    }
}

//
// ----------------------------------------------------------------------------
// Common functions


static void
destroy_test_socket(TestSocket4TCP** ppu)
{
    delete *ppu;
    *ppu = 0;
}

static void
start_sending(TestSocket4TCP** ppu)
{
    TestSocket4TCP* pu = *ppu;
    pu->start_sending(512, 200);
}

static void
stop_sending(TestSocket4TCP** ppu)
{
    TestSocket4TCP* pu = *ppu;
    pu->stop_sending();
}

static void
match_bytes_sent_received(TestSocket4TCP** pps /* sender */,
			  TestSocket4TCP** ppr /* receiver */,
			  bool* eflag)
{
    TestSocket4TCP* ps = *pps;
    TestSocket4TCP* pr = *ppr;

    if (ps->bytes_sent() != pr->bytes_received()) {
	verbose_err("Bytes sender sent[%d] do not match bytes receiver "
		    "received[%d]\n", ps->bytes_sent(), pr->bytes_received());
	*eflag = true;
    } else {
    	verbose_log("Bytes sent equals bytes received.\n");
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

    SchedulingTime stime;

    //
    // Create server, bind to port 5000, listen, check states.
    //
    TestSocket4TCP* server;
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(create_server,
					&e, fea_target_name, finder_host,
					finder_port, &server)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_closed, &server,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_client_closed, &server,
					true, &eflag, file_location())));


    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(bind_server, &server,
					IPv4::LOOPBACK(), uint16_t(5000),
					&eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(listen_server, &server,
					uint16_t(2),
					&eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(2000),
			       callback(verify_server_closed, &server,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_client_closed, &server,
					true, &eflag, file_location())));

    //
    // Create client and bind on port 5001, check state.
    //
    TestSocket4TCP* client;
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(create_client,
					&e, fea_target_name, finder_host,
					finder_port, &client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_closed, &client,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(bind_connect_client, &client,
					IPv4::LOOPBACK(), uint16_t(5001),
					IPv4::LOOPBACK(), uint16_t(5000),
					&eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(2000),
			       callback(verify_client_closed, &client,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_connected, &client,
					true, &eflag, file_location())));

    //
    // Send packets from client to server, check bytes send/received and states
    //
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(start_sending, &client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(10000),
			       callback(stop_sending, &client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(match_bytes_sent_received,
					&client, &server, &eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_closed, &client,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_connected, &client,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_closed, &server,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_client_closed, &server,
					false, &eflag, file_location())));

    //
    // Send packets from server to client, check bytes send/received and states
    //
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(start_sending, &server)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(10000),
			       callback(stop_sending, &server)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(match_bytes_sent_received,
					&server, &client, &eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_closed, &client,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_connected, &client,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_closed, &server,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_client_closed, &server,
					false, &eflag, file_location())));

    //
    // Close/disconnect client, verify states
    //
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(close_client_socket,
					&client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_closed, &client,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_client_connected, &client,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_client_closed, &server,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_closed, &server,
					false, &eflag, file_location())));

    //
    // Destroy client and server.
    //
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(destroy_test_socket, &client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(destroy_test_socket, &server)));

    //
    // Create server and client again, and connect as before, and this time
    // use the wildcard port 0 for the client.
    //
    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(create_server,
					&e, fea_target_name, finder_host,
					finder_port, &server)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(bind_server, &server,
    					IPv4::LOOPBACK(), uint16_t(5000),
					&eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(listen_server, &server,
    					uint16_t(2),
					&eflag)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(create_client,
					&e, fea_target_name, finder_host,
					finder_port, &client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(bind_connect_client, &client,
					IPv4::LOOPBACK(), uint16_t(0),
					IPv4::LOOPBACK(), uint16_t(5000),
					&eflag)));

    //
    // Verify states, then close client socket on server
    //
    ev.push_back(e.new_oneoff_after_ms(stime.next(2000),
    			       callback(verify_client_closed, &client,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_client_connected, &client,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_server_closed, &server,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_server_client_closed, &server,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(close_server_client_socket,
					&server)));

    //
    // Verify states, then close the server socket
    //

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_client_closed, &client,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_client_connected, &client,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_server_closed, &server,
					false, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
    			       callback(verify_server_client_closed, &server,
					true, &eflag, file_location())));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
				callback(close_server_socket,
					 &server)));

    //
    // Verify states, then destroy server and client
    //

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(verify_server_closed, &server,
					true, &eflag, file_location())));

    //
    // Destroy socket server.
    //

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(destroy_test_socket, &client)));

    ev.push_back(e.new_oneoff_after_ms(stime.next(),
			       callback(destroy_test_socket, &server)));

    //
    // Force exit.
    //
    bool finish(false);
    ev.push_back(e.set_flag_after_ms(stime.next(1000), &finish));

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
    fprintf(stderr, "Runs remote TCP socket test using Xrls.\n");
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
    // XXX: verbosity of the error messages temporarily increased
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
