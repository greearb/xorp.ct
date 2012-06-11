// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



#include <iomanip>

#include "rip/rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ref_ptr.hh"
#include "libxorp/service.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/rip_xif.hh"
#include "xrl/interfaces/ripng_xif.hh"

#include "common.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static const char* NO_PEERS = "There are no known peers.";
static const char* NO_PEERS_ON_ADDR = "There are no known peers on ";

// ----------------------------------------------------------------------------
// Utility methods

static void
usage()
{
    fprintf(stderr, "Usage: %s [options] (rip|ripng) [<interface> <vif> <addr> [<peer>]]\n",
	    xlog_process_name());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t -1 "
	    "Single line output (used by 'show rip peer')\n");
    fprintf(stderr, "\t -F <finder_host>:<finder_port> "
	    "Specify Finder host and port to use.\n");
    fprintf(stderr, "\t -T <targetname>                "
	    "Specify XrlTarget to query.\n\n");
    exit(-1);
}

template <typename A>
static void
print_peer_header(const A& 	peer,
		  const string& ifn,
		  const string& vifn,
		  const A& 	addr)
{
    cout << endl;
    cout << "* RIP statistics for peer " << peer.str() << " on "
	 << ifn << " " << vifn << " " << addr.str() << endl << endl;
}

static void
pretty_print_counters(const XrlAtomList& descriptions,
		      const XrlAtomList& values,
		      uint32_t		 last_active)
{
    static const uint32_t COL1 = 32;
    static const uint32_t COL2 = 16;

    time_t when = static_cast<time_t>(last_active);
    char whenbuf[12];
    (void)strftime(&whenbuf[0], sizeof(whenbuf), "%H:%M:%S", gmtime(&when));

    // Save flags
    ios::fmtflags fl = cout.flags();

    cout << "  ";
    cout << "Last Active at " << whenbuf << endl;

    cout.flags(ios::left);
    cout << "  ";
    cout << setw(COL1) << "Counter" << setw(0) << " ";
    cout.flags(ios::right);
    cout << setw(COL2) << "Value" << endl;

    cout.flags(ios::left);
    cout << "  ";
    cout << setw(COL1) << string(COL1, '-') << setw(0) << " ";
    cout.flags(ios::right);
    cout << setw(COL2) << string(COL2, '-') << endl;

    for (size_t i = 0; i != descriptions.size(); ++i) {

	const XrlAtom& d = descriptions.get(i);
	const XrlAtom& v = values.get(i);

	cout.flags(ios::left);
	cout << setw(0) << "  ";
	// NB we use string.c_str() here as GCC's string ostream
	// renderer seems to ignore the field width.  Formatting
	// ostreams is supremely fugly.
	cout << setw(COL1) << d.text().c_str();
	cout << setw(0) << " ";

	cout.flags(ios::right);
	cout << setw(COL2) << v.uint32() << endl;
    }

    // Restore flags
    cout.flags(fl);
}

template <typename A>
static void
pretty_print_counters_single_line(const XrlAtomList& descriptions,
				  const XrlAtomList& values,
				  uint32_t	     last_active,
				  const A& 	     peer,
				  const string&      ifn,
				  const string&      vifn)
{
    time_t when = static_cast<time_t>(last_active);
    char whenbuf[12];
    (void)strftime(&whenbuf[0], sizeof(whenbuf), "%H:%M:%S", gmtime(&when));

    // RIP sends the descriptions of each counter back,
    // we will need to scan the atom list for what we want.
    uint32_t rxcount = 0;
    for (size_t i = 0; i != descriptions.size(); ++i) {
	const XrlAtom& d = descriptions.get(i);
	const XrlAtom& v = values.get(i);
	if (0 == strcasecmp(d.text().c_str(), "Request Packets Received")) {
		rxcount = v.uint32();
		break;
	}
    }

    ios::fmtflags fl = cout.flags();
    cout.flags(ios::left);

    // XXX We are using C++ style formatting here. I would prefer to use
    // fprintf because you can just bang that out. This is somewhat tedious.
    cout << setw(17) << peer.str() <<
            setw(4) << ifn << setw(1) << "/" <<
            setw(12) << vifn <<
            setw(8) << "Up" <<
            setw(12) << rxcount <<	// receive count (not held)
            setw(12) << 0 <<		// transmit count XXX notyet
            setw(10) << (last_active == 0 ? "Never" : whenbuf) << endl;


    cout.flags(fl);
}


