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

// $XORP: xorp/fea/fticonfig.hh,v 1.11 2003/10/25 08:13:06 pavlin Exp $

#ifndef	__FEA_FTICONFIG_HH__
#define __FEA_FTICONFIG_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/trie.hh"

#include "fte.hh"
#include "fticonfig_entry_get.hh"
#include "fticonfig_entry_set.hh"
#include "fticonfig_entry_observer.hh"
#include "fticonfig_table_get.hh"
#include "fticonfig_table_set.hh"
#include "fticonfig_table_observer.hh"

class EventLoop;
class FtiConfigEntryGet;
class FtiConfigEntrySet;
class FtiConfigEntryObserver;
class FtiConfigTableGet;
class FtiConfigTableSet;
class FtiConfigTableObserver;

typedef Trie<IPv4, Fte4> Trie4;
typedef Trie<IPv6, Fte6> Trie6;

/**
 * @short Forwarding Table Interface.
 *
 * Abstract class.
 */
class FtiConfig {
public:

    /**
     * Constructor.
     * 
     * @param eventloop the event loop.
     */
    FtiConfig(EventLoop& eventloop);

    /**
     * Virtual destructor (in case this class is used as base class).
     */
    virtual ~FtiConfig();

    EventLoop& eventloop() { return _eventloop; }

    int register_ftic_entry_get(FtiConfigEntryGet *ftic_entry_get);
    int register_ftic_entry_set(FtiConfigEntrySet *ftic_entry_set);
    int register_ftic_entry_observer(FtiConfigEntryObserver *ftic_entry_observer);
    int register_ftic_table_get(FtiConfigTableGet *ftic_table_get);
    int register_ftic_table_set(FtiConfigTableSet *ftic_table_set);
    int register_ftic_table_observer(FtiConfigTableObserver *ftic_table_observer);
    
    FtiConfigEntryGet&		ftic_entry_get() { return *_ftic_entry_get; }
    FtiConfigEntrySet&		ftic_entry_set() { return *_ftic_entry_set; }
    FtiConfigEntryObserver&	ftic_entry_observer() { return *_ftic_entry_observer; }
    FtiConfigTableGet&		ftic_table_get() { return *_ftic_table_get; }
    FtiConfigTableSet&		ftic_table_set() { return *_ftic_table_set; }
    FtiConfigTableObserver&	ftic_table_observer() { return *_ftic_table_observer; }

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
     * Start a configuration interval. All modifications must be
     * within a marked "configuration" interval.
     *
     * @return true if configuration successfully started.
     */
    bool start_configuration();
    
    /**
     * End of configuration interval.
     *
     * @return true configuration success pushed down into forwarding table.
     */
    bool end_configuration();
    
    /**
     * Add a single routing entry. Must be within a configuration interval.
     *
     * @param fte the entry to add.
     *
     * @return true on success, otherwise false.
     */
    virtual bool add_entry4(const Fte4& fte);

    /**
     * Delete a single routing entry. Must be with a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_entry4(const Fte4& fte);

    /**
     * Set the unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool set_table4(const list<Fte4>& fte_list);

    /**
     * Delete all entries in the routing table. Must be within a
     * configuration interval.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_all_entries4();

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry4(const IPv4Net& dst, Fte4& fte);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table4(list<Fte4>& fte_list);

    /**
     * Add a single routing entry. Must be within a configuration interval.
     *
     * @param fte the entry to add.
     *
     * @return true on success, otherwise false.
     */
    virtual bool add_entry6(const Fte6& fte);

    /**
     * Set the unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool set_table6(const list<Fte6>& fte_list);

    /**
     * Delete a single routing entry. Must be within a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_entry6(const Fte6& fte);

    /**
     * Delete all entries in the routing table. Must be within a
     * configuration interval.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_all_entries6();

    /**
     * Lookup a route.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup entry.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_entry6(const IPv6Net& dst, Fte6& fte);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table6(list<Fte6>& fte_list);

    /**
     * Test if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool have_ipv4() const;

    /**
     * Test if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool have_ipv6() const;

    /**
     * Test whether the IPv4 unicast forwarding engine is enabled or disabled
     * to forward packets.
     * 
     * @param ret_value if true on return, then the IPv4 unicast forwarding
     * is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unicast_forwarding_enabled4(bool& ret_value, string& error_msg) const;

    /**
     * Test whether the IPv6 unicast forwarding engine is enabled or disabled
     * to forward packets.
     * 
     * @param ret_value if true on return, then the IPv6 unicast forwarding
     * is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unicast_forwarding_enabled6(bool& ret_value, string& error_msg) const;
    
    /**
     * Test whether the acceptance of IPv6 Router Advertisement messages is
     * enabled or disabled.
     * 
     * @param ret_value if true on return, then the acceptance of IPv6 Router
     * Advertisement messages is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int accept_rtadv_enabled6(bool& ret_value, string& error_msg) const;
    
    /**
     * Set the IPv4 unicast forwarding engine to enable or disable forwarding
     * of packets.
     * 
     * @param v if true, then enable IPv4 unicast forwarding, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_enabled4(bool v, string& error_msg);

    /**
     * Set the IPv6 unicast forwarding engine to enable or disable forwarding
     * of packets.
     * 
     * @param v if true, then enable IPv6 unicast forwarding, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_enabled6(bool v, string& error_msg);
    
    /**
     * Enable or disable the acceptance of IPv6 Router Advertisement messages
     * from other routers. It should be enabled for hosts, and disabled for
     * routers.
     * 
     * @param v if true, then enable the acceptance of IPv6 Router
     * Advertisement messages, otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_accept_rtadv_enabled6(bool v, string& error_msg);
    
    /**
     * Get the IPv4 Trie (used for testing purpose).
     * 
     * @return the IPv4 Trie.
     */
    Trie4& trie4() { return _trie4; }

