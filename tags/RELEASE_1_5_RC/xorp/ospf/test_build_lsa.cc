// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/test_build_lsa.cc,v 1.21 2007/02/16 22:46:42 pavlin Exp $"

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
    } else if ("LinkLsa" == word) {
	lsa = link_lsa(args);
    } else if ("IntraAreaPrefixLsa" == word) {
	lsa = intra_area_prefix_lsa(args);
    } else {
	xorp_throw(InvalidString, c_format("Unknown LSA name <%s>. [%s]",
					   word.c_str(),
					   args.original_line().c_str()))
    }
	
    return lsa;
}

template <typename A>
bool
getit(Lsa *lsa, OspfTypes::Version version, Options& options)
{
    A *alsa = dynamic_cast<A *>(lsa);
    if (alsa) {
	options = Options(version, alsa->get_options());
	return true;
    }

    return false;
}

Options
BuildLsa::get_options(Lsa *lsa)
{
    switch(_version) {
    case OspfTypes::V2:
	return Options(_version, lsa->get_header().get_options());
	break;
    case OspfTypes::V3: {
	Options options(_version, 0);
	if (getit<RouterLsa>(lsa, _version, options))
	    return options;
	if (getit<NetworkLsa>(lsa, _version, options))
	    return options;
	if (getit<SummaryRouterLsa>(lsa, _version, options))
	    return options;
	if (getit<LinkLsa>(lsa, _version, options))
	    return options;

	xorp_throw(InvalidString,
		   c_format("%s LSA does not have an options field (get)",
			    lsa->name()));
    }
	break;
    }

    return Options(_version, 0);
}

template <typename A>
bool
setit(Lsa *lsa, Options& options)
{
    A *alsa = dynamic_cast<A *>(lsa);
    if (alsa) {
	alsa->set_options(options.get_options());
	return true;
    }

    return false;
}

