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

#ident "$XORP: xorp/rip/tools/show_peer_stats.cc,v 1.1 2004/03/11 00:04:20 hodson Exp $"

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

// ----------------------------------------------------------------------------
// Utility methods

static uint32_t
guess_ip_version(int argc, char* const argv[])
{
    for (int i = 0; i < argc; i++) {
	if (strcmp(argv[i], "rip") == 0) {
	    return 4;
	}
	if (strcmp(argv[i], "ripng") == 0) {
	    return 6;
	}
	// Try parsing argument as an IPv4 or IPv6 address
	try {
	    IPv4 a(argv[i]);
	    return a.ip_version();
	}
	catch (...) {
	    try {
		IPv6 a(argv[i]);
		return a.ip_version();
	    }
	    catch (...) {}
	}
    }
    return 4;
}

static const char*
default_xrl_target(uint32_t ip_version)
{
    if (ip_version == 6) {
	return "ripng";
    } else {
	return "rip";
    }
}

static void
usage()
{
    fprintf(stderr, "%s [options] [<interface> <vif> <addr> <peer>]\n",
	    xlog_process_name());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t -F <finder_host>:<finder_port> "
	    "Specify Finder host and port to use.\n");
    fprintf(stderr, "\t -T <targetname>                "
	    "Specify XrlTarget to query.\n\n");
}

static bool
parse_finder_args(const string& host_colon_port, string& host, uint16_t& port)
{
    string::size_type sp = host_colon_port.find(":");
    if (sp == string::npos) {
        host = host_colon_port;
        // Do not set port, by design it has default finder port value.
    } else {
        host = string(host_colon_port, 0, sp);
        string s_port = string(host_colon_port, sp + 1, 14);
        uint32_t t_port = atoi(s_port.c_str());
        if (t_port == 0 || t_port > 65535) {
            XLOG_ERROR("Finder port %d is not in range 1--65535.\n", t_port);
            return false;
        }
        port = (uint16_t)t_port;
    }
    return true;
}

template <typename A>
static void
print_peer_header(const A& 	peer,
		  const string& ifn,
		  const string& vifn,
		  const A& 	addr)
{
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

    // Save flags
    ios::fmtflags fl = cout.flags();

    time_t when(last_active);
    cout << "  ";
    cout << "Last Active at " << ctime(&when) << endl;

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


// ----------------------------------------------------------------------------
// Job classes and related

class XrlJobBase;

/**
 * Base class for Xrl Job Queue.
 */
class XrlJobQueueBase {
public:
    typedef ref_ptr<XrlJobBase> Job;

public:
    virtual ~XrlJobQueueBase() {}
    virtual XrlSender*	  sender()	 = 0;
    virtual const string& target() const = 0;

    virtual void enqueue(const Job& j)	 = 0;

    virtual void dispatch_complete(const XrlError&	xe,
				   const XrlJobBase*	cmd) = 0;
};

/**
 * Base class for Xrl Jobs that are invoked by classes derived
 * from XrlJobQueueBase.
 */
class XrlJobBase : public CallbackSafeObject {
public:
    XrlJobBase(XrlJobQueueBase& q) : _q(q) {}

    virtual ~XrlJobBase() {}
    virtual bool dispatch() = 0;

protected:
    XrlJobQueueBase& queue() { return _q; }

private:
    XrlJobBase(const XrlJobBase&);		// Not implemented
    XrlJobBase& operator=(const XrlJobBase&);	// Not implemented

private:
    XrlJobQueueBase& _q;
};

/**
 * Invoke Xrl to get peer stats on RIP address and pretty print result.
 */
class GetPeerStats4 : public XrlJobBase {
public:
    GetPeerStats4(XrlJobQueueBase&	jq,
		     const string&	ifname,
		     const string&	vifname,
		     IPv4		addr,
		     IPv4		peer,
		     bool		hide_errors = false)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr), _p(peer),
	  _hide_errs(hide_errors)
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
	    print_peer_header(_p, _ifn, _vifn, _a);
	    pretty_print_counters(*descriptions, *values, *peer_last_active);
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
    bool	_hide_errs;
};

/**
 * Invoke Xrl to get all peers, which we then use to get the counters
 * for to pretty print result.
 */