/**
 * Invoke Xrl to get peer stats on RIP address and pretty print result.
 */
class GetPeerStats4 : public XrlJobBase {
public:
    GetPeerStats4(XrlJobQueue&	jq,
		     const string&	ifname,
		     const string&	vifname,
		     IPv4		addr,
		     IPv4		peer,
		     bool		single_line = false,
		     bool		hide_errors = false)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr), _p(peer),
	  _single_line(single_line), _hide_errs(hide_errors)
    {}

    bool
    dispatch()
    {
	XrlRipV0p1Client cl(queue().sender());
	return cl.send_get_peer_counters(queue().target().c_str(),
					 _ifn, _vifn, _a, _p,
					 callback(this,
						  &GetPeerStats4::cmd_callback)
					 );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	descriptions,
		 const XrlAtomList*	values,
		 const uint32_t*	peer_last_active)
    {
	if (xe == XrlError::OKAY()) {
	    if (_single_line) {
		pretty_print_counters_single_line(*descriptions, *values,
		    *peer_last_active, _p, _ifn, _vifn);
	    } else {
		print_peer_header(_p, _ifn, _vifn, _a);
		pretty_print_counters(*descriptions, *values,
		    *peer_last_active);
	    }
	    queue().dispatch_complete(xe, this);
	} else if (_hide_errs) {
	    // When invoked by GetAllPeerStats4 or GetPortPeerStats4
	    // we do not report an error.  They queried RIP to get the
	    // complete peers list and if we get here the specified
	    // port has been garbage collected at the time it's stats
	    // are queried.
	    queue().dispatch_complete(XrlError::OKAY(), this);
	} else {
	    queue().dispatch_complete(xe, this);
	}
    }

protected:
    string 	_ifn;
    string 	_vifn;
    IPv4   	_a;
    IPv4 	_p;
    bool	_single_line;
    bool	_hide_errs;
};

/**
 * Invoke Xrl to get all peers, which we then use to get the counters
 * for to pretty print result.
 */
class GetAllPeerStats4 : public XrlJobBase {
public:
    GetAllPeerStats4(XrlJobQueue& jq, bool single_line = false)
	: XrlJobBase(jq), _single_line(single_line)
    {
    }

    bool
    dispatch()
    {
	XrlRipV0p1Client cl(queue().sender());
	return cl.send_get_all_peers(queue().target().c_str(),
				     callback(this,
					      &GetAllPeerStats4::cmd_callback)
				     );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	peers,
		 const XrlAtomList*	ifnames,
		 const XrlAtomList*	vifnames,
		 const XrlAtomList*	addrs)
    {
	if (xe == XrlError::OKAY()) {
	    if (peers->size() == 0) {
		cout << NO_PEERS << endl;
	    } else {
		for (size_t i = 0; i < peers->size(); i++) {
		    const IPv4& 	peer_addr = peers->get(i).ipv4();
		    const string& 	ifn 	  = ifnames->get(i).text();
		    const string& 	vifn 	  = vifnames->get(i).text();
		    const IPv4& 	addr 	  = addrs->get(i).ipv4();
		    queue().enqueue(
				    new GetPeerStats4(queue(), ifn, vifn,
						      addr, peer_addr,
						      _single_line, true)
				    );
		}
	    }
	} else {
	    cerr << xe.str() << endl;
	}
	queue().dispatch_complete(xe, this);
    }

    bool	_single_line;
};

/**
 * Invoke Xrl to get peers on if/vif/addr, which we then use to get
 * the counters for to pretty print result.
 */
class GetPortPeerStats4 : public XrlJobBase {
public:
    GetPortPeerStats4(XrlJobQueue& 	jq,
		      const string& 	ifname,
		      const string& 	vifname,
		      const IPv4&	addr)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr)
    {}

    bool
    dispatch()
    {
	XrlRipV0p1Client cl(queue().sender());
	return cl.send_get_all_peers(queue().target().c_str(),
			callback(this, &GetPortPeerStats4::cmd_callback));
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	peers,
		 const XrlAtomList*	ifnames,
		 const XrlAtomList*	vifnames,
		 const XrlAtomList*	addrs)
    {
	if (xe == XrlError::OKAY()) {
	    if (peers->size() == 0) {
		cout << NO_PEERS_ON_ADDR << _ifn << " " << _vifn << " "
		     << _a.str() << endl;
	    } else {
		for (size_t i = 0; i < peers->size(); i++) {
		    const IPv4& 	peer_addr = peers->get(i).ipv4();
		    const string& 	ifn 	  = ifnames->get(i).text();
		    const string& 	vifn 	  = vifnames->get(i).text();
		    const IPv4& 	addr 	  = addrs->get(i).ipv4();

		    if (ifn == _ifn && vifn == _vifn && _a == addr) {
			queue().enqueue(
					new GetPeerStats4(queue(), ifn, vifn,
							  addr, peer_addr,
							  false, true)
					);
		    }
		}
	    }
	}
	queue().dispatch_complete(xe, this);
    }

