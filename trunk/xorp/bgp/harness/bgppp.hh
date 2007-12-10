// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/bgp/harness/bgppp.hh,v 1.6 2007/02/16 22:45:25 pavlin Exp $

#ifndef __BGP_HARNESS_BGPPP_HH__
#define __BGP_HARNESS_BGPPP_HH__

class BGPPeerData;
string bgppp(const uint8_t *buf, const size_t len, const BGPPeerData* peerdata);

#endif // __BGP_HARNESS_BGPPP_HH__