class GetAllPeerStats4 : public XrlJobBase {
public:
    GetAllPeerStats4(XrlJobQueueBase& jq)
	: XrlJobBase(jq)
    {}

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
	    for (size_t i = 0; i < peers->size(); i++) {
		const IPv4& 	peer_addr = peers->get(i).ipv4();
		const string& 	ifn 	  = ifnames->get(i).text();
		const string& 	vifn 	  = vifnames->get(i).text();
		const IPv4& 	addr 	  = addrs->get(i).ipv4();
		queue().enqueue(
			new GetPeerStats4(queue(), ifn, vifn, addr, peer_addr,
					  true)
			);
	    }
	}
	queue().dispatch_complete(xe, this);
    }
};

/**
 * Invoke Xrl to get peers on if/vif/addr, which we then use to get
 * the counters for to pretty print result.
 */
class GetPortPeerStats4 : public XrlJobBase {
public:
    GetPortPeerStats4(XrlJobQueueBase& 	jq,
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
	    for (size_t i = 0; i < peers->size(); i++) {
		const IPv4& 	peer_addr = peers->get(i).ipv4();
		const string& 	ifn 	  = ifnames->get(i).text();
		const string& 	vifn 	  = vifnames->get(i).text();
		const IPv4& 	addr 	  = addrs->get(i).ipv4();

		if (ifn == _ifn && vifn == _vifn && _a == addr) {
		    queue().enqueue(
			new GetPeerStats4(queue(), ifn, vifn, addr, peer_addr,
					  true)
			);
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
    GetPeerStats6(XrlJobQueueBase&	jq,
		     const string&	ifname,
		     const string&	vifname,
		     IPv6		addr,
		     IPv6		peer,
		     bool		hide_errors = false)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr), _p(peer),
	  _hide_errs(hide_errors)
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
	    print_peer_header(_p, _ifn, _vifn, _a);
	    pretty_print_counters(*descriptions, *values, *peer_last_active);
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
    bool	_hide_errs;
};

/**
 * Invoke Xrl to get all peers, which we then use to get the counters
 * for to pretty print result.
 */
class GetAllPeerStats6 : public XrlJobBase {
public:
    GetAllPeerStats6(XrlJobQueueBase& jq)
	: XrlJobBase(jq)
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
	    for (size_t i = 0; i < peers->size(); i++) {
		const IPv6& 	peer_addr = peers->get(i).ipv6();
		const string& 	ifn 	  = ifnames->get(i).text();
		const string& 	vifn 	  = vifnames->get(i).text();
		const IPv6& 	addr 	  = addrs->get(i).ipv6();
		queue().enqueue(
			new GetPeerStats6(queue(), ifn, vifn, addr, peer_addr,
					  true)
			);
	    }
	}
	queue().dispatch_complete(xe, this);
    }
};

/**
 * Invoke Xrl to get peers on if/vif/addr, which we then use to get
 * the counters for to pretty print result.
 */