protected:
    string 	_ifn;
    string 	_vifn;
    IPv4 	_a;
};

/**
 * Invoke Xrl to get peer stats on RIP address and pretty print result.
 */
class GetPeerStats6 : public XrlJobBase {
public:
    GetPeerStats6(XrlJobQueue&	jq,
		     const string&	ifname,
		     const string&	vifname,
		     IPv6		addr,
		     IPv6		peer,
		     bool		single_line = false,
		     bool		hide_errors = false)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr), _p(peer),
	  _single_line(single_line), _hide_errs(hide_errors)
    {}

    bool
    dispatch()
    {
	XrlRipngV0p1Client cl(queue().sender());
	return cl.send_get_peer_counters(queue().target().c_str(),
					 _ifn, _vifn, _a, _p,
					 callback(this,
						  &GetPeerStats6::cmd_callback)
					 );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	descriptions,
		 const XrlAtomList*	values,
		 const uint32_t*	peer_last_active)
    {
	if (xe == XrlError::OKAY()) {
	    if (_single_line) {
		pretty_print_counters_single_line(*descriptions, *values,
		    *peer_last_active, _p, _ifn, _vifn);
	    } else {
		print_peer_header(_p, _ifn, _vifn, _a);
		pretty_print_counters(*descriptions, *values,
		    *peer_last_active);
	    }
	    queue().dispatch_complete(xe, this);
	} else if (_hide_errs) {
	    // When invoked by GetAllPeerStats6 or GetPortPeerStats6
	    // we do not report an error.  They queried RIP to get the
	    // complete peers list and if we get here the specified
	    // port has been garbage collected at the time it's stats
	    // are queried.
	    queue().dispatch_complete(XrlError::OKAY(), this);
	} else {
	    queue().dispatch_complete(xe, this);
	}
    }

protected:
    string 	_ifn;
    string 	_vifn;
    IPv6   	_a;
    IPv6 	_p;
    bool	_single_line;
    bool	_hide_errs;
};

/**
 * Invoke Xrl to get all peers, which we then use to get the counters
 * for to pretty print result.
 */
class GetAllPeerStats6 : public XrlJobBase {
public:
    GetAllPeerStats6(XrlJobQueue& jq, bool single_line = false)
	: XrlJobBase(jq), _single_line(single_line)
    {}

    bool
    dispatch()
    {
	XrlRipngV0p1Client cl(queue().sender());
	return cl.send_get_all_peers(queue().target().c_str(),
				     callback(this,
					      &GetAllPeerStats6::cmd_callback)
				     );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	peers,
		 const XrlAtomList*	ifnames,
		 const XrlAtomList*	vifnames,
		 const XrlAtomList*	addrs)
    {
	if (xe == XrlError::OKAY()) {
	    if (peers->size() == 0) {
		cout << NO_PEERS << endl;
	    } else {
		for (size_t i = 0; i < peers->size(); i++) {
		    const IPv6& 	peer_addr = peers->get(i).ipv6();
		    const string& 	ifn 	  = ifnames->get(i).text();
		    const string& 	vifn 	  = vifnames->get(i).text();
		    const IPv6& 	addr 	  = addrs->get(i).ipv6();
		    queue().enqueue(
				    new GetPeerStats6(queue(), ifn, vifn,
						      addr, peer_addr,
						      _single_line, true)
				    );
		}
	    }
	}
	queue().dispatch_complete(xe, this);
    }

    bool	_single_line;
};

/**
 * Invoke Xrl to get peers on if/vif/addr, which we then use to get
 * the counters for to pretty print result.
 */
