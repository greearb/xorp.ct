// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/rib/parser_direct_cmds.hh,v 1.5 2003/09/27 22:32:45 mjh Exp $

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
	// safe to just say it's an IGP, even if its not
	return _rib.new_origin_table(_tablename, "", "", _admin_distance, IGP);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectTableMergedCommand : public TableMergedCommand {
public:
    DirectTableMergedCommand(RIB<IPv4>& rib)
	: TableMergedCommand(), _rib(rib) {}
    int execute() {
	cout << "TableMergedCommand::execute " << _tablename << "\n";
	return _rib.new_merged_table(_tablename, _t1, _t2);
    }
private:
    RIB<IPv4>& _rib;
};

class DirectTableExtIntCommand : public TableExtIntCommand {
public:
    DirectTableExtIntCommand(RIB<IPv4>& rib)
	: TableExtIntCommand(), _rib(rib) {}
    int execute() {
	cout << "TableExtIntCommand::execute " << _tablename << "\n";
	return _rib.new_extint_table(_tablename, _t1, _t2);
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
	return _rib.add_route(_tablename, _net, _nexthop, 
			      (uint32_t)_metric);
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
	cout << "RouteVerifyCommand::execute " << _lookupaddr.str() << " "
	     << _ifname << " " << _nexthop.str() << " "
	     << c_format("%d", _metric) << "\n";
	int dummy = _rib.verify_route(_lookupaddr, _ifname, _nexthop, 
				      (uint32_t)_metric);
	if (dummy != XORP_OK) {
	    cerr << "RouteVerify Failed!\n";
	    abort();
	}
	return dummy;
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

class DirectRedistEnableCommand : public RedistEnableCommand {
public:
    DirectRedistEnableCommand(RIB<IPv4>& rib)
	: RedistEnableCommand(), _rib(rib) {}
    int execute() {
	cout << "RedistEnableCommand::execute " << _fromtable << " ";
	cout << _totable << "\n";
	int dummy = _rib.redist_enable(_fromtable, _totable);
	return dummy;
    }
private:
    RIB<IPv4>& _rib;
};

class DirectRedistDisableCommand : public RedistDisableCommand {
public:
    DirectRedistDisableCommand(RIB<IPv4>& rib) :
	RedistDisableCommand(), _rib(rib) {}
    int execute() {
	cout << "RedistDisableCommand::execute " << _fromtable << " ";
	cout << _totable << "\n";
	int dummy = _rib.redist_disable(_fromtable, _totable);
	return dummy;
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
