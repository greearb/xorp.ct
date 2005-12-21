// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_table_set_iphelper.cc,v 1.3 2005/08/23 22:29:10 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_set.hh"

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "libxorp/win_io.h"

//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to set the information is the IP Helper API for
// Windows (IPHLPAPI.DLL).
//

#ifdef HOST_OS_WINDOWS
//
// Helper method to determine if the Routing and Remote Access Service
// is installed and running.
//
static bool
is_rras_running()
{
    bool is_installed = false;
    bool is_running = false;

    SC_HANDLE h_scm = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (h_scm != NULL) {
    	SC_HANDLE h_rras = OpenService(h_scm, RRAS_SERVICE_NAME, GENERIC_READ);
	if (h_rras != NULL) {
	    is_installed = true;
	    SERVICE_STATUS ss;
	    if (0 != ControlService(h_rras, SERVICE_CONTROL_INTERROGATE, &ss)) {
		is_running = true;
	    } else {
		DWORD result = GetLastError();
		if (result == ERROR_SERVICE_CANNOT_ACCEPT_CTRL) {
		    is_running = true;
		} else if (result != ERROR_SERVICE_NOT_ACTIVE) {
		    XLOG_WARNING("ControlService() failed: %s",
				 win_strerror(result));
		}
	    }
	    CloseServiceHandle(h_rras);
	} else {
	    DWORD result = GetLastError();
	    if (result != ERROR_SERVICE_DOES_NOT_EXIST) {
		XLOG_WARNING("OpenService() failed: %s", win_strerror(result));
	    }
	}
        CloseServiceHandle(h_scm);
    } else {
	XLOG_WARNING("OpenSCManager() failed: %s",
		     win_strerror(GetLastError()));
    }
    return (is_running && is_installed);
}
#endif

FtiConfigTableSetIPHelper::FtiConfigTableSetIPHelper(FtiConfig& ftic)
    : FtiConfigTableSet(ftic)
{
#ifdef HOST_OS_WINDOWS
    register_ftic_primary();

    if (is_rras_running()) {
	XLOG_WARNING("Windows Routing and Remote Access Service is running.\n"
		     "Some change operations through IP Helper may not work.");
    }
#endif
}

FtiConfigTableSetIPHelper::~FtiConfigTableSetIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper mechanism to set "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigTableSetIPHelper::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    // Cleanup any leftover entries from previously run XORP instance
    delete_all_entries4();
    delete_all_entries6();

    //
    // XXX: This mechanism relies on the FtiConfigEntrySet mechanism
    // to set the forwarding table, hence there is nothing else to do.
    //

    _is_running = true;

    return (XORP_OK);

    UNUSED(error_msg);
}

int
FtiConfigTableSetIPHelper::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    delete_all_entries4();
    delete_all_entries6();

    //
    // XXX: This mechanism relies on the FtiConfigEntrySet mechanism
    // to set the forwarding table, hence there is nothing else to do.
    //

    _is_running = false;

    return (XORP_OK);

    UNUSED(error_msg);
}

bool
FtiConfigTableSetIPHelper::set_table4(const list<Fte4>& fte_list)
{
    list<Fte4>::const_iterator iter;

    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	ftic().add_entry4(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetIPHelper::delete_all_entries4()
{
    list<Fte4> fte_list;
    list<Fte4>::const_iterator iter;
    
    // Get the list of all entries
    ftic().get_table4(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	if (fte.xorp_route())
	    ftic().delete_entry4(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetIPHelper::set_table6(const list<Fte6>& fte_list)
{
    list<Fte6>::const_iterator iter;
    
    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	ftic().add_entry6(fte);
    }
    
    return true;
}
    
bool
FtiConfigTableSetIPHelper::delete_all_entries6()
{
    list<Fte6> fte_list;
    list<Fte6>::const_iterator iter;
    
    // Get the list of all entries
    ftic().get_table6(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	if (fte.xorp_route())
	    ftic().delete_entry6(fte);
    }
    
    return true;
}
