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

#ident "$XORP: xorp/fea/fticonfig.cc,v 1.3 2003/05/10 00:06:39 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"

//
// Unicast forwarding table related configuration.
//


FtiConfig::FtiConfig(EventLoop& eventloop)
    : _eventloop(eventloop),
      _ftic_entry_get(NULL), _ftic_entry_set(NULL), _ftic_entry_observer(NULL),
      _ftic_table_get(NULL), _ftic_table_set(NULL), _ftic_table_observer(NULL),
      _ftic_entry_get_dummy(*this),
      _ftic_entry_get_netlink(*this),
      _ftic_entry_get_rtsock(*this),
      _ftic_entry_set_dummy(*this),
      _ftic_entry_set_rtsock(*this),
      _ftic_entry_observer_dummy(*this),
      _ftic_entry_observer_rtsock(*this),
      _ftic_table_get_dummy(*this),
      _ftic_table_get_netlink(*this),
      _ftic_table_get_sysctl(*this),
      _ftic_table_set_dummy(*this),
      _ftic_table_set_rtsock(*this),
      _ftic_table_observer_dummy(*this),
      _ftic_table_observer_rtsock(*this)
{
    
}

int
FtiConfig::register_ftic_entry_get(FtiConfigEntryGet *ftic_entry_get)
{
    _ftic_entry_get = ftic_entry_get;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_set(FtiConfigEntrySet *ftic_entry_set)
{
    _ftic_entry_set = ftic_entry_set;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_observer(FtiConfigEntryObserver *ftic_entry_observer)
{
    _ftic_entry_observer = ftic_entry_observer;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_get(FtiConfigTableGet *ftic_table_get)
{
    _ftic_table_get = ftic_table_get;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_set(FtiConfigTableSet *ftic_table_set)
{
    _ftic_table_set = ftic_table_set;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_observer(FtiConfigTableObserver *ftic_table_observer)
{
    _ftic_table_observer = ftic_table_observer;
    
    return (XORP_OK);
}

int
FtiConfig::set_dummy()
{
    register_ftic_entry_get(&_ftic_entry_get_dummy);
    register_ftic_entry_set(&_ftic_entry_set_dummy);
    register_ftic_entry_observer(&_ftic_entry_observer_dummy);
    register_ftic_table_get(&_ftic_table_get_dummy);
    register_ftic_table_set(&_ftic_table_set_dummy);
    register_ftic_table_observer(&_ftic_table_observer_dummy);
    
    return (XORP_OK);
}

int
FtiConfig::start()
{
    if (_ftic_entry_get != NULL) {
	if (_ftic_entry_get->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_entry_observer != NULL) {
	if (_ftic_entry_observer->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_table_get != NULL) {
	if (_ftic_table_get->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_table_observer != NULL) {
	if (_ftic_table_observer->start() < 0)
	    return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
FtiConfig::stop()
{
    int ret_value = XORP_OK;
    
    if (_ftic_table_observer != NULL) {
	if (_ftic_table_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_table_get != NULL) {
	if (_ftic_table_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_entry_observer != NULL) {
	if (_ftic_entry_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_entry_get != NULL) {
	if (_ftic_entry_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

bool
FtiConfig::start_configuration()
{
    bool ret_value = false;

    //
    // XXX: we need to call start_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->start_configuration() == true)
	    ret_value = true;
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->start_configuration() == true)
	    ret_value = true;
    }
    
    return ret_value;
}

bool
FtiConfig::end_configuration()
{
    bool ret_value = false;
    
    //
    // XXX: we need to call end_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->end_configuration() == true)
	    ret_value = true;
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->end_configuration() == true)
	    ret_value = true;
    }
    
    return ret_value;
}

bool
FtiConfig::add_entry4(const Fte4& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->add_entry4(fte));
}

bool
FtiConfig::delete_entry4(const Fte4& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->delete_entry4(fte));
}

bool
FtiConfig::set_table4(const list<Fte4>& fte_list)
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->set_table4(fte_list));
}

bool
FtiConfig::delete_all_entries4()
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->delete_all_entries4());
}

bool
FtiConfig::lookup_route4(const IPv4& dst, Fte4& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_route4(dst, fte));
}

bool
FtiConfig::lookup_entry4(const IPv4Net& dst, Fte4& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_entry4(dst, fte));
}

bool
FtiConfig::get_table4(list<Fte4>& fte_list)
{
    if (_ftic_table_get == NULL)
	return false;
    return (_ftic_table_get->get_table4(fte_list));
}

bool
FtiConfig::add_entry6(const Fte6& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->add_entry6(fte));
}

bool
FtiConfig::delete_entry6(const Fte6& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->delete_entry6(fte));
}

bool
FtiConfig::set_table6(const list<Fte6>& fte_list)
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->set_table6(fte_list));
}

bool
FtiConfig::delete_all_entries6()
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->delete_all_entries6());
}

bool
FtiConfig::lookup_route6(const IPv6& dst, Fte6& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_route6(dst, fte));
}

bool
FtiConfig::lookup_entry6(const IPv6Net& dst, Fte6& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_entry6(dst, fte));
}

bool
FtiConfig::get_table6(list<Fte6>& fte_list)
{
    if (_ftic_table_get == NULL)
	return false;
    return (_ftic_table_get->get_table6(fte_list));
}

