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

// $XORP: xorp/fea/fti_click.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef __FEA_FTI_CLICK_HH__
#define __FEA_FTI_CLICK_HH__

#include "click_glue.hh"

class FtiClick : public Fti {
public:
    FtiClick() throw (FtiError);

    bool start_configuration();
    bool end_configuration();

    bool delete_all_entries4();
    bool delete_entry4(const Fte4& fte);

    bool add_entry4(const Fte4& fte);

    bool start_reading4();
    bool read_entry4(Fte4& fte);

    bool lookup_route4(IPv4 addr, Fte4& fte);
    bool lookup_entry4(const IPv4Net& dst, Fte4& fte);

    bool delete_all_entries6();
    bool delete_entry6(const Fte6& fte);

    bool add_entry6(const Fte6& fte);

    bool start_reading6();
    bool read_entry6(Fte6& fte);

    bool lookup_route6(const IPv6& addr, Fte6& fte);
    bool lookup_entry6(const IPv6Net& dst, Fte6& fte);

    void add_portmap(ClickPortmap cpm) {
	_cpm = cpm;
    }
private:
    const char *_click_proc;

    bool open(FILE **fp, string file, const char *mode);
    bool close(FILE **fp, string file);
    bool write(const string& file, string value);
    bool read(const string& file, string& value);

    ClickPortmap _cpm;

    int ifname_to_port(const string& ifname);
    string port_to_ifname(int port);

    bool make_routing_list(list<Fte4>& rt);

    string _tid;
    bool _reading;
    list<Fte4> _routing_table;
    list<Fte4>::iterator _current_entry;

};

#endif // __FEA_FTI_CLICK_HH__
