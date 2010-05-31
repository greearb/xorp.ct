// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "packet.hh"
#include "peer.hh"
#include "bgp.hh"



/*
 * XXX NOTE THAT THIS TEST IS CURRENTLY UNUSED.
 *
 * we try to decode a handcrafted update packet.
 */
uint8_t buffer[] = {
	// marker
	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,
	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

	0x00,   0x13,	// length (to be filled later)
	0x02,	// type 2 = update
#if 0
	0x00,	0x00,	0x00,	0x32,	0x40,	0x01,	0x01,	0x00,
	0x40,	0x02,	0x24,	0x02,	0x11,	0xfd,	0xe8,	0xfd,
	0xec,	0x41,	0x36,	0x00,	0x19,	0x2c,	0x9f,	0x01,
	0x25,	0x01,	0x79,	0x01,	0x79,	0x01,	0x79,	0x01,
	0x79,	0x01,	0x79,	0x01,	0x79,	0x01,	0x79,	0x01,
	0x79,	0x01,	0x79,	0x01,	0x79,	0x01,	0x79,	0x40,
	0x03,	0x04,	0xc0,	0x96,	0xbb,	0x02,	0x18,	0xc0,
	0x96,	0xfd,	0x18,	0xc6,	0x33,	0xee,	0x18,	0xc6,
	0x33,	0xef,	0x18,	0xc0,	0xbe,	0xc9,	0x18,	0xc0,
	0xbe,	0xca,	0x18,	0xc0,	0xc3,	0x69,	0x18,	0xc1,
	0x20,	0x10,	0x18,	0xc1,	0x20
#endif
};

void
test1(unsigned int as_size)
{
    ASSegment seq1 = ASSegment(AS_SEQUENCE);
    for (unsigned int i = 0; i < as_size; i++)
	seq1.add_as(AsNum(10));

    size_t len;
    fprintf(stderr, "trying size %u wire_size %u\n",
	    as_size, XORP_UINT_CAST(seq1.wire_size()));
    const uint8_t *d = seq1.encode(len, NULL);
    ASSegment *seq2 = new ASSegment(d);
    delete[] d;

    ASSegment seq3(*seq2);
    delete seq2;
}

int
main(int, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    EventLoop eventloop;
    LocalData localdata(eventloop);
    Iptuple iptuple;
    BGPPeerData* pd = new BGPPeerData(localdata, iptuple, AsNum(0), IPv4(),0);
    pd->compute_peer_type();
    BGPMain main(eventloop);
    BGPPeer peer(&localdata, pd, NULL, &main);


    try {
 	UpdatePacket pac(&buffer[0], sizeof(buffer), pd, &main, true);
	
	for(int i = 0; i < 2048; i++)
	    test1(i);

    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
