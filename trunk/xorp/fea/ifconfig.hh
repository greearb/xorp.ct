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

// $XORP: xorp/fea/ifconfig.hh,v 1.31 2004/10/25 23:27:56 pavlin Exp $

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
class NexthopPortMapper;


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
     * @param nexthop_port_mapper the next-hop port mapper.
     */
    IfConfig(EventLoop& eventloop, IfConfigUpdateReporterBase& ur,
	     IfConfigErrorReporterBase& er,
	     NexthopPortMapper& nexthop_port_mapper);

    /**
     * Virtual destructor (in case this class is used as base class).
     */
    virtual ~IfConfig() { stop(); }

    /**
     * Get a reference to the @see EventLoop instance.
     *
     * @return a reference to the @see EventLoop instance.
     */
    EventLoop& eventloop() { return _eventloop; }

    /**
     * Get a reference to the @see NexthopPortMapper instance.
     *
     * @return a reference to the @see NexthopPortMapper instance.
     */
    NexthopPortMapper& nexthop_port_mapper() { return _nexthop_port_mapper; }

    /**
     * Get error reporter associated with IfConfig.
     */
    inline IfConfigErrorReporterBase&	er() { return _er; }

    IfTree& live_config() { return (_live_config); }
    void    set_live_config(const IfTree& it) { _live_config = it; }

    const IfTree& pulled_config() { return (_pulled_config); }

    int register_ifc_get_primary(IfConfigGet *ifc_get);
    int register_ifc_set_primary(IfConfigSet *ifc_set);
    int register_ifc_observer_primary(IfConfigObserver *ifc_observer);
    int register_ifc_get_secondary(IfConfigGet *ifc_get);
    int register_ifc_set_secondary(IfConfigSet *ifc_set);
    int register_ifc_observer_secondary(IfConfigObserver *ifc_observer);

    IfConfigGet&	ifc_get_primary() { return *_ifc_gets.front(); }
    IfConfigSet&	ifc_set_primary() { return *_ifc_sets.front(); }
    IfConfigObserver&	ifc_observer_primary() { return *_ifc_observers.front(); }

    IfConfigGet&	ifc_get_ioctl() { return _ifc_get_ioctl; }

    /**
     * Setup the unit to behave as dummy (for testing purpose).
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_dummy();

    /**
     * Test if running in dummy mode.
     * 
     * @return true if running in dummy mode, otherwise false.
     */
    bool is_dummy() const { return _is_dummy; }

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
     * Return true if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool have_ipv4() const { return _have_ipv4; }

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool have_ipv6() const { return _have_ipv6; }

    /**
     * Test if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool test_have_ipv4() const;

    /**
     * Test if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool test_have_ipv6() const;

    /**
     * Push IfTree structure down to platform.  Errors are reported
     * via the constructor supplied ErrorReporter instance.
     *
     * @param config the configuration to be pushed down.
     *
     * @return true on success, otherwise false.
     */
    bool push_config(const IfTree& config);

    /**
     * Pull up current config from platform.
     *
     * @return the platform IfTree.
     */
    const IfTree& pull_config();

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
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface& fi,
			 bool is_system_interfaces_reportee);

    /**
     * Check IfTreeVif and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface& fi, const IfTreeVif& fv,
			 bool is_system_interfaces_reportee);

    /**
     * Check IfTreeAddr4 and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr4&     fa,
			 bool  is_system_interfaces_reportee);

    /**
     * Check IfTreeAddr6 and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr6&     fa,
			 bool  is_system_interfaces_reportee);

    /**
     * Report that updates were completed to IfConfigUpdateReporter.
     */
    void   report_updates_completed(bool is_system_interfaces_reportee);

    /**
     * Check every item within IfTree and report updates to
     * IfConfigUpdateReporter.
     */
    void   report_updates(const IfTree& it, bool is_system_interfaces_reportee);

    void	map_ifindex(uint32_t if_index, const string& ifname);
    void	unmap_ifindex(uint32_t if_index);
    const char*	find_ifname(uint32_t if_index) const;
    uint32_t	find_ifindex(const string& ifname) const;

    const char*	get_insert_ifname(uint32_t if_index);
    uint32_t	get_insert_ifindex(const string& ifname);

