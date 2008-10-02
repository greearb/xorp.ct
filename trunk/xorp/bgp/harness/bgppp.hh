// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/harness/bgppp.hh,v 1.9 2008/07/23 05:09:42 pavlin Exp $

#ifndef __BGP_HARNESS_BGPPP_HH__
#define __BGP_HARNESS_BGPPP_HH__

class BGPPeerData;
string bgppp(const uint8_t *buf, const size_t len, const BGPPeerData* peerdata);

#endif // __BGP_HARNESS_BGPPP_HH__
