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

// $XORP: xorp/fea/ifconfig.hh,v 1.4 2003/05/02 23:21:38 pavlin Exp $

#ifndef __FEA_IFCONFIG_HH__
#define __FEA_IFCONFIG_HH__

#include "ifconfig_get.hh"
#include "ifconfig_set.hh"
#include "ifconfig_observer.hh"
#include "iftree.hh"

class EventLoop;
class IfConfigGet;
class IfConfigSet;
class IfConfigObserver;
class IfConfigErrorReporterBase;
class IfConfigUpdateReporterBase;


/**
 * Base class for pushing and pulling interface configurations down to
 * the particular platform.
 */
class IfConfig {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop.
     * @param ur update reporter that receives updates through when
     *           configurations are pushed down and when triggered
     *		 spontaneously on the underlying platform.
     * @param er error reporter that errors are propagated through when
     *           configurations are pushed down.
     */
    IfConfig(EventLoop& eventloop, IfConfigUpdateReporterBase& ur,
	     IfConfigErrorReporterBase& er);

    /**
     * Virtual destructor (in case this class is used as base class).
     */
    virtual ~IfConfig() {}
    
    EventLoop& eventloop() { return _eventloop; }
    
    /**
     * Get error reporter associated with IfConfig.
     */
    inline IfConfigErrorReporterBase&	er() { return _er; }

    IfTree& live_config() { return (_live_config); }
    void    set_live_config(const IfTree& it) { _live_config = it; }
    
    int register_ifc_get(IfConfigGet *ifc_get);
    int register_ifc_set(IfConfigSet *ifc_set);
    int register_ifc_observer(IfConfigObserver *ifc_observer);

    IfConfigGet&	ifc_get() { return *_ifc_get; }
    IfConfigSet&	ifc_set() { return *_ifc_set; }
    IfConfigObserver&	ifc_observer() { return *_ifc_observer; }

    /**
     * Setup the unit to behave as dummy (for testing purpose).
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_dummy();
    
    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop();
    
    /**
     * Push IfTree structure down to platform.  Errors are reported
     * via the constructor supplied ErrorReporter instance.
     *
     * @param config the configuration to be pushed down.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int push_config(const IfTree& config);
    
    /**
     * Pull up current config from platform.
     *
     * @param config an IfTree item that can be used to write the
     * config to.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int pull_config(IfTree& config);
    
    void flush_config() { _live_config = IfTree(); }
    
    IfTreeInterface *get_if(IfTree& it, const string& ifname);
    IfTreeVif *get_vif(IfTree& it, const string& ifname,
		       const string& vifname);
    
    /**
     * Get error message associated with push operation.
     */
    const string& push_error() const;
    
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

    void map_ifindex(uint32_t index, const string& name);
    void unmap_ifindex(uint32_t index);    
    const char* get_ifname(uint32_t index);
    
private:
    
    EventLoop&			_eventloop;
    IfConfigUpdateReporterBase&	_ur;
    IfConfigErrorReporterBase&	_er;
    
    // A cache of associative array of interface names to interface index.
    // Needed because the RTM_IFANNOUNCE upcall is called after the interface
    // name is deleted from the kernel.
    typedef map<uint32_t, string> IfIndex2NameMap;
    IfIndex2NameMap	_ifnames;
    
    IfTree		_live_config;
    
    IfConfigGet		*_ifc_get;
    IfConfigSet		*_ifc_set;
    IfConfigObserver	*_ifc_observer;
    
    //
    // The mechanisms to get interface-related information
    // from the underlying system.
    // Ordering is important: the last that is supported is the one to use.
    //
    IfConfigGetDummy	_ifc_get_dummy;
    IfConfigGetIoctl	_ifc_get_ioctl;
    IfConfigGetSysctl	_ifc_get_sysctl;
    IfConfigGetGetifaddrs _ifc_get_getifaddrs;
    
    //
    // The mechanisms to set interface-related information
    // within the underlying system.
    // Ordering is important: the last that is supported is the one to use.
    //
    IfConfigSetDummy	_ifc_set_dummy;
    IfConfigSetIoctl	_ifc_set_ioctl;
    
    //
    // The mechanisms to observe whether the interface-related information
    // within the underlying system has changed.
    // Ordering is important: the last that is supported is the one to use.
    //
    IfConfigObserverDummy _ifc_observer_dummy;
    IfConfigObserverRtsock _ifc_observer_rtsock;
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
    IfConfigErrorReporterBase() : _error_cnt(0) {}
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
    inline uint32_t error_count() const		{ return _error_cnt; }
    
    /**
     * Reset error count and error message.
     */
    inline void reset() {
	_first_error.erase();
	_last_error.erase();
	_error_cnt = 0;
    }
    
    inline void	log_error(const string& s) {
	if (0 == _error_cnt)
	    _first_error = s;
	_last_error = s;
	_error_cnt++;
    }
    
private:
    string	_last_error;	// last error reported	
    string	_first_error;	// first error reported

    uint32_t	_error_cnt;	// number of errors reported
};

/**
 * @short Class for propagating error information during IfConfig operations.
 *
 */
class IfConfigErrorReporter : public IfConfigErrorReporterBase {
public:
    IfConfigErrorReporter();

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

private:
};

#endif // __FEA_IFCONFIG_HH__
