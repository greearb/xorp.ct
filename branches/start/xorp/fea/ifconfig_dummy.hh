// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_dummy.hh,v 1.26 2002/12/09 18:28:57 hodson Exp $

#ifndef __FEA_IFCONFIG_DUMMY_HH__
#define __FEA_IFCONFIG_DUMMY_HH__

#include <vector>
#include <net/if.h>

#include "ifconfig.hh"

/**
 * Dummy platform interface.  Accepts any IfTree supplied and as a
 * result reports no errors.  Solely intended for testing purposes and
 * evaluation purposes when a suitable platform configuration class does
 * not exist.
 */
class IfConfigDummy : public IfConfig {
public:
    IfConfigDummy(IfConfigUpdateReporterBase& ur,
		  SimpleIfConfigErrorReporter&  er) : IfConfig(ur, er) {}

    bool push_config(const IfTree& config);

    const IfTree& pull_config(IfTree& config);

private:
    IfTree _config;
};

#endif // __FEA_IFCONFIG_DUMMY_HH__
