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

#ident "$XORP: xorp/fea/demo_fea_ifmgr_client.cc,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $"

//
// This program is a simple example of what is required to be an interface
// event observer.  It largely exists for testing purposes, but may serve as
// a helpful example.
//

#include <string>
#include <iostream>

#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/targets/demo_fea_ifmgr_client_base.hh"
#include "xrl/interfaces/fea_ifmgr_xif.hh"

enum IfEvent {
    CREATED = 1,
    DELETED = 2,
    CHANGED = 3
};

const char * const
if_event(const uint32_t& i)
{
    switch (IfEvent(i)) {
    case CREATED: return "Creation";
    case DELETED: return "Deletion";
    case CHANGED: return "Change";
    }
    return "Unhandled";
}

class DemoFeaIfmgrClientTarget : public XrlDemoFeaIfmgrClientTargetBase
{
public:
    DemoFeaIfmgrClientTarget(XrlRouter& rtr, ostream& os)
	: XrlDemoFeaIfmgrClientTargetBase(&rtr), _os(os), _registered(false) {}

    XrlCmdError common_0_1_get_target_name(string&	name)
    {
	name = _router->name();
	return XrlCmdError::OKAY();
    };

    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version)
    {
	version = "0.1";
	return XrlCmdError::OKAY();
    }

    XrlCmdError fea_ifmgr_client_0_1_interface_update(
	// Input values,
	const string&	ifname,
	const uint32_t&	event)
    {
	_os << if_event(event) << " Event on interface " << ifname << endl;
	return XrlCmdError::OKAY();
    }

    XrlCmdError fea_ifmgr_client_0_1_vif_update(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	event)
    {
	_os << if_event(event) << " Event on vif " << vifname
	    << " on interface " << ifname << endl;
	return XrlCmdError::OKAY();
    }

    XrlCmdError fea_ifmgr_client_0_1_vifaddr4_update(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t&	event)
    {
	_os << if_event(event) << " Event on address " << addr.str()
	    << " on vif " << vifname << " on interface " << ifname << endl;
	return XrlCmdError::OKAY();
    }

    XrlCmdError fea_ifmgr_client_0_1_vifaddr6_update(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const uint32_t&	event)
    {
	_os << if_event(event) << " Event on address " << addr.str()
	    << " on vif " << vifname << " on interface " << ifname << endl;
	return XrlCmdError::OKAY();
    }

    void registration_result(const XrlError& e, string who)
    {
	if (XrlError::OKAY() == e) {
	    _os << "Registered with " << who << "." << endl;
	    _registered = true;
	    return;
	} else {
	    _os << "Register returned " << e.str() << endl;
	}
    }

    void register_with(const string& who)
    {
	XrlRouter* rtr = static_cast<XrlRouter*>(_router);
	XrlIfmgrV0p1Client ifmgr_client(rtr);
	ifmgr_client.send_register_client(
	    who.c_str(), _router->name(),
	    callback(this,
		     &DemoFeaIfmgrClientTarget::registration_result, who));
	_os << "Sending Register to " << who << endl;
    }

    bool registered() const { return _registered; }

protected:
    ostream&	_os;
    bool	_registered;
};

bool send_register(DemoFeaIfmgrClientTarget* client)
{
    if (client->registered())
	return false; // nothing more to do

    client->register_with("fea");
    return true;
}

void demo_main()
{
    EventLoop e;
    XrlStdRouter rtr(e, "demofeaifmgrclient");
    DemoFeaIfmgrClientTarget client(rtr, cout);

    send_register(&client);
    XorpTimer r = e.new_periodic(5000, callback(send_register, &client));

    for (;;) {
	e.run();
    }
}

int main(int, char* const *argv)
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	demo_main();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
