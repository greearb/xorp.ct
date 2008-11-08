// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/bgp_varrw_export.hh,v 1.6 2008/07/23 05:09:32 pavlin Exp $

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