void
BuildLsa::set_options(Lsa *lsa, Options& options)
{
    switch(_version) {
    case OspfTypes::V2:
	lsa->get_header().set_options(options.get_options());
	break;
    case OspfTypes::V3: {
	if (setit<RouterLsa>(lsa, options))
	    return;
	if (setit<NetworkLsa>(lsa, options))
	    return;
	if (setit<SummaryRouterLsa>(lsa, options))
	    return;
	if (setit<LinkLsa>(lsa, options))
	    return;

	xorp_throw(InvalidString,
		   c_format("%s LSA does not have an options field (set)",
			    lsa->name()));
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
    // an option on an OSPFv3 LSA that does not support options an
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
	    set_link_state_id(set_id(get_next_word(args, word).c_str()));
    } else if ("adv" == word) {
	lsa->get_header().
	    set_advertising_router(set_id(get_next_word(args, word).c_str()));
    } else if ("seqno" == word) {
	lsa->get_header().
	    set_ls_sequence_number(get_next_number(args, word));
    } else if ("cksum" == word) {
	lsa->get_header().set_ls_checksum(get_next_number(args, word));
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
	    rl.set_link_id(set_id(get_next_word(args, nword).c_str()));
	} else if ("ldata" == nword) {	// OSPFv2
	    rl.set_link_data(set_id(get_next_word(args, nword).c_str()));
	} else if ("metric" == nword) {
	    rl.set_metric(get_next_number(args, nword));
	} else if ("iid" == nword) {	// OSPFv3
	    rl.set_interface_id(get_next_number(args, nword));
	} else if ("nid" == nword) {	// OSPFv3
	    rl.set_neighbour_interface_id(get_next_number(args, nword));
	} else if ("nrid" == nword) {	// OSPFv3
	    rl.set_neighbour_router_id(set_id(get_next_word(args, nword).
					      c_str()));
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
	} else if ("bit-W" == word) {	// OSPFv3
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
	    lsa->set_network_mask(get_next_number(args, word));
	} else if ("add-router" == word) {
	    lsa->get_attached_routers().
		push_back(set_id(get_next_word(args, word).c_str()));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

IPv6Prefix
BuildLsa::ipv6prefix(Args& args, bool use_metric)
{
    IPv6Prefix prefix(_version, use_metric);

    prefix.set_network(IPNet<IPv6>(get_next_word(args, "IPv6Prefix").c_str()));

    string nword;
    while(args.get_next(nword)) {
	if ("DN-bit" == nword) {
	    prefix.set_dn_bit(true);
	} else if ("P-bit" == nword) {
	    prefix.set_p_bit(true);
	} else if ("MC-bit" == nword) {
	    prefix.set_mc_bit(true);
	} else if ("LA-bit" == nword) {
	    prefix.set_la_bit(true);
	} else if ("NU-bit" == nword) {
	    prefix.set_nu_bit(true);
	} else if (prefix.use_metric() && "metric" == nword) {
	    prefix.set_metric(get_next_number(args, nword));
	} else {
	    args.push_back();
	    break;
	}
    }

    return prefix;
}

Lsa *
BuildLsa::summary_network_lsa(Args& args)
{
    SummaryNetworkLsa *lsa = new SummaryNetworkLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("netmask" == word) {	// OSPFv2
 	    lsa->set_network_mask(get_next_number(args, word));
	} else if ("metric" == word) {
 	    lsa->set_metric(get_next_number(args, word));
	} else if ("IPv6Prefix" == word) {	// OSPFv3
	    lsa->set_ipv6prefix(ipv6prefix(args));
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
	if ("netmask" == word) {	// OSPFv2
 	    lsa->set_network_mask(get_next_number(args, word));
	} else if ("metric" == word) {
 	    lsa->set_metric(get_next_number(args, word));
	} else if ("drid" == word) {	// OSPFv3
 	    lsa->set_destination_id(set_id(get_next_word(args, word).c_str()));
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
  	    lsa->set_network_mask(get_next_number(args, word));
	} else if ("bit-E" == word) {
	    lsa->set_e_bit(true);
	} else if ("bit-F" == word) {	// OSPFv3 only
	    lsa->set_f_bit(true);
	} else if ("bit-T" == word) {	// OSPFv3 only
	    lsa->set_t_bit(true);
	} else if ("metric" == word) {
	    lsa->set_metric(get_next_number(args, word));
	} else if ("IPv6Prefix" == word) {	// OSPFv3
	    lsa->set_ipv6prefix(ipv6prefix(args));
	} else if ("rlstype" == word) {		// OSPFv3
	    lsa->set_referenced_ls_type(get_next_number(args, word));
	} else if ("forward4" == word) {
	    lsa->set_forwarding_address_ipv4(get_next_word(args,word).c_str());
	} else if ("forward6" == word) {
	    lsa->set_forwarding_address_ipv6(get_next_word(args,word).c_str());
	} else if ("tag" == word) {
	    lsa->set_external_route_tag(get_next_number(args, word));
	} else if ("rlsid" == word) {
	    lsa->set_referenced_link_state_id(set_id(get_next_word(args, word)
						     .c_str()));
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
  	    lsa->set_network_mask(get_next_number(args, word));
	} else if ("bit-E" == word) {
	    lsa->set_e_bit(true);
	} else if ("bit-F" == word) {	// OSPFv3 only
	    lsa->set_f_bit(true);
	} else if ("bit-T" == word) {	// OSPFv3 only
	    lsa->set_t_bit(true);
	} else if ("metric" == word) {
	    lsa->set_metric(get_next_number(args, word));
	} else if ("IPv6Prefix" == word) {	// OSPFv3
	    lsa->set_ipv6prefix(ipv6prefix(args));
	} else if ("rlstype" == word) {		// OSPFv3
	    lsa->set_referenced_ls_type(get_next_number(args, word));
	} else if ("forward4" == word) {
	    lsa->set_forwarding_address_ipv4(get_next_word(args,word).c_str());
	} else if ("forward6" == word) {
	    lsa->set_forwarding_address_ipv6(get_next_word(args,word).c_str());
	} else if ("tag" == word) {
	    lsa->set_external_route_tag(get_next_number(args, word));
	} else if ("rlsid" == word) {
	    lsa->set_referenced_link_state_id(set_id(get_next_word(args, word)
						     .c_str()));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::link_lsa(Args& args)	// OSPFv3 only
{
    LinkLsa *lsa = new LinkLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("rtr-priority" == word) {
  	    lsa->set_rtr_priority(get_next_number(args, word));
	} else if ("link-local-address" == word) {
  	    lsa->set_link_local_address(get_next_word(args, word).c_str());
	} else if ("IPv6Prefix" == word) {
	    lsa->get_prefixes().push_back(ipv6prefix(args));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}

Lsa *
BuildLsa::intra_area_prefix_lsa(Args& args)	// OSPFv3 only
{
    IntraAreaPrefixLsa *lsa = new IntraAreaPrefixLsa(_version);

    string word;
    while(args.get_next(word)) {
	if (common_header(lsa, word, args))
	    continue;
	if ("rlstype" == word) {
	    string nword = get_next_word(args, word);
	    uint16_t referenced_ls_type;
	    if ("RouterLsa" == nword) {
		referenced_ls_type = RouterLsa(_version).get_ls_type();
	    } else if ("NetworkLsa" == nword) {
		referenced_ls_type = NetworkLsa(_version).get_ls_type();
	    } else {
		referenced_ls_type = get_number(nword);
	    }
	    lsa->set_referenced_ls_type(referenced_ls_type);
	} else if ("rlsid" == word) {
	    lsa->set_referenced_link_state_id(set_id(get_next_word(args, word)
						     .c_str()));
	} else if ("radv" == word) {
	    lsa->set_referenced_advertising_router(set_id(get_next_word(args,
									word)
							  .c_str()));
	} else if ("IPv6Prefix" == word) {
	    lsa->get_prefixes().push_back(ipv6prefix(args, true));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown option <%s>. [%s]",
					       word.c_str(),
					       args.original_line().c_str()));
	}
    }

    return lsa;
}
