// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rip/test_request.cc,v 1.11 2004/06/10 22:41:47 hodson Exp $"

#include <set>

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"

#include "auth.hh"
#include "port.hh"
#include "peer.hh"
#include "route_db.hh"
#include "system.hh"

#include "test_utils.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_request";
static const char *program_description  = "Test RIP handling route requests";
static const char *program_version_id   = "0.1";
static const char *program_date         = "July, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";


// ----------------------------------------------------------------------------
// Spoof Port that supports just a single Peer
//

template <typename A>
class SpoofPort : public Port<A> {
public:
    SpoofPort(PortManagerBase<A>& pm, A addr) : Port<A>(pm)
    {
	this->_peers.push_back(new Peer<A>(*this, addr));
	verbose_log("Constructing SpoofPort instance\n");
	this->set_enabled(true);
    }
    ~SpoofPort()
    {
	verbose_log("Destructing SpoofPort instance\n");
	while (this->_peers.empty() == false) {
	    delete this->_peers.front();
	    this->_peers.pop_front();
	}
    }
};


// ----------------------------------------------------------------------------
// Type specific helpers

template <typename A>
struct DefaultPeer {
    static A get();
};

template <>
IPv4 DefaultPeer<IPv4>::get() { return IPv4("10.0.0.1"); }

template <>
IPv6 DefaultPeer<IPv6>::get() { return IPv6("10::1"); }

// ----------------------------------------------------------------------------
// Spoof Port Manager instance support a single Spoof Port which in turn
// contains a single Peer.
//

template <typename A>
class SpoofPortManager : public PortManagerBase<A> {
public:
    SpoofPortManager(System<A>& s)
	: PortManagerBase<A>(s)
    {
	this->_ports.push_back(new SpoofPort<A>(*this, DefaultPeer<A>::get()));
    }

    ~SpoofPortManager()
    {
	while (!this->_ports.empty()) {
	    delete this->_ports.front();
	    this->_ports.pop_front();
	}
    }

    Port<A>* the_port()
    {
	XLOG_ASSERT(this->_ports.size() == 1);
	return this->_ports.front();
    }

    Peer<A>* the_peer()
    {
	XLOG_ASSERT(this->_ports.size() == 1);
	XLOG_ASSERT(this->_ports.front()->peers().size() == 1);
	return this->_ports.front()->peers().front();
    }
};

// ----------------------------------------------------------------------------
// Test PortIO class

template <typename A>
class SpoofPortIO : public PortIOBase<A>
{
public:
    typedef A			Addr;
    typedef PortIOUserBase<A>	PortIOUser;

public:
    SpoofPortIO(PortIOUserBase<A>& user)
	: PortIOBase<A>(user, "if0", "vif0", IPv4("10.0.0.1"))
    {
	last_rip_send_flush();
    }

    /**
     * Called by RIP Port instance.
     */
    bool send(const Addr&    addr,
	      uint16_t	     port,
	      const vector<uint8_t>& rip_packet)
    {
	_lo_addr = addr;
	_lo_port = port;
	_lo_data = rip_packet;
	_pending = true;
	return true;
    }

    bool pending() const
    {
	return _lo_port != 0;
    }

    const Addr&	   last_rip_send_addr() const		{ return _lo_addr; }
    const uint16_t last_rip_send_port() const		{ return _lo_port; }
    const vector<uint8_t>& last_rip_send_data() const	{ return _lo_data; }

    void last_rip_send_flush()
    {
	_lo_addr = A::ZERO();
	_lo_port = 0;
	_lo_data.resize(0);
	_pending = false;
    }

private:
    vector<uint8_t>	_lo_data;
    Addr		_lo_addr;
    uint16_t		_lo_port;
    bool		_pending;
};


static const IPv4	REQUESTING_HOST = IPv4("10.0.100.1");
static const uint16_t	REQUESTING_PORT = 1092;
static const uint8_t	ROUTE_COST	= 5;

class RequestPacketTester {
public:
    static const uint32_t INJECTED_ROUTES  = 10;
    static const uint32_t REQUESTED_ROUTES = 25;

public:
    RequestPacketTester() : _e(), _rip_system(_e), _pm(_rip_system)
    {
	_portio	= new SpoofPortIO<IPv4>(*_pm.the_port());
	_pm.the_port()->set_io_handler(_portio, false);
	_portio->set_enabled(true);
    }

    ~RequestPacketTester()
    {
	_portio->set_enabled(false);
	delete _portio;
	RouteDB<IPv4>& rdb = _rip_system.route_db();
	rdb.flush_routes();
    }

    bool init_nets()
    {
	RouteDB<IPv4>& rdb = _rip_system.route_db();
	make_nets(_testnets, REQUESTED_ROUTES);

	set<IPv4Net>::const_iterator n = _testnets.begin();
	for (uint32_t i = 0; i < INJECTED_ROUTES; i++) {
	    if (rdb.update_route(*n, IPv4::ZERO(), ROUTE_COST, 0,
				 _pm.the_peer(),PolicyTags()) == false) {
		verbose_log("Failed to add route for %s\n",
			    n->str().c_str());
		return false;
	    }
	    n++;
	}
	return true;
    }

