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

// $XORP: xorp/fea/ifconfig.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef __FEA_IFCONFIG_HH__
#define __FEA_IFCONFIG_HH__

#include "iftree.hh"

class IfConfigUpdateReporterBase;
class SimpleIfConfigErrorReporter;

/**
 * Base class for pushing and pulling interface configurations down to
 * the particular platform.
 */
class IfConfig {
public:
    /**
     * Constructor.
     *
     * @param ur update reporter that receives updates through when
     *           configurations are pushed down and when triggered
     *		 spontaneously on the underlying platform.
     *
     * @param er error reporter that errors are propagated through when
     *           configurations are pushed down.
     */
    IfConfig(IfConfigUpdateReporterBase&  ur,
	     SimpleIfConfigErrorReporter& er)
	: _ur(ur), _er(er) {}

    virtual ~IfConfig() {}

    /**
     * Push IfTree structure down to platform.  Errors are reported
     * via the constructor supplied ErrorReporter instance.
     *
     * @param config the configuration to be pushed down.
     *
     * @return true on success, false if errors occurred.
     */
    virtual bool push_config(const IfTree& config) = 0;

    /**
     * Pull up current config from platform.
     *
     * @param config an IfTree item that can be used to write the
     * config to, if the underlying platform does maintain it's own
     * copy of the config.
     *
     * @return the platform IfTree.
     */
    virtual const IfTree& pull_config(IfTree& config) = 0;

    /**
     * Get error message associated with push operation.
     */
    const string& push_error() const;
    
protected:
    /**
     * Check IfTreeInterface and report updates to IfConfigUpdateReporter.
     */
    void   report_update(const IfTreeInterface& fi);

    /**
     * Check IfTreeVif and report updates to IfConfigUpdateReporter.
     */
    void   report_update(const IfTreeInterface& fi, const IfTreeVif& fv);

    /**
     * Check IfTreeAddr4 and report updates to IfConfigUpdateReporter.
     */
    void   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr4&     fa);

    /**
     * Check IfTreeAddr6 and report updates to IfConfigUpdateReporter.
     */
    void   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr6&     fa);

    /**
     * Check every item within IfTree and report updates to
     * IfConfigUpdateReporter.
     */
    void   report_updates(const IfTree& it);

    /**
     * Get error reporter associated with IfConfig.
     */
    inline SimpleIfConfigErrorReporter&	er() { return _er; }

private:
    IfConfigUpdateReporterBase&		_ur;
    SimpleIfConfigErrorReporter&	_er;
};

/**
 * @short Base class for propagating update information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * updates to an IfConfigUpdateReporter.  The @ref IfConfig instance
 * takes a pointer to the IfConfigUpdateReporter object it should use.
 */
class IfConfigUpdateReporterBase {
public:
    enum Update { CREATED, DELETED, CHANGED };

    virtual ~IfConfigUpdateReporterBase() {}

    virtual void interface_update(const string& ifname,
				  const Update& u) = 0;

    virtual void vif_update(const string& ifname,
			    const string& vifname,
			    const Update& u) = 0;

    virtual void vifaddr4_update(const string& ifname,
				 const string& vifname,
				 const IPv4&   addr,
				 const Update& u) = 0;

    virtual void vifaddr6_update(const string& ifname,
				 const string& vifname,
				 const IPv6&   addr,
				 const Update& u) = 0;
};

/**
 * @short Base class for propagating error information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * errors to an IfConfigErrorReporterBase.  The @ref IfConfig instance
 * takes a pointer to the IfConfigErrorReporterBase object it should use.
 */
class IfConfigErrorReporterBase {
public:
    virtual ~IfConfigErrorReporterBase() {}

    virtual void interface_error(const string& ifname,
				 const string& error_msg) = 0;

    virtual void vif_error(const string& ifname,
			   const string& vifname,
			   const string& error_msg) = 0;

    virtual void vifaddr_error(const string& ifname,
			       const string& vifname,
			       const IPv4&   addr,
			       const string& error_msg) = 0;

    virtual void vifaddr_error(const string& ifname,
			       const string& vifname,
			       const IPv6&   addr,
			       const string& error_msg) = 0;
};

/**
 * @short Class for propagating error information during IfConfig operations.
 *
 */
class SimpleIfConfigErrorReporter : public IfConfigErrorReporterBase {
public:
    SimpleIfConfigErrorReporter();

    void interface_error(const string& ifname,
			 const string& error_msg);

    void vif_error(const string& ifname,
		   const string& vifname,
		   const string& error_msg);

    void vifaddr_error(const string& ifname,
		       const string& vifname,
		       const IPv4&   addr,
		       const string& error_msg);

    void vifaddr_error(const string& ifname,
		       const string& vifname,
		       const IPv6&   addr,
		       const string& error_msg);

    /**
     * @return error message of first error encountered.
     */
    inline const string& first_error() const	{ return _first_error; }

    /**
     * @return error message of last error encountered.
     */
    inline const string& last_error() const	{ return _last_error; }

    /**
     * @return number of errors reported.
     */
    inline uint32_t error_count() const			{ return _error_cnt; }

    /**
     * Reset error count and error message.
     */
    inline void reset() {
	_first_error.erase();
	_last_error.erase();
	_error_cnt = 0;
    }

protected:
    inline void	log_error(const string& s) {
	if (0 == _error_cnt)
	    _first_error = s;
	_last_error = s;
	_error_cnt++;
    }

    string	_last_error;	// last error reported	
    string	_first_error;	// first error reported

    uint32_t	_error_cnt;	// number of errors reported
};

#endif // __FEA_IFCONFIG_HH__
