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

// $XORP: xorp/bgp/bgp_varrw_export.hh,v 1.8 2008/11/08 06:14:36 mjh Exp $

#ifndef __BGP_BGP_VARRW_EXPORT_HH__
#define __BGP_BGP_VARRW_EXPORT_HH__

#include "bgp_varrw.hh"

/**
 * @short Returns the output peering for the neighbor variable.
 *
 * The neighbor variable in a dest {} policy block refers to the output peer
 * where the advertisement is being sent.
 */
template <class A>
class BGPVarRWExport : public BGPVarRW<A> {
public:
    /**
     * same as BGPVarRW but returns a different neighbor.
     *
     * @param name the name of the filter printed while tracing.
     * @param neighbor value to return for neighbor variable.
     */
    BGPVarRWExport(const string& name, const string& neighbor);

protected:
    /**
     * read the neighbor variable---the peer the advertisement is about to be
     * sent to.
     *
     * @return the neighbor variable.
     */
     Element* read_neighbor();

private:
    const string _neighbor;
};

#endif // __BGP_BGP_VARRW_EXPORT_HH__
