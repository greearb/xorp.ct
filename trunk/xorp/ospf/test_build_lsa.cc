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

#ident "$XORP: xorp/ospf/test_build_lsa.cc,v 1.6 2006/12/15 00:50:19 atanu Exp $"

#include "ospf_module.h"

#include "libxorp/xorp.h"
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
#include "test_args.hh"
#include "test_build_lsa.hh"

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
	RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsa);
	if (rlsa)
	    return Options(_version, rlsa->get_options());
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
	RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsa);
	if (rlsa) {
	    rlsa->set_options(options.get_options());
	    return;
	}
	break;
    }
}

bool
BuildLsa::common_header(Lsa *lsa, const string& word, Args& args)
{
    // Note: OSPFv3 LSAs do not have an options field in the common
    // header. it reduces the amount of code to process the options
    // for the OSPFv3 case here. The get_options() and set_options()
    // will perform the relevant magic. If an attempt is made to set
    // an option on an OSPFv3 LSA that does nto support options an
    // exception will be thrown. Some options are version specific in
    // the case of a version mismatch the option code will abort().

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
    } else if ("N/P-bit" == word || "N-bit" == word) {
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

bool
BuildLsa::router_link(RouterLsa *rlsa, const string& word, Args& args)
{
    RouterLink rl(_version);
    if ("p2p" == word) {
	rl.set_type(RouterLink::p2p);
    } else if ("transit" == word) {
	rl.set_type(RouterLink::transit);
    } else if ("stub" == word) {
	rl.set_type(RouterLink::stub);
    } else if ("vlink" == word) {
	rl.set_type(RouterLink::vlink);
    } else {
	return false;
    }

    string nword;
    while(args.get_next(nword)) {
	if ("lsid" == nword) {	// OSPFv2
	    rl.set_link_id(set_id(get_next_word(args, "lsid").c_str()));
	    continue;
	} else if ("ldata" == nword) {	// OSPFv2
	    rl.set_link_data(set_id(get_next_word(args, "ldata").c_str()));
	    continue;
	} else if ("metric" == nword) {
	    rl.set_metric(get_next_number(args, "ldata"));
	    continue;
	} else if ("iid" == nword) {	// OSPFv3
	    rl.set_interface_id(get_next_number(args, "iid"));
	    continue;
	} else if ("nid" == nword) {	// OSPFv3
	    rl.set_neighbour_interface_id(get_next_number(args, "nid"));
	    continue;
	} else if ("nrid" == nword) {	// OSPFv3
	    rl.set_neighbour_router_id(get_next_number(args, "nrid"));
	    continue;
	} else {
	    args.push_back();
	    break;
	}
    }

    rlsa->get_router_links().push_back(rl);

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
	if (router_link(lsa, word, args))
	    continue;
	if ("bit-NT" == word) {
	    lsa->set_nt_bit(true);
	} else if ("bit-W" == word) {
	    lsa->set_w_bit(true);
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
