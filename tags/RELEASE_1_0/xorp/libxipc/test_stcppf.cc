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

#ident "$XORP: xorp/libxipc/test_stcppf.cc,v 1.4 2003/03/16 08:20:30 pavlin Exp $"

#include <stdio.h>

#include "xrl_module.h"
#include "config.h"
#include "xrl_error.hh"
#include "xrl_pf_stcp_ph.hh"

static void
test_packet_header(bool is_request, const XrlError& e, uint32_t p_bytes)
{
    printf("Testing STCPPacketHeader(%s, \"%s\", %ul)... ",
	   is_request ? "true" : "false", string(e).c_str(), p_bytes);
    STCPPacketHeader sph(is_request, e, p_bytes);

    if (sph.is_valid() == false) {
	printf("invalid header\n");
    } else if (sph.is_request() != is_request) {
	printf("request bit corrupted.\n");
    } else if (sph.status() != e) {
	printf("error identifier corrupted.\n");
    } else if (sph.payload_bytes() != p_bytes) {
	printf("payload bytes corrupted.\n");
    } else {
	printf("okay.\n");
    }
}

int main(int /* argc */, char *argv[])
{

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    test_packet_header(true, XrlError::OKAY(), 0xaabbccdd);
    test_packet_header(false, XrlError::SYSCALL_FAILED(), 0x10203040);

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