    bool
    send_rip_route_queries()
    {
	vector<uint8_t> buf;
	buf.resize(sizeof(RipPacketHeader) +
		   REQUESTED_ROUTES * sizeof(PacketRouteEntry<IPv4>));

	RipPacketHeader* rph = new (&(buf[0])) RipPacketHeader();
	rph->initialize(RipPacketHeader::REQUEST, 2);

	set<IPv4Net>::const_iterator n = _testnets.begin();
	for (uint32_t i = 0; i < REQUESTED_ROUTES; i++) {
	    assert(n != _testnets.end());
	    uint32_t offset = sizeof(RipPacketHeader) +
		i * sizeof(PacketRouteEntry<IPv4>);

	    PacketRouteEntry<IPv4>* pre =
		new (&(buf[offset])) PacketRouteEntry<IPv4>;

	    pre->initialize(0, *n, IPv4::ZERO(), 0);
	    n++;
	}
	assert(_pm.the_port() != 0);
	_pm.the_port()->port_io_receive(REQUESTING_HOST, REQUESTING_PORT,
					&(buf[0]), buf.size());
	return true;
    }

    bool
    check_response()
    {
	if (_portio->pending() == false) {
	    verbose_log("No response packet sent by rip\n");
	    return false;
	}
	if (_portio->last_rip_send_addr() != REQUESTING_HOST) {
	    verbose_log("Response was not sent to originator\n");
	    return false;
	}
	if (_portio->last_rip_send_port() != REQUESTING_PORT) {
	    verbose_log("Response was not sent to originators port\n");
	    return false;
	}

	const vector<uint8_t> buf = _portio->last_rip_send_data();

	// Validate RIP packet header
	const RipPacketHeader* rph
	    = reinterpret_cast<const RipPacketHeader*>(&buf[0]);

	if (rph->valid_command() == false) {
	    verbose_log("Invalid command\n");
	    return false;
	}

	if (rph->valid_version(RipPacketHeader::IPv4_VERSION) == false) {
	    verbose_log("Invalid version\n");
	    return false;
	}

	if (rph->valid_padding() == false) {
	    verbose_log("Invalid padding\n");
	    return false;
	}

	if (rph->command != RipPacketHeader::RESPONSE) {
	    verbose_log("Not a response packet\n");
	    return false;
	}

	// Validate entries
	const PacketRouteEntry<IPv4>* pre =
	    reinterpret_cast<const PacketRouteEntry<IPv4>*>(rph + 1);
	uint32_t n_entries = buf.size() / sizeof (PacketRouteEntry<IPv4>) - 1;

	if (n_entries > _testnets.size()) {
	    verbose_log("Got more routes than requested (%u > %u).\n",
			n_entries, (uint32_t)_testnets.size());
	    return false;
	}

	set<IPv4Net>::const_iterator ni = _testnets.begin();

	for (uint32_t i = 0; i < n_entries; i++, pre++) {
	    verbose_log("%s %s %d %d\n",
			pre->net().str().c_str(),
			pre->nexthop().str().c_str(),
			pre->metric(),
			pre->tag());
	    if (pre->addr_family() != PacketRouteEntry<IPv4>::ADDR_FAMILY) {
		verbose_log("Invalid address family in route entry %d\n", i);
		return false;
	    }
	    if (*ni != pre->net()) {
		verbose_log("Mismatched net in route entry %d\n", i);
		return false;
	    }
	    if (i < INJECTED_ROUTES) {
		if (pre->metric() != ROUTE_COST) {
		    verbose_log("Metric changed.\n");
		    return false;
		}
	    } else {
		if (pre->metric() != RIP_INFINITY) {
		    verbose_log("Non-existant route with finite metric??\n");
		}
	    }
	    ni++;
	}
	return true;
    }

    int run_test()
    {
	if (init_nets() == false) {
	    return -1;
	}

	bool start_test = false;
	XorpTimer d = _e.set_flag_after_ms(1 * 1000, &start_test);
	while (start_test == false) {
	    _e.run();
	}

	if (send_rip_route_queries() == false) {
	    return -1;
	}

	if (check_response() == false) {
	    return -1;
	}
	return 0;
    }

protected:
    EventLoop			_e;
    System<IPv4>		_rip_system;
    SpoofPortManager<IPv4>	_pm;
    SpoofPortIO<IPv4>*		_portio;
    set<IPv4Net>		_testnets;
};


/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/*
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "usage: %s [-v] [-h]\n", progname);
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

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

    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'v':
            set_verbose(true);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            xlog_stop();
            xlog_exit();
            if (ch == 'h')
                return (0);
            else
                return (1);
        }
    }
    argc -= optind;
    argv += optind;

    int rval = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	RequestPacketTester rpt;
	rval = rpt.run_test();
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        rval = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return rval;
}
