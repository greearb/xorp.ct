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

#ident "$XORP: xorp/rip/tools/show_stats.cc,v 1.5 2004/06/06 02:06:48 hodson Exp $"

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
// Forward declarations

template <typename A>
static void
enqueue_address_query(XrlJobQueue&	jq,
		      const string&	ifname,
		      const string&	vifname,
		      const A&		address,
		      bool		brief);

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
    fprintf(stderr, "\t -b                             "
	    "Brief output.");
    exit(-1);
}

template <typename A>
static void
print_header(const string&	ifn,
	     const string&	vifn,
	     const A&		addr)
{
    cout << endl;
    cout << "* RIP on " << ifn << " " << vifn << " " << addr.str() << endl;
}

static void
pretty_print_status(const string& status)
{
    cout << "  Status: " << status << endl << endl;
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
// GetListAtom - extract argument of templatized type from XrlAtomList
// [ This should probably be part of libxipc/xrl_atom_list.hh ]

template <typename A>
struct GetListAtom {
    inline GetListAtom(const XrlAtomList& xal) : _xal(xal) {}
    inline GetListAtom(const XrlAtomList* pxal) : _xal(*pxal) {}
    inline const A& at(uint32_t index);

private:
    const XrlAtomList& _xal;
};

template <>
inline const IPv4&
GetListAtom<IPv4>::at(uint32_t index)
{
    return _xal.get(index).ipv4();
}

template <>
const IPv6&
GetListAtom<IPv6>::at(uint32_t index)
{
    return _xal.get(index).ipv6();
}

template <>
const string&
GetListAtom<string>::at(uint32_t index)
{
    return _xal.get(index).text();
}


// ----------------------------------------------------------------------------
// Job classes and related

template <typename A>
class XrlAddrJobBase : public XrlJobBase {
public:
    XrlAddrJobBase(XrlJobQueue& 	jq,
		   const string& 	ifname,
		   const string& 	vifname,
		   const A& 		addr)
	: XrlJobBase(jq), _ifname(ifname), _vifname(vifname), _addr(addr)
    {}
    inline const string& ifname() const 		{ return _ifname; }
    inline const string& vifname()const			{ return _vifname; }
    inline const A& addr() const			{ return _addr;	}

protected:
    string	_ifname;
    string	_vifname;
    A		_addr;
};

/**
 * Class to print header.
 */
template <typename A>
class PrintAddress : public XrlAddrJobBase<A>
{
public:
    PrintAddress(XrlJobQueue&	jq,
		 const string&	ifname,
		 const string&	vifname,
		 const A&	addr)
	: XrlAddrJobBase<A>(jq, ifname, vifname, addr)
    {}
    bool dispatch()
    {
	print_header(this->ifname(), this->vifname(), this->addr());
	this->queue().dispatch_complete(XrlError::OKAY(), this);
	return true;
    }
};

/**
 * Invoke Xrl to get address stats on RIP address and pretty print result.
 */
template <typename A>
class GetAddressStats : public XrlAddrJobBase<A> {
public:
    GetAddressStats(XrlJobQueue&	jq,
		    const string&	ifname,
		    const string&	vifname,
		    const A&		addr)
	: XrlAddrJobBase<A>(jq, ifname, vifname, addr)
    {}

    bool dispatch();

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const XrlAtomList*	descriptions,
		 const XrlAtomList*	values)
    {
	if (xe == XrlError::OKAY()) {
	    pretty_print_counters(descriptions, values);
	}
	this->queue().dispatch_complete(xe, this);
    }
};

template <>
bool
GetAddressStats<IPv4>::dispatch()
{
    XrlRipV0p1Client cl(queue().sender());
    return cl.send_get_counters(queue().target().c_str(),
				this->ifname(), this->vifname(), this->addr(),
				callback(this,
					 &GetAddressStats<IPv4>::cmd_callback)
				);
}

template <>
bool
GetAddressStats<IPv6>::dispatch()
{
    XrlRipngV0p1Client cl(queue().sender());
    return cl.send_get_counters(queue().target().c_str(),
				this->ifname(), this->vifname(), this->addr(),
				callback(this,
					 &GetAddressStats<IPv6>::cmd_callback)
				);
}

