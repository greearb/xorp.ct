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

#ident "$XORP: xorp/bgp/update_test.cc,v 1.9 2003/03/10 23:20:08 hodson Exp $"

#include "bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "packet.hh"

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
    AsSegment seq1 = AsSegment(AS_SEQUENCE);
    for (unsigned int i = 0; i < as_size; i++)
	seq1.add_as(AsNum(10));

    size_t len;
    fprintf(stderr, "trying size %d wire_size %u\n",
	    as_size, (uint32_t)seq1.wire_size());
    const uint8_t *d = seq1.encode(len, NULL);
    AsSegment *seq2 = new AsSegment(d);
    delete[] d;

    AsSegment seq3(*seq2);
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

    try {
 	UpdatePacket pac(&buffer[0], sizeof(buffer));
	
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
