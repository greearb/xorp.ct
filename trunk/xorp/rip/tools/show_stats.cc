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

#ident "$XORP: xorp/rip/tools/show_stats.cc,v 1.1 2004/03/11 00:04:20 hodson Exp $"

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
    fprintf(stderr, "%s [options] <interface> <vif> <addr>\n",
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

static void
pretty_print_counters(const XrlAtomList* descriptions,
		      const XrlAtomList* values)
{
    static const uint32_t COL1 = 32;
    static const uint32_t COL2 = 16;

    // Save flags
    ios::fmtflags fl = cout.flags();

    cout.flags(ios::left);
    cout << setw(COL1) << "Counter" << setw(0) << " ";
    cout.flags(ios::right);
    cout << setw(COL2) << "Value" << endl;

    cout.flags(ios::left);
    cout << setw(COL1) << string(COL1, '-') << setw(0) << " ";
    cout.flags(ios::right);
    cout << setw(COL2) << string(COL2, '-') << endl;

    for (size_t i = 0; i != descriptions->size(); ++i) {
	const XrlAtom& d = descriptions->get(i);
	const XrlAtom& v = values->get(i);

	cout.flags(ios::left);
	// NB we use string.c_str() here as GCC's string ostream renderer
	// seems to ignore the field width.
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
    virtual ~XrlJobQueueBase() {}
    virtual XrlSender*	  sender()	 = 0;
    virtual const string& target() const = 0;

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
 * Invoke Xrl to get address stats on RIP address and pretty print result.
 */
class GetAddressStats4 : public XrlJobBase {
public:
    GetAddressStats4(XrlJobQueueBase&	jq,
		     const string&	ifname,
		     const string&	vifname,
		     IPv4		addr)
	: XrlJobBase(jq), _ifn(ifname), _vifn(vifname), _a(addr)
    {}

    bool
    dispatch()
    {
	XrlRipV0p1Client cl(queue().sender());
	return cl.send_get_counters(queue().target().c_str(), _ifn, _vifn, _a,
				    callback(this,
					     &GetAddressStats4::cmd_callback)
				    );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	descriptions,
		 const XrlAtomList*	values)
    {
	if (xe == XrlError::OKAY()) {
	    cout << "RIP statistics for " << _ifn << " " << _vifn << " "
		 << _a.str() << endl << endl;
	    pretty_print_counters(descriptions, values);
	}
	queue().dispatch_complete(xe, this);
    }

protected:
    string _ifn;
    string _vifn;
    IPv4   _a;
};

/**
 * Invoke Xrl to get address stats on RIPng address and pretty print result.
 */
class GetAddressStats6 : public XrlJobBase {
public:
    GetAddressStats6(XrlJobQueueBase&	d,
		     const string&		ifname,
		     const string&		vifname,
		     const IPv6&		addr)
	: XrlJobBase(d), _ifn(ifname), _vifn(vifname), _a(addr)
    {}

    bool
    dispatch()
    {
	XrlRipngV0p1Client cl(queue().sender());
	return cl.send_get_counters(queue().target().c_str(), _ifn, _vifn, _a,
				    callback(this,
					     &GetAddressStats6::cmd_callback));
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	descriptions,
		 const XrlAtomList*	values)
    {
	if (xe == XrlError::OKAY()) {
	    cout << "RIP statistics for " << _ifn << " " << _vifn << " "
		 << _a.str() << endl << endl;
	    pretty_print_counters(descriptions, values);
	}
	queue().dispatch_complete(xe, this);
    }

protected:
    string _ifn;
    string _vifn;
    IPv6   _a;
};


/**
 * Xrl Job Queue
 */
class XrlJobQueue
    : public XrlJobQueueBase, public ServiceBase
{
public:
    typedef ref_ptr<XrlJobBase> Job;

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

	    // Skip over "show ripX statistics" if present
	    // ie run from xorpsh.
	    for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "statistics") == 0) {
		    argc -= i + 1;
		    argv += i + 1;
		    break;
		}
	    }
	    if (argc == 3) {
		EventLoop e;
		XrlJobQueue job_queue(e, finder_host, finder_port, xrl_target);

		if (ip_version == 4) {
		    job_queue.enqueue(
			new GetAddressStats4(job_queue,
					     argv[0], argv[1], argv[2])
			);
		} else if (ip_version == 6) {
		    job_queue.enqueue(
			new GetAddressStats6(job_queue,
					     argv[0], argv[1], argv[2])
			);
		}

		job_queue.startup();
		while (job_queue.status() != SHUTDOWN) {
		    if (job_queue.status() == FAILED) {
			cerr << "Failed: " << job_queue.status_note() << endl;
			break;
		    }
		    e.run();
		}
	    } else {
		fprintf(stderr, "Expected <ifname> <vifname> <address>\n");
		usage();
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
