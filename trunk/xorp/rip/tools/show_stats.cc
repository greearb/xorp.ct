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

#ident "$XORP: xorp/rip/tools/show_stats.cc,v 1.3 2004/05/31 04:06:41 hodson Exp $"

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

// ----------------------------------------------------------------------------
// Utility methods

static void
usage()
{
    fprintf(stderr,
	    "Usage: %s [options] (rip|ripng) [<interface> <vif> <addr>]\n",
	    xlog_process_name());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t -F <finder_host>:<finder_port> "
	    "Specify Finder host and port to use.\n");
    fprintf(stderr, "\t -T <targetname>                "
	    "Specify XrlTarget to query.\n\n");
    exit(-1);
}

template <typename A>
static void
print_header(const string&	ifn,
	     const string&	vifn,
	     const A&		addr)
{
    cout << endl;
    cout << "* RIP statistics for " << ifn << " " << vifn << " "
	 << addr.str() << endl << endl;
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
    cout << "  ";
    cout << setw(COL1) << "Counter" << setw(0) << " ";
    cout.flags(ios::right);
    cout << setw(COL2) << "Value" << endl;

    cout.flags(ios::left);
    cout << "  ";
    cout << setw(COL1) << string(COL1, '-') << setw(0) << " ";
    cout.flags(ios::right);
    cout << setw(COL2) << string(COL2, '-') << endl;

    for (size_t i = 0; i != descriptions->size(); ++i) {
	const XrlAtom& d = descriptions->get(i);
	const XrlAtom& v = values->get(i);

	cout.flags(ios::left);
	cout << setw(0) << "  ";
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

/**
 * Invoke Xrl to get address stats on RIP address and pretty print result.
 */
class GetAddressStats4 : public XrlJobBase {
public:
    GetAddressStats4(XrlJobQueue&	jq,
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
	    print_header(_ifn, _vifn, _a);
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
 * Invoke Xrl to get all addresses, which we then use to get the counters
 * for to pretty print result.
 */
class GetAllAddressStats4 : public XrlJobBase {
public:
    GetAllAddressStats4(XrlJobQueue& jq)
	: XrlJobBase(jq)
    {}

    bool
    dispatch()
    {
	XrlRipV0p1Client cl(queue().sender());
	return cl.send_get_all_addresses(queue().target().c_str(),
			callback(this, &GetAllAddressStats4::cmd_callback)
					 );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	ifnames,
		 const XrlAtomList*	vifnames,
		 const XrlAtomList*	addrs)
    {
	if (xe == XrlError::OKAY()) {
	    for (size_t i = 0; i < ifnames->size(); i++) {
		const string& 	ifn 	  = ifnames->get(i).text();
		const string& 	vifn 	  = vifnames->get(i).text();
		const IPv4& 	addr 	  = addrs->get(i).ipv4();
		queue().enqueue(
				new GetAddressStats4(queue(), ifn, vifn, addr)
				);
	    }
	}
	queue().dispatch_complete(xe, this);
    }
};

/**
 * Invoke Xrl to get address stats on RIPng address and pretty print result.
 */
class GetAddressStats6 : public XrlJobBase {
public:
    GetAddressStats6(XrlJobQueue&	d,
		     const string&	ifname,
		     const string&	vifname,
		     const IPv6&	addr)
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
	    print_header(_ifn, _vifn, _a);
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
 * Invoke Xrl to get all addresses, which we then use to get the counters
 * for to pretty print result.
 */
class GetAllAddressStats6 : public XrlJobBase {
public:
    GetAllAddressStats6(XrlJobQueue& jq)
	: XrlJobBase(jq)
    {}

    bool
    dispatch()
    {
	XrlRipV0p1Client cl(queue().sender());
	return cl.send_get_all_addresses(queue().target().c_str(),
			callback(this, &GetAllAddressStats6::cmd_callback)
					 );
    }

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	ifnames,
		 const XrlAtomList*	vifnames,
		 const XrlAtomList*	addrs)
    {
	if (xe == XrlError::OKAY()) {
	    for (size_t i = 0; i < ifnames->size(); i++) {
		const string& 	ifn 	  = ifnames->get(i).text();
		const string& 	vifn 	  = vifnames->get(i).text();
		const IPv6& 	addr 	  = addrs->get(i).ipv6();
		queue().enqueue(
				new GetAddressStats6(queue(), ifn, vifn, addr)
				);
	    }
	}
	queue().dispatch_complete(xe, this);
    }
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

	if (argc == 0) {
	    usage();
	}

	uint32_t ip_version = rip_name_to_ip_version(argv[0]);
	if (ip_version == 0) {
	    cerr << "Bad ip version (" << ip_version << ")" << endl;
	    usage();
	}
	argc -= 1;
	argv += 1;

	if (xrl_target.empty()) {
	    const char* xt = default_xrl_target(ip_version);
	    if (xt == 0) {
		cerr << "Bad xrl target (" << ip_version << ")" << endl;
		usage();
	    }
	    xrl_target = xt;
	}

	if (do_run) {
	    EventLoop e;
	    XrlJobQueue job_queue(e, finder_host, finder_port, xrl_target);
	    if (argc == 3) {
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
	    } else if (argc == 0) {
		if (ip_version == 4) {
		    job_queue.enqueue(new GetAllAddressStats4(job_queue));
		} else if (ip_version == 6) {
		    job_queue.enqueue(new GetAllAddressStats6(job_queue));
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
