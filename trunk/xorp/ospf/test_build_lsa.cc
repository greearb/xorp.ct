// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/test_build_lsa.cc,v 1.2 2006/03/28 02:41:17 atanu Exp $"

#include "config.h"
#include "ospf_module.h"

#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "packet.hh"

#include "test_build_lsa.hh"

/**
 * Get a number in base 8,10 or 16.
 */
inline
uint32_t
get_number(const string& word) throw(InvalidString)
{
    char *endptr;
    
    uint32_t number = strtoul(word.c_str(), &endptr, 0);
    if (0 != *endptr)
	xorp_throw(InvalidString,
		   c_format("<%s> is not a number", word.c_str()));

    return number;
}

/**
 * Get the next word throw an exception if its not present.
 */
inline
string
get_next_word(Args& args, const string& varname) throw(InvalidString)
{
    string var;
    if (!args.get_next(var))
	xorp_throw(InvalidString,
		   c_format("No argument to %s. [%s]",
			    varname.c_str(),
			    args.original_line().c_str()));

    return var;
}

inline
uint32_t
get_next_number(Args& args, const string& varname) throw(InvalidString)
{
    return get_number(get_next_word(args, varname));
}

Lsa *
BuildLsa::generate(Args& args)
{
    string word;
    if (!args.get_next(word))
	return 0;
    
    Lsa *lsa = 0;
    if ("RouterLsa" == word) {
	lsa = router_lsa(args);
    } else if ("NetworkLsa" == word) {
	lsa = network_lsa(args);
    } else if ("SummaryNetworkLsa" == word) {
	lsa = summary_network_lsa(args);
    } else if ("SummaryRouterLsa" == word) {
	lsa = summary_router_lsa(args);
    } else if ("ASExternalLsa" == word) {
	lsa = as_external_lsa(args);
    } else if ("Type7Lsa" == word) {
	lsa = type_7_lsa(args);
    } else {
	xorp_throw(InvalidString, c_format("Unknown LSA name <%s>. [%s]",
					   word.c_str(),
					   args.original_line().c_str()))
    }
	
    return lsa;
}

Options
BuildLsa::get_options(Lsa *lsa)
{
    switch(_version) {
    case OspfTypes::V2:
	return Options(_version, lsa->get_header().get_options());
	break;
    case OspfTypes::V3:
	XLOG_UNFINISHED();
	break;
    }

    return Options(_version, 0);
}

void
BuildLsa::set_options(Lsa *lsa, Options& options)
{
    switch(_version) {
    case OspfTypes::V2:
	lsa->get_header().set_options(options.get_options());
	break;
    case OspfTypes::V3:
	XLOG_UNFINISHED();
	break;
    }
}

bool
BuildLsa::common_header(Lsa *lsa, const string& word, Args& args)
{
    if ("age" == word) {
	string age;
	if (!args.get_next(age))
	    xorp_throw(InvalidString, c_format("No argument to age. [%s]",
					       args.original_line().c_str()));
	
	lsa->get_header().set_ls_age(get_number(age)); 
    } else if ("V6-bit" == word) {
	Options options = get_options(lsa);
	options.set_v6_bit(true);
	set_options(lsa, options);
    } else if ("E-bit" == word) {
	Options options = get_options(lsa);
	options.set_e_bit(true);
	set_options(lsa, options);
    } else if ("MC-bit" == word) {
	Options options = get_options(lsa);
	options.set_mc_bit(true);
	set_options(lsa, options);
    } else if ("N/P-bit" == word) {
	Options options = get_options(lsa);
	options.set_n_bit(true);
	set_options(lsa, options);
    } else if ("R-bit" == word) {
	Options options = get_options(lsa);
	options.set_r_bit(true);
	set_options(lsa, options);
    } else if ("EA-bit" == word) {
	Options options = get_options(lsa);
	options.set_ea_bit(true);
	set_options(lsa, options);
    } else if ("DC-bit" == word) {
	Options options = get_options(lsa);
	options.set_dc_bit(true);
	set_options(lsa, options);
    } else if ("lsid" == word) {
	lsa->get_header().
	    set_link_state_id(set_id(get_next_word(args, "lsid").c_str()));
    } else if ("adv" == word) {
	lsa->get_header().
	    set_advertising_router(set_id(get_next_word(args, "adv").c_str()));
    } else if ("seqno" == word) {
	lsa->get_header().
	    set_ls_sequence_number(get_next_number(args, "seqno"));
    } else if ("cksum" == word) {
	lsa->get_header().set_ls_checksum(get_next_number(args, "cksum"));
    } else {
	return false;
    }

    return true;
}

Lsa *
BuildLsa::router_lsa(Args& args)
{
    RouterLsa *lsa = new RouterLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("bit-NT" == word) {
	    lsa->set_nt_bit(true);
	} else if ("bit-V" == word) {
	    lsa->set_v_bit(true);
	} else if ("bit-E" == word) {
	    lsa->set_e_bit(true);
	} else if ("bit-B" == word) {
	    lsa->set_b_bit(true);
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::network_lsa(Args& args)
{
    NetworkLsa *lsa = new NetworkLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("netmask" == word) {
	    lsa->set_network_mask(get_next_number(args, "netmask"));
	} else if ("add-router" == word) {
	    lsa->get_attached_routers().
		push_back(set_id(get_next_word(args, "router").c_str()));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::summary_network_lsa(Args& args)
{
    SummaryNetworkLsa *lsa = new SummaryNetworkLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("netmask" == word) {
 	    lsa->set_network_mask(get_next_number(args, "netmask"));
	} else if ("metric" == word) {
 	    lsa->set_metric(get_next_number(args, "netmask"));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::summary_router_lsa(Args& args)
{
    SummaryRouterLsa *lsa = new SummaryRouterLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("netmask" == word) {
 	    lsa->set_network_mask(get_next_number(args, "netmask"));
	} else if ("metric" == word) {
 	    lsa->set_metric(get_next_number(args, "netmask"));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::as_external_lsa(Args& args)
{
    ASExternalLsa *lsa = new ASExternalLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("netmask" == word) {
  	    lsa->set_network_mask(get_next_number(args, "netmask"));
	} else if ("bit-E" == word) {
	    lsa->set_e_bit(true);
	} else if ("metric" == word) {
	    lsa->set_metric(get_next_number(args, "metric"));
	} else if ("forward4" == word) {
	    lsa->set_forwarding_address_ipv4(get_next_word(args, "forward4").
					     c_str());
	} else if ("tag" == word) {
	    lsa->set_external_route_tag(get_next_number(args, "tag"));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::type_7_lsa(Args& args)
{
    Type7Lsa *lsa = new Type7Lsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("netmask" == word) {
  	    lsa->set_network_mask(get_next_number(args, "netmask"));
	} else if ("bit-E" == word) {
	    lsa->set_e_bit(true);
	} else if ("metric" == word) {
	    lsa->set_metric(get_next_number(args, "metric"));
	} else if ("forward4" == word) {
	    lsa->set_forwarding_address_ipv4(get_next_word(args, "forward4").
					     c_str());
	} else if ("tag" == word) {
	    lsa->set_external_route_tag(get_next_number(args, "tag"));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}
