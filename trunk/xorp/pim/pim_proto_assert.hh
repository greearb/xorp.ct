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

// $XORP: xorp/pim/pim_proto_assert.hh,v 1.11 2008/07/23 05:11:15 pavlin Exp $


#ifndef __PIM_PIM_PROTO_ASSERT_HH__
#define __PIM_PIM_PROTO_ASSERT_HH__


//
// PIM Assert related definitions.
//


#include "libxorp/ipvx.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

// Assert-related metric
class AssertMetric {
public:
    AssertMetric(const IPvX& addr) : _addr(addr) {}
    bool	rpt_bit_flag() const { return (_rpt_bit_flag); }
    void	set_rpt_bit_flag(bool v) { _rpt_bit_flag = v; }
    uint32_t	metric_preference() const { return (_metric_preference); }
    void	set_metric_preference(uint32_t v) { _metric_preference = v; }
    uint32_t	metric() const { return (_metric); }
    void	set_metric(uint32_t v) { _metric = v; }
    const IPvX&	addr() const { return (_addr); }
    void	set_addr(const IPvX& v) { _addr = v; }
    bool	operator>(const AssertMetric& other) const;
    bool	is_assert_cancel_metric() const;
    
private:
    bool	_rpt_bit_flag;		// The SPT/RPT bit
    uint32_t	_metric_preference;	// The metric preference
    uint32_t	_metric;		// The metric
    IPvX	_addr;			// The address of the Assert origin
};

enum assert_state_t {
    ASSERT_STATE_NOINFO = 0,
    ASSERT_STATE_WINNER,
    ASSERT_STATE_LOSER
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_PROTO_ASSERT_HH__
