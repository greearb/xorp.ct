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

#ident "$XORP: xorp/fea/click_elements/forward1.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
// ALWAYS INCLUDE click/config.h
#include <click/config.h>
// ALWAYS INCLUDE click/package.hh
#include <click/package.hh>
#include <click/error.hh>
#include <click/confparse.hh>
#include "forward1.hh"
#include "rtable1.hh"

Forward1::Forward1()
{
    // CONSTRUCTOR MUST MOD_INC_USE_COUNT
    MOD_INC_USE_COUNT;
    add_input();
}

Forward1::~Forward1()
{
    // DESTRUCTOR MUST MOD_DEC_USE_COUNT
    MOD_DEC_USE_COUNT;
}

int 
Forward1::configure(const Vector<String> &conf, ErrorHandler *errh)
{
    int max_port = -1;

    for(int i = 0; i < conf.size(); i++) {
        click_chatter((String)conf[i]);

        unsigned int dst, mask;
        unsigned int gw = 0;
        int port;

        Vector<String> words;
        cp_spacevec(conf[i], words);

        /*
        ** The options are:
        ** address/mask   [gw optional] port
        ** address/prefix [gw optional] port
        */
        if(!((2 == words.size()) || (3 == words.size()))) {
            String tmp = conf[i];
            errh->warning("bad argument line %d: %s", i + 1, tmp.cc());
            return -1;
        }

        /*
        ** Pick out the destination
        */
        if(!cp_ip_prefix(words[0], (unsigned char *)&dst, 
                         (unsigned char *)&mask, true, this)) {
            errh->warning("bad argument line %d: %s should be addr/mask",
                          i + 1, words[0].cc());
            return -1;
        }

        /*
        ** If there are three words pick out the optional gateway.
        */
        if((3 == words.size())) {
            if(!cp_ip_address(words[1], (unsigned char *)&gw, this)) {
                errh->warning("bad argument line %d: %s should be addr", 
                              i + 1, words[1].cc());
                return -1;
            }
        }
        
        /*
        ** port
        */
        if(!cp_integer(words.back(), &port)) {
                errh->warning(
                              "bad argument line %d: %s should be port number",
                              i + 1, words.back().cc());
                return -1;
        }

        _rt.add(dst, mask, gw, port);
        
        max_port = port > max_port ? port : max_port;
    }

    set_noutputs(max_port + 1);

    return 0;
}

int
Forward1::initialize(ErrorHandler *)
{
//     errh->message("Successfully linked with package!");

    //    Rtable1 rt;

    _rt.test();

    return 0;
}

void
Forward1::push(int, Packet *p)
{
    IPV4address a = p->dst_ip_anno().addr();
    IPV4address gw;
    int oport;
    
    if(!_rt.lookup(a, gw, oport)) {
        click_chatter("Forward1::push: No route for %s",
                      (const char *)a.string());
        p->kill();
        return;
    }

//     click_chatter("Forward1::push: %s", (const char *)a.string());

    output(oport).push(p);
    return;
}

EXPORT_ELEMENT(Forward1)
ELEMENT_REQUIRES(HoopyFrood)