class GetPortPeerStats6 : public XrlJobBase {
public:
    GetPortPeerStats6(XrlJobQueue& 	jq,
		      const string& 	ifname,
		      const string& 	vifname,
		      const IPv6&	addr)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr)
    {}

    bool
    dispatch()
    {
	XrlRipngV0p1Client cl(queue().sender());
	return cl.send_get_all_peers(queue().target().c_str(),
			callback(this, &GetPortPeerStats6::cmd_callback));
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	peers,
		 const XrlAtomList*	ifnames,
		 const XrlAtomList*	vifnames,
		 const XrlAtomList*	addrs)
    {
	if (xe == XrlError::OKAY()) {
	    if (peers->size() == 0) {
		cout << NO_PEERS_ON_ADDR << _ifn << " " << _vifn << " "
		     << _a.str() << endl;
	    } else {
		for (size_t i = 0; i < peers->size(); i++) {
		    const IPv6& 	peer_addr = peers->get(i).ipv6();
		    const string& 	ifn 	  = ifnames->get(i).text();
		    const string& 	vifn 	  = vifnames->get(i).text();
		    const IPv6& 	addr 	  = addrs->get(i).ipv6();

		    if (ifn == _ifn && vifn == _vifn && _a == addr) {
			queue().enqueue(
					new GetPeerStats6(queue(), ifn, vifn,
							  addr, peer_addr,
							  false, true)
					);
		    }
		}
	    }
	}
	queue().dispatch_complete(xe, this);
    }

protected:
    string 	_ifn;
    string 	_vifn;
    IPv6 	_a;
};


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
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	bool		do_single_line = false;
	bool		do_run 	    = true;
	string          finder_host = FinderConstants::FINDER_DEFAULT_HOST().str();
        uint16_t        finder_port = FinderConstants::FINDER_DEFAULT_PORT();
	string		xrl_target;

	int ch;
	while ((ch = getopt(argc, argv, "1F:T:")) != -1) {
	    switch (ch) {
	    case '1':
		do_single_line = true;
		break;
	    case 'F':
		do_run = parse_finder_args(optarg, finder_host, finder_port);
		break;
	    case 'T':
		xrl_target = optarg;
		break;
	    default:
		usage();
		do_run = false;
	    }
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
	    usage();
	}

	uint32_t ip_version = rip_name_to_ip_version(argv[0]);
	if (ip_version == 0) {
	    usage();
	}
	argc -= 1;
	argv += 1;

	if (xrl_target.empty()) {
	    const char* xt = default_xrl_target(ip_version);
	    if (xt == 0) {
		usage();
	    }
	    xrl_target = xt;
	}

	if (do_run) {
	    EventLoop e;
	    XrlJobQueue job_queue(e, finder_host, finder_port, xrl_target);

	    if (do_single_line) {
//  Address    Interface   State  Hello Rx  Hello Tx  Last Hello" << endl;
		ios::fmtflags fl = cout.flags();
		cout.flags(ios::left);
	        cout << setw(17) << "  Address" <<
		        setw(17) << "Interface" <<
		        setw(8) << "State" <<
		        setw(12) << "Hello Rx" <<
		        setw(12) << "Hello Tx" <<
		        setw(10) << "Last Hello" <<
		        endl;
		cout.flags(fl);
	    }

	    if (argc == 4) {
		if (ip_version == 4) {
		    job_queue.enqueue(
			new GetPeerStats4(job_queue,
					  argv[0], argv[1], argv[2], argv[3],
					  do_single_line)
			);
		} else if (ip_version == 6) {
		    job_queue.enqueue(
			new GetPeerStats6(job_queue,
					  argv[0], argv[1], argv[2], argv[3],
					  do_single_line)
			);
		}
	    } else if (argc == 3) {
		if (ip_version == 4) {
		    job_queue.enqueue(
			new GetPortPeerStats4(job_queue,
					      argv[0], argv[1], argv[2])
			);
		} else if (ip_version == 6) {
		    job_queue.enqueue(
			new GetPortPeerStats6(job_queue,
					      argv[0], argv[1], argv[2])
			);
		}
	    } else if (argc == 0) {
		if (ip_version == 4) {
		    job_queue.enqueue(new GetAllPeerStats4(job_queue,
							   do_single_line));
		} else if (ip_version == 6) {
		    job_queue.enqueue(new GetAllPeerStats6(job_queue,
							   do_single_line));
		}
	    } else {
		usage();
	    }

	    job_queue.startup();
	    while (job_queue.status() != SERVICE_SHUTDOWN) {
		if (job_queue.status() == SERVICE_FAILED) {
		    cerr << "Failed: " << job_queue.status_note() << endl;
		    break;
		}
		e.run();
	    }
	}
    } catch (...) {
	xorp_print_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