/**
 * Invoke Xrl to get address state on RIP address and pretty print result.
 */
template <typename A>
class GetAddressStatus : public XrlAddrJobBase<A> {
public:
    GetAddressStatus(XrlJobQueue&	jq,
		     const string&	ifname,
		     const string&	vifname,
		     const A&		addr)
	: XrlAddrJobBase<A>(jq, ifname, vifname, addr)
    {}

    bool dispatch();

protected:
    void
    cmd_callback(const XrlError&	xe,
		 const string*		status)
    {
	if (xe == XrlError::OKAY()) {
	    pretty_print_status(*status);
	}
	this->queue().dispatch_complete(xe, this);
    }
};

template <>
bool
GetAddressStatus<IPv4>::dispatch()
{
    XrlRipV0p1Client cl(queue().sender());
    return cl.send_rip_address_status(queue().target().c_str(),
		this->ifname(), this->vifname(), this->addr(),
		callback(this, &GetAddressStatus<IPv4>::cmd_callback)
		);
}

template <>
bool
GetAddressStatus<IPv6>::dispatch()
{
    XrlRipngV0p1Client cl(queue().sender());
    return cl.send_rip_address_status(queue().target().c_str(),
		this->ifname(), this->vifname(), this->addr(),
		callback(this, &GetAddressStatus<IPv6>::cmd_callback)
		);
}

/**
 * Invoke Xrl to get all addresses, which we then use to get the counters
 * for to pretty print result.
 */
template <typename A>
class GetAllAddressStats : public XrlJobBase {
public:
    GetAllAddressStats(XrlJobQueue& jq, bool brief)
	: XrlJobBase(jq), _brief(brief)
    {}

    bool dispatch();

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
		const A& 	addr 	  = GetListAtom<A>(addrs).at(i);
		enqueue_address_query(queue(), ifn, vifn, addr, _brief);
	    }
	}
	queue().dispatch_complete(xe, this);
    }
private:
    bool _brief;
};

template <>
bool
GetAllAddressStats<IPv4>::dispatch()
{
    XrlRipV0p1Client cl(queue().sender());
    return cl.send_get_all_addresses(queue().target().c_str(),
		callback(this, &GetAllAddressStats<IPv4>::cmd_callback)
				     );
}

template <>
bool
GetAllAddressStats<IPv6>::dispatch()
{
    XrlRipV0p1Client cl(queue().sender());
    return cl.send_get_all_addresses(queue().target().c_str(),
		callback(this, &GetAllAddressStats<IPv6>::cmd_callback)
				     );
}

template <typename A>
void
enqueue_address_query(XrlJobQueue& 	jq,
		      const string& 	ifname,
		      const string& 	vifname,
		      const A& 		addr,
		      bool 		brief)
{
    jq.enqueue(new PrintAddress<A>(jq, ifname, vifname, addr));
    jq.enqueue(new GetAddressStatus<A>(jq, ifname, vifname, addr));
    if (brief == false)
	jq.enqueue(new GetAddressStats<A>(jq, ifname, vifname, addr));
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
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	bool		do_run 	    = true;
	bool		brief 	    = false;
	string  	finder_host = FINDER_DEFAULT_HOST.str();
        uint16_t        finder_port = FINDER_DEFAULT_PORT;
	string		xrl_target;

	int ch;
	while ((ch = getopt(argc, argv, "bF:T:")) != -1) {
	    switch (ch) {
	    case 'b':
		brief = true;
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
		    enqueue_address_query(job_queue,
					  argv[0], argv[1], IPv4(argv[2]),
					  brief);
		} else if (ip_version == 6) {
		    enqueue_address_query(job_queue,
					  argv[0], argv[1], IPv6(argv[2]),
					  brief);

		}
	    } else if (argc == 0) {
		if (ip_version == 4) {
		    job_queue.enqueue(new GetAllAddressStats<IPv4>(job_queue,
								   brief));
		} else if (ip_version == 6) {
		    job_queue.enqueue(new GetAllAddressStats<IPv6>(job_queue,
								   brief));
		}
	    } else {
		cerr << "Expected (rip|ripng) (all|<ifname> <vifname> <address>)\n";
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
