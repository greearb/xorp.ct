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

// $XORP: xorp/rib/parser_direct_cmds.hh,v 1.15 2004/11/02 22:52:14 bms Exp $

#ifndef __RIB_PARSER_DIRECT_CMDS_HH__
#define __RIB_PARSER_DIRECT_CMDS_HH__

#include "parser.hh"

// ----------------------------------------------------------------------------
// Direct RIB Commands (operate on an instance of a RIB<IPv4>).

class DirectTableOriginCommand : public TableOriginCommand {
public:
    DirectTableOriginCommand(RIB<IPv4>& rib)
	: TableOriginCommand(), _rib(rib) {}
    int execute() {
	cout << "TableOriginCommand::execute " << _tablename << "\n";
	// On the context of this test code, we don't care whether it's an
	// IGP or an EGP, because we do the plumbing explicitly.  So it's
	// safe to just say it's an IGP, even if its not.
	return _rib.new_origin_table(_tablename, "", "", _admin_distance, IGP);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectRouteAddCommand : public RouteAddCommand {
public:
    DirectRouteAddCommand(RIB<IPv4>& rib)
	: RouteAddCommand(), _rib(rib) {}
    int execute() {
	cout << "RouteAddCommand::execute " << _tablename << " ";
	cout << _net.str() << " " << _nexthop.str() << " "
	     << c_format("%d", _metric) << "\n";
	return _rib.add_route(_tablename, _net, _nexthop, "", "",
			      (uint32_t)_metric, PolicyTags());
    }
private:
    RIB<IPv4>& _rib;
};

class DirectRouteVifAddCommand : public RouteVifAddCommand {
public:
    DirectRouteVifAddCommand(RIB<IPv4>& rib)
	: RouteVifAddCommand(), _rib(rib) {}
    int execute() {
	cout << "RouteVifAddCommand::execute " << _tablename << " ";
	cout << _net.str() << " " << _vifname << " " << _nexthop.str() << " "
	     << c_format("%d", _metric) << "\n";
	return _rib.add_route(_tablename, _net, _nexthop, "", _vifname,
			      (uint32_t)_metric, PolicyTags());
    }
private:
    RIB<IPv4>& _rib;
};

class DirectRouteDeleteCommand : public RouteDeleteCommand {
public:
    DirectRouteDeleteCommand(RIB<IPv4>& rib)
	: RouteDeleteCommand(), _rib(rib) {}
    int execute() {
	cout << "RouteDeleteCommand::execute " << _tablename << " "
	     << _net.str() << endl;
	return _rib.delete_route(_tablename, _net);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectRouteVerifyCommand : public RouteVerifyCommand {
public:
    DirectRouteVerifyCommand(RIB<IPv4>& rib)
	: RouteVerifyCommand(), _rib(rib) {}
    int execute() {
	cout << "RouteVerifyCommand::execute " << _type
	     << " " << _lookupaddr.str()
	     << " " << _ifname
	     << " " << _nexthop.str()
	     << " " << c_format("%d", _metric)
	     << "\n";

	RibVerifyType verifytype;
	if (_type == "miss")
		verifytype = RibVerifyType(MISS);
	else if (_type == "discard")
		verifytype = RibVerifyType(DISCARD);
	else if (_type == "ip")
		verifytype = RibVerifyType(IP);
	else {
	    cerr <<
"RouteVerify Failed: invalid match type specification " << _type << "\n";
#ifndef TESTING_INTERACTIVELY
	    // XXX stub this out if interactive
	    abort();
#endif
	}

	int dummy = _rib.verify_route(_lookupaddr, _ifname, _nexthop,
				      (uint32_t)_metric, verifytype);
	if (dummy != XORP_OK) {
	    cerr << "RouteVerify Failed!\n";
#ifndef TESTING_INTERACTIVELY
	    // XXX stub this out if interactive
	    abort();
#endif
	}
#ifndef TESTING_INTERACTIVELY
	    // XXX stub this out if interactive
	return dummy;
#else
	return XORP_OK;
#endif
    }
private:
    RIB<IPv4>& _rib;
};

class DirectDiscardVifCommand : public DiscardVifCommand {
public:
    DirectDiscardVifCommand(RIB<IPv4>& rib)
	: DiscardVifCommand(), _rib(rib) {}
    int execute() {
	cout << "DiscardVifCommand::execute " << _ifname << " ";
	cout << _addr.str() << "\n";

	Vif vif(_ifname);
	IPv4Net subnet(_addr, _prefix_len);
	VifAddr vifaddr(_addr, subnet, IPv4::ZERO(), IPv4::ZERO());
	vif.add_address(vifaddr);
	vif.set_discard(true);
	cout << "**** Vif: " << vif.str() << endl;

	return _rib.new_vif(_ifname, vif);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectEtherVifCommand : public EtherVifCommand {
public:
    DirectEtherVifCommand(RIB<IPv4>& rib)
	: EtherVifCommand(), _rib(rib) {}
    int execute() {
	cout << "EtherVifCommand::execute " << _ifname << " ";
	cout << _addr.str() << "\n";

	Vif vif(_ifname);
	IPv4Net subnet(_addr, _prefix_len);
	VifAddr vifaddr(_addr, subnet, IPv4::ZERO(), IPv4::ZERO());
	vif.add_address(vifaddr);
	cout << "**** Vif: " << vif.str() << endl;

	return _rib.new_vif(_ifname, vif);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectLoopbackVifCommand : public LoopbackVifCommand {
public:
    DirectLoopbackVifCommand(RIB<IPv4>& rib)
	: LoopbackVifCommand(), _rib(rib) {}
    int execute() {
	cout << "LoopbackVifCommand::execute " << _ifname << " ";
	cout << _addr.str() << "\n";

	Vif vif(_ifname);
	IPv4Net subnet(_addr, _prefix_len);
	VifAddr vifaddr(_addr, subnet, IPv4::ZERO(), IPv4::ZERO());
	vif.add_address(vifaddr);
	vif.set_loopback(true);
	cout << "**** Vif: " << vif.str() << endl;

	return _rib.new_vif(_ifname, vif);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectAddIGPTableCommand : public AddIGPTableCommand {
public:
    DirectAddIGPTableCommand(RIB<IPv4>& rib) :
	AddIGPTableCommand(), _rib(rib) {}
    int execute() {
	cout << "AddIGPTableCommand::execute " << _tablename << "\n";
	return _rib.add_igp_table(_tablename, "", "");
    }
private:
    RIB<IPv4>& _rib;
};

class DirectDeleteIGPTableCommand : public DeleteIGPTableCommand {
public:
    DirectDeleteIGPTableCommand(RIB<IPv4>& rib) :
	DeleteIGPTableCommand(), _rib(rib) {}
    int execute() {
	cout << "DeleteIGPTableCommand::execute " << _tablename << "\n";
	return _rib.delete_igp_table(_tablename, "", "");
    }
private:
    RIB<IPv4>& _rib;
};

class DirectAddEGPTableCommand : public AddEGPTableCommand {
public:
    DirectAddEGPTableCommand(RIB<IPv4>& rib) :
	AddEGPTableCommand(), _rib(rib) {}
    int execute() {
	cout << "AddEGPTableCommand::execute " << _tablename << "\n";
	return _rib.add_egp_table(_tablename, "", "");
    }
private:
    RIB<IPv4>& _rib;
};

class DirectDeleteEGPTableCommand : public DeleteEGPTableCommand {
public:
    DirectDeleteEGPTableCommand(RIB<IPv4>& rib) :
	DeleteEGPTableCommand(), _rib(rib) {}
    int execute() {
	cout << "DeleteEGPTableCommand::execute " << _tablename << "\n";
	return _rib.delete_egp_table(_tablename, "", "");
    }
private:
    RIB<IPv4>& _rib;
};

#endif // __RIB_PARSER_DIRECT_CMDS_HH__
