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

#ident "$XORP: xorp/rib/rib_manager.cc,v 1.6 2003/03/16 07:18:58 pavlin Exp $"

#include "config.h"

#include "rib_module.h"
#include "rib_manager.hh"

RibManager::RibManager(EventLoop& event_loop, XrlStdRouter& xrl_std_router)
    : _event_loop(event_loop),
      _xrl_rtr(xrl_std_router),
      _fea(_xrl_rtr),
      _rserv(&_xrl_rtr),
      _urib4(UNICAST),
      _mrib4(MULTICAST),
      _urib6(UNICAST),
      _mrib6(MULTICAST),
      _vifmanager(_xrl_rtr, _event_loop, this),
      _xrt(&_xrl_rtr, _urib4, _mrib4, _urib6, _mrib6, _vifmanager, this)
{
    _urib4.initialize_export(&_fea);
    _urib4.initialize_register(&_rserv);
    if (_urib4.add_igp_table("connected") < 0) {
	XLOG_ERROR("Could not add igp table \"connected\" for urib4");
	return;
    }

    _mrib4.initialize_export(&_fea);
    _mrib4.initialize_register(&_rserv);
    if (_mrib4.add_igp_table("connected") < 0) {
	XLOG_ERROR("Could not add igp table \"connected\" for mrib4");
	return;
    }

    _urib6.initialize_export(&_fea);
    _urib6.initialize_register(&_rserv);
    if (_urib6.add_igp_table("connected") < 0) {
	XLOG_ERROR("Could not add igp table \"connected\" for urib6");
	return;
    }

    _mrib6.initialize_export(&_fea);
    _mrib6.initialize_register(&_rserv);
    if (_mrib6.add_igp_table("connected") < 0) {
	XLOG_ERROR("Could not add igp table \"connected\" for mrib6");
	return;
    }

    _vifmanager.start();
}

void
RibManager::run_event_loop() 
{
    while (true)
	_event_loop.run();
}

int
RibManager::new_vif(const string& vifname, const Vif& vif, string& err) 
{
    if (_urib4.new_vif(vifname, vif)) {
	err = (c_format("Failed to add vif \"%s\" to unicast IPv4 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib4.new_vif(vifname, vif)) {
	err = (c_format("Failed to add vif \"%s\" to multicast IPv4 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }

    if (_urib6.new_vif(vifname, vif)) {
	err = (c_format("Failed to add vif \"%s\" to unicast IPv6 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib6.new_vif(vifname, vif)) {
	err = (c_format("Failed to add vif \"%s\" to multicast IPv6 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }
    return XORP_OK;
}

int
RibManager::delete_vif(const string& vifname, string& err) 
{
    if (_urib4.delete_vif(vifname)) {
	err = (c_format("Failed to delete vif \"%s\" from unicast IPv4 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib4.delete_vif(vifname)) {
	err = (c_format("Failed to delete vif \"%s\" from multicast IPv4 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }

    if (_urib6.delete_vif(vifname)) {
	err = (c_format("Failed to delete vif \"%s\" from unicast IPv6 rib",
			vifname.c_str()));
	return XORP_ERROR;
    }

    if (_mrib6.delete_vif(vifname)) {
	err = (c_format("Failed to delete vif \"%s\" from multicast IPv6 rib",
			     vifname.c_str()));
	return XORP_ERROR;
    }
    return XORP_OK;
}

int
RibManager::add_vif_addr(const string& vifname,
		      const IPv4& addr,
		      const IPNet<IPv4>& subnet,
		      string& err) 
{
    if (_urib4.add_vif_address(vifname, addr, subnet)) {
	err = "Failed to add IPv4 Vif address to unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib4.add_vif_address(vifname, addr, subnet)) {
	err = "Failed to add IPv4 Vif address to multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
RibManager::delete_vif_addr(const string& vifname,
			 const IPv4& addr,
			 string& err) 
{
    if (_urib4.delete_vif_address(vifname, addr)) {
	err = "Failed to delete IPv4 Vif address from unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib4.delete_vif_address(vifname, addr)) {
	err = "Failed to delete IPv4 Vif address from multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
RibManager::add_vif_addr(const string& vifname,
		      const IPv6& addr,
		      const IPNet<IPv6>& subnet,
		      string& err) 
{
    if (_urib6.add_vif_address(vifname, addr, subnet)) {
	err = "Failed to add IPv6 Vif address to unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib6.add_vif_address(vifname, addr, subnet)) {
	err = "Failed to add IPv6 Vif address to multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

int
RibManager::delete_vif_addr(const string& vifname,
			 const IPv6& addr,
			 string& err) 
{
    if (_urib6.delete_vif_address(vifname, addr)) {
	err = "Failed to delete IPv6 Vif address from unicast RIB";
	return XORP_ERROR;
    }

    if (_mrib6.delete_vif_address(vifname, addr)) {
	err = "Failed to delete IPv6 Vif address from multicast RIB";
	return XORP_ERROR;
    }

    return XORP_OK;
}

void
RibManager::set_fea_enabled(bool en)
{
    _fea.set_enabled(en);
}

bool
RibManager::fea_enabled() const
{
    return _fea.enabled();
}

