// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/fibconfig_entry_set.hh,v 1.7 2007/06/04 23:17:32 pavlin Exp $

#ifndef __FEA_FIBCONFIG_ENTRY_SET_HH__
#define __FEA_FIBCONFIG_ENTRY_SET_HH__

#include "fte.hh"
#include "iftree.hh"

class FibConfig;


class FibConfigEntrySet {
public:
    FibConfigEntrySet(FibConfig& fibconfig)
	: _is_running(false),
	  _fibconfig(fibconfig),
	  _in_configuration(false),
	  _is_primary(true)
    {}
    virtual ~FibConfigEntrySet() {}
    
    FibConfig&	fibconfig() { return _fibconfig; }
    
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }
    virtual bool is_running() const { return _is_running; }

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;
    
    /**
     * Start a configuration interval. All modifications to FibConfig
     * state must be within a marked "configuration" interval.
     *
     * This method provides derived classes with a mechanism to perform
     * any actions necessary before forwarding table modifications can
     * be made.
     *
     * @param error_msg the error message (if error).
     * @return true if configuration successfully started.
     */
    virtual bool start_configuration(string& error_msg) {
	// Nothing particular to do, just label start.
	return mark_configuration_start(error_msg);
    }

    /**
     * End of configuration interval.
     *
     * This method provides derived classes with a mechanism to
     * perform any actions necessary at the end of a configuration, eg
     * write a file.
     *
     * @param error_msg the error message (if error).
     * @return true configuration success pushed down into forwarding table.
     */
    virtual bool end_configuration(string& error_msg) {
	// Nothing particular to do, just label start.
	return mark_configuration_end(error_msg);
    }
    
    /**
     * Add a single routing entry.  Must be within a configuration
     * interval.
     *
     * @param fte the entry to add.
     *
     * @return true on success, otherwise false.
     */
    virtual bool add_entry4(const Fte4& fte) = 0;

    /**
     * Delete a single routing entry. Must be with a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_entry4(const Fte4& fte) = 0;

    /**
     * Add a single routing entry. Must be within a configuration
     * interval.
     *
     * @param fte the entry to add.
     *
     * @return true on success, otherwise false.
     */
    virtual bool add_entry6(const Fte6& fte) = 0;

    /**
     * Delete a single routing entry.  Must be within a configuration
     * interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_entry6(const Fte6& fte) = 0;

protected:
    /**
     * Mark start of a configuration.
     *
     * @param error_msg the error message (if error).
     * @return true if configuration can be marked as started, false otherwise.
     */
    bool mark_configuration_start(string& error_msg) {
	if (false == _in_configuration) {
	    _in_configuration = true;
	    return true;
	}
	error_msg = c_format("Cannot start configuration: "
			     "configuration in progress");
	return false;
    }

    /**
     * Mark end of a configuration.
     *
     * @param error_msg the error message (if error).
     * @return true if configuration can be marked as ended, false otherwise.
     */
    bool mark_configuration_end(string& error_msg) {
	if (true == _in_configuration) {
	    _in_configuration = false;
	    return true;
	}
	error_msg = c_format("Cannot end configuration: "
			     "configuration not in progress");
	return false;
    }
    
    bool in_configuration() const { return _in_configuration; }

    // Misc other state
    bool	_is_running;

private:
    FibConfig&	_fibconfig;
    bool	_in_configuration;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

#endif // __FEA_FIBCONFIG_ENTRY_SET_HH__
