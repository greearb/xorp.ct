// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/fea/fti_dummy.hh,v 1.12 2002/12/09 18:28:56 hodson Exp $

#ifndef	__FEA_FTI_DUMMY_HH__
#define __FEA_FTI_DUMMY_HH__

#include "fti.hh"

#include "libxorp/trie.hh"

class DummyFti : public Fti {
public:
    bool start_configuration();
    bool end_configuration();

    bool delete_all_entries4();
    bool delete_entry4(const Fte4& fte);

    bool add_entry4(const Fte4& fte);

    bool start_reading4();
    bool read_entry4(Fte4& fte);

    bool lookup_route4(IPv4 addr, Fte4& fte);
    bool lookup_entry4(const IPv4Net& net, Fte4& fte);

    bool delete_all_entries6();
    bool delete_entry6(const Fte6& fte);

    bool add_entry6(const Fte6& fte);

    bool start_reading6();
    bool read_entry6(Fte6& fte);

    bool lookup_route6(const IPv6& addr, Fte6& fte);
    bool lookup_entry6(const IPv6Net& net, Fte6& fte);

private:
    typedef Trie<IPv4, Fte4> Trie4;
    Trie4	_t4;

    typedef Trie<IPv6, Fte6> Trie6;
    Trie6	_t6;
};

#endif	// __FEA_FTI_DUMMY_HH__