    /**
     * Get the IPv6 Trie (used for testing purpose).
     * 
     * @return the IPv6 Trie.
     */
    Trie6& trie6() { return _trie6; }
    
protected:
    Trie4	_trie4;		// IPv4 trie (used for testing purpose)
    Trie6	_trie6;		// IPv6 trie (used for testing purpose)
    
private:
    EventLoop&	_eventloop;

    FtiConfigEntryGet		*_ftic_entry_get;
    FtiConfigEntrySet		*_ftic_entry_set;
    FtiConfigEntryObserver	*_ftic_entry_observer;
    FtiConfigTableGet		*_ftic_table_get;
    FtiConfigTableSet		*_ftic_table_set;
    FtiConfigTableObserver	*_ftic_table_observer;
    
    //
    // The mechanisms to get single-entry information from the unicast
    // forwarding table.
    // Ordering is important: the last that is supported is the one to use.
    //
    FtiConfigEntryGetDummy	_ftic_entry_get_dummy;
    FtiConfigEntryGetRtsock	_ftic_entry_get_rtsock;
    FtiConfigEntryGetNetlink	_ftic_entry_get_netlink;
    
    //
    // The mechanisms to set single-entry information into the unicast
    // forwarding table.
    // Ordering is important: the last that is supported is the one to use.
    //
    FtiConfigEntrySetDummy	_ftic_entry_set_dummy;
    FtiConfigEntrySetRtsock	_ftic_entry_set_rtsock;
    FtiConfigEntrySetNetlink	_ftic_entry_set_netlink;
    
    //
    // The mechanisms to observe single-entry information change about the
    // unicast forwarding table.
    // E.g., if the forwarding table has changed, then the information
    // received by the observer would specify the particular entry that
    // has changed.
    // Ordering is important: the last that is supported is the one to use.
    //
    FtiConfigEntryObserverDummy	 _ftic_entry_observer_dummy;
    FtiConfigEntryObserverRtsock _ftic_entry_observer_rtsock;
    FtiConfigEntryObserverNetlink _ftic_entry_observer_netlink;

    //
    // The mechanisms to get the whole table information from the unicast
    // forwarding table.
    // Ordering is important: the last that is supported is the one to use.
    //
    FtiConfigTableGetDummy	_ftic_table_get_dummy;
    FtiConfigTableGetSysctl	_ftic_table_get_sysctl;
    FtiConfigTableGetNetlink	_ftic_table_get_netlink;
    
    //
    // The mechanisms to set the whole table information into the unicast
    // forwarding table.
    // Ordering is important: the last that is supported is the one to use.
    //
    FtiConfigTableSetDummy	_ftic_table_set_dummy;
    FtiConfigTableSetRtsock	_ftic_table_set_rtsock;
    FtiConfigTableSetNetlink	_ftic_table_set_netlink;
    
    //
    // The mechanisms to observe the whole-table information change about the
    // unicast forwarding table.
    // E.g., if the forwarding table has changed, then the information
    // received by the observer would NOT specify the particular entry that
    // has changed.
    // Ordering is important: the last that is supported is the one to use.
    //
    FtiConfigTableObserverDummy	 _ftic_table_observer_dummy;
    FtiConfigTableObserverRtsock _ftic_table_observer_rtsock;
    FtiConfigTableObserverNetlink _ftic_table_observer_netlink;
    
    //
    // Original state from the underlying system before the FEA was started
    //
    bool _unicast_forwarding_enabled4;
    bool _unicast_forwarding_enabled6;
    bool _accept_rtadv_enabled6;

    //
    // Misc other state
    //
    bool	_is_dummy;
};

#endif	// __FEA_FTICONFIG_HH__