class GetPortPeerStats6 : public XrlJobBase {
public:
    GetPortPeerStats6(XrlJobQueueBase& 	jq,
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
	    for (size_t i = 0; i < peers->size(); i++) {
		const IPv6& 	peer_addr = peers->get(i).ipv6();
		const string& 	ifn 	  = ifnames->get(i).text();
		const string& 	vifn 	  = vifnames->get(i).text();
		const IPv6& 	addr 	  = addrs->get(i).ipv6();

		if (ifn == _ifn && vifn == _vifn && _a == addr) {
		    queue().enqueue(
			new GetPeerStats6(queue(), ifn, vifn, addr, peer_addr,
					  true)
			);
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


/**
 * Xrl Job Queue.
 *
 * Executes a list of Xrl command objects.
 *
 * At start up, instances poll on the ready state of their internal
 * XrlRouter.  If the XrlRouter becomes ready, polling is ceased, and
 * Xrl command objects executed until there are no more left or one
 * fails by reporting a return status not of XrlError::OKAY().  If the
 * XrlRouter does not become ready within a preset time, the Job Queue
 * enters the FAILED state of the ServiceBase class.
 */
class XrlJobQueue
    : public XrlJobQueueBase, public ServiceBase
{
public:
    typedef XrlJobQueueBase::Job Job;

public:
    XrlJobQueue(EventLoop& 	e,
		const string& 	finder_host,
		uint16_t 	finder_port,
		const string& 	tgtname)
	: _e(e), _fhost(finder_host), _fport(finder_port), _tgt(tgtname),
	  _rtr(0), _rtr_poll_cnt(0)
    {
	set_status(READY);
    }

    ~XrlJobQueue()
    {
	delete _rtr;
    }

    bool startup()
    {
	string cls = c_format("%s-%u\n",
			      xlog_process_name(), (uint32_t)getpid());
	_rtr = new XrlStdRouter(_e, cls.c_str(), _fhost.c_str(), _fport);
	_rtr->finalize();
	set_status(STARTING);
	_rtr_poll = _e.new_periodic(100,
			callback(this, &XrlJobQueue::xrl_router_ready_poll));
	return true;
    }

    bool shutdown()
    {
	while (_jobs.empty() == false) {
	    _jobs.pop_front();
	}
	set_status(SHUTDOWN);
	return true;
    }

    void
    dispatch_complete(const XrlError& xe, const XrlJobBase* cmd)
    {
	XLOG_ASSERT(_jobs.empty() == false);
	XLOG_ASSERT(_jobs.front().get() == cmd);

	if (xe != XrlError::OKAY()) {
	    shutdown();
	    return;
	}
	_jobs.pop_front();
	if (_jobs.empty() == false) {
	    process_next_job();
	} else {
	    shutdown();
	}
    }

    XrlSender* sender()				{ return _rtr; }

    const string& target() const		{ return _tgt; }

    void enqueue(const Job& cmd)		{ _jobs.push_back(cmd); }

protected:
    bool
    xrl_router_ready_poll()
    {
	if (_rtr->ready()) {
	    process_next_job();
	    return false;
	}
	if (_rtr_poll_cnt++ > 50) {
	    set_status(FAILED, "Could not contact XORP Finder process");
	}
	return true;
    }

    void
    process_next_job()
    {
	if (_jobs.front()->dispatch() == false) {
	    set_status(FAILED, "Could not dispatch xrl");
	}
    }

protected:
    EventLoop&		_e;
    string 		_fhost;	// Finder host
    uint16_t 		_fport;	// Finder port
    string		_tgt; 	// Xrl target to for jobs

    list<Job> 		_jobs;
    XrlStdRouter* 	_rtr;
    XorpTimer		_rtr_poll;	// Timer used to poll XrlRouter::ready
    uint32_t		_rtr_poll_cnt;	// Number of timer XrlRouter polled.
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
	bool		do_run 	    = true;
	string          finder_host = FINDER_DEFAULT_HOST.str();
        uint16_t        finder_port = FINDER_DEFAULT_PORT;
	string		xrl_target;

	int ch;
	while ((ch = getopt(argc, argv, "F:T:")) != -1) {
	    switch (ch) {
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

	if (do_run) {
	    uint32_t ip_version = guess_ip_version(argc, argv);
	    if (xrl_target.empty())
		xrl_target = default_xrl_target(ip_version);
	    // Skip over "show ripX peer statistics" if present
	    // ie run from xorpsh.
	    for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "statistics") == 0) {
		    argc -= i + 1;
		    argv += i + 1;
		    break;
		}
	    }

	    EventLoop e;
	    XrlJobQueue job_queue(e, finder_host, finder_port, xrl_target);

	    if (argc == 4) {
		if (ip_version == 4) {
		    job_queue.enqueue(
			new GetPeerStats4(job_queue,
					  argv[0], argv[1], argv[2], argv[3])
			);
		} else if (ip_version == 6) {
		    job_queue.enqueue(
			new GetPeerStats6(job_queue,
					  argv[0], argv[1], argv[2], argv[3])
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
		    job_queue.enqueue(new GetAllPeerStats4(job_queue));
		} else if (ip_version == 6) {
		    job_queue.enqueue(new GetAllPeerStats6(job_queue));
		}
	    } else {
		fprintf(stderr, "Expected <ifname> <vifname> <address>\n");
		usage();
	    }

	    job_queue.startup();
	    while (job_queue.status() != SHUTDOWN) {
		if (job_queue.status() == FAILED) {
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
