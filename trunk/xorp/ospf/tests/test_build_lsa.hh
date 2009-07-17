// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/test_build_lsa.hh,v 1.11 2008/10/02 21:57:49 bms Exp $

#ifndef __OSPF_TEST_BUILD_LSA_HH__
#define __OSPF_TEST_BUILD_LSA_HH__

#include "libxorp/tokenize.hh"

class BuildLsa {
 public:
    BuildLsa(OspfTypes::Version version) : _version(version)
    {}

    /**
     * From a textual representation build an LSA that can be used for
     * testing.
     */
    Lsa *generate(Args& args);

 private:
    OspfTypes::Version _version;

    Options get_options(Lsa *lsa);

    void set_options(Lsa *lsa, Options& options);

    bool common_header(Lsa *lsa, const string& word, Args& args);

    bool router_link(RouterLsa *lsa, const string& word, Args& args);

    Lsa *router_lsa(Args& args);

    Lsa *network_lsa(Args& args);

    IPv6Prefix ipv6prefix(Args& args, bool use_metric = false);

    Lsa *summary_network_lsa(Args& args);

    Lsa *summary_router_lsa(Args& args);

    Lsa *as_external_lsa(Args& args);

    Lsa *type_7_lsa(Args& args);

    Lsa *link_lsa(Args& args);

    Lsa *intra_area_prefix_lsa(Args& args);
};

#endif // __OSPF_TEST_BUILD_LSA_HH__