private:

    EventLoop&			_eventloop;
    IfConfigUpdateReporterBase&	_ur;
    IfConfigErrorReporterBase&	_er;
    NexthopPortMapper&		_nexthop_port_mapper;

    //
    // A cache of associative array of interface names to interface index.
    // Needed because the RTM_IFANNOUNCE upcall is called after the interface
    // name is deleted from the kernel.
    //
    typedef map<uint32_t, string> IfIndex2NameMap;
    IfIndex2NameMap	_ifnames;

    IfTree		_live_config;	// The IfTree with live config
    IfTree		_pulled_config;	// The IfTree when we pull the config

    list<IfConfigGet*>		_ifc_gets;
    list<IfConfigSet*>		_ifc_sets;
    list<IfConfigObserver*>	_ifc_observers;

    //
    // The primary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigGetDummy		_ifc_get_dummy;
    IfConfigGetIoctl		_ifc_get_ioctl;
    IfConfigGetSysctl		_ifc_get_sysctl;
    IfConfigGetGetifaddrs	_ifc_get_getifaddrs;
    IfConfigGetProcLinux	_ifc_get_proc_linux;
    IfConfigGetNetlink		_ifc_get_netlink;

    //
    // The secondary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is not important.
    //
    IfConfigGetClick		_ifc_get_click;

    //
    // The primary mechanisms to set interface-related information
    // within the underlying system.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigSetDummy		_ifc_set_dummy;
    IfConfigSetIoctl		_ifc_set_ioctl;
    IfConfigSetNetlink		_ifc_set_netlink;

    //
    // The secondary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is not important.
    //
    IfConfigSetClick		_ifc_set_click;

    //
    // The primary mechanisms to observe whether the interface-related
    // information within the underlying system has changed.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigObserverDummy	_ifc_observer_dummy;
    IfConfigObserverRtsock	_ifc_observer_rtsock;
    IfConfigObserverNetlink	_ifc_observer_netlink;

    //
    // Misc other state
    //
    bool	_have_ipv4;
    bool	_have_ipv6;
    bool	_is_dummy;
    bool	_is_running;
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
				  const Update& u,
				  bool  is_system_interfaces_reportee) = 0;

    virtual void vif_update(const string& ifname,
			    const string& vifname,
			    const Update& u,
			    bool  is_system_interfaces_reportee) = 0;

    virtual void vifaddr4_update(const string& ifname,
				 const string& vifname,
				 const IPv4&   addr,
				 const Update& u,
				 bool  is_system_interfaces_reportee) = 0;

    virtual void vifaddr6_update(const string& ifname,
				 const string& vifname,
				 const IPv6&   addr,
				 const Update& u,
				 bool  is_system_interfaces_reportee) = 0;

    virtual void updates_completed(bool is_system_interfaces_reportee) = 0;
};

/**
 * @short A class to replicate update notifications to multiple reporters.
 */
class IfConfigUpdateReplicator : public IfConfigUpdateReporterBase {
public:
    typedef IfConfigUpdateReporterBase::Update Update;

public:
    virtual ~IfConfigUpdateReplicator();

    /**
     * Add a reporter instance to update notification list.
     *
     * @return true on success, false if object already receiving updates or
     * could not be added.
     */
    bool add_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Remove a reporter instance from update notification list.
     *
     * @return true on success, false if instance is not on the
     * receiving updates list.
     */
    bool remove_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Forward interface update notification to reporter instances on
     * update notification list.
     */
    void interface_update(const string& ifname,
			  const Update& u,
			  bool  is_system_interfaces_reportee);

    /**
     * Forward virtual interface update notification to reporter
     * instances on update notification list.
     */
    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& u,
		    bool is_system_interfaces_reportee);

    /**
     * Forward virtual interface address update notification to
     * reporter instances on update notification list.
     */
    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& u,
			 bool is_system_interfaces_reportee);

    /**
     * Forward virtual interface address update notification to
     * reporter instances on update notification list.
     */
    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6&   addr,
			 const Update& u,
			 bool is_system_interfaces_reportee);

    /**
     * Forward notification that updates were completed to
     * reporter instances on update notification list.
     */
    void updates_completed(bool is_system_interfaces_reportee);

protected:
    list<IfConfigUpdateReporterBase*> _reporters;
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

    virtual void config_error(const string& error_msg) = 0;

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

    void config_error(const string& error_msg);

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
