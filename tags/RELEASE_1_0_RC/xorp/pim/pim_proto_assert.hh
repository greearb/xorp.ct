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

// $XORP: xorp/pim/pim_proto_assert.hh,v 1.3 2003/03/10 23:20:50 hodson Exp $


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
