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

// $XORP: xorp/libxipc/finder_client.hh,v 1.11 2003/06/19 00:44:42 hodson Exp $

#ifndef __LIBXIPC_FINDER_CLIENT_HH__
#define __LIBXIPC_FINDER_CLIENT_HH__

#include <list>
#include <map>
#include <string>

#include "finder_client_base.hh"
#include "finder_messenger.hh"

#include "xrl_pf.hh"

class FinderClientOp;
class FinderClientObserver;

/**
 * A one-to-many container used by the FinderClient to store
 * unresolved-to-resolved Xrl mappings.
 */
struct FinderDBEntry
{
    FinderDBEntry(const string& key);
    FinderDBEntry(const string& key, const string& value);

    inline const string& key() const		{ return _key; }
    inline const list<string>& values() const	{ return _values; }
    inline list<string>& values()		{ return _values; }
    inline void clear();

protected:
    string	 _key;
    list<string> _values;
};

/**
 * Interface class for FinderClient Xrl requests.
 *
 * The methods in this interface are implemented by the FinderClient
 * to handle its XRL requests.  It exists as a separate class to restrict
 * the operations that the XRL interface can invoke.
 */
class FinderClientXrlCommandInterface
{
public:
    virtual void uncache_xrl(const string& xrl) = 0;
    virtual void uncache_xrls_from_target(const string& target) = 0;
    virtual XrlCmdError dispatch_tunneled_xrl(const string& xrl) = 0;
};

/**
 * @short Class that represents clients of the Finder.
 *
 * The FinderClient class performs communication processing with the
 * Finder on behalf of XORP processes.  It handles XRL registration
 * and resolution requests.
 */
class FinderClient :
    public FinderMessengerManager, public FinderClientXrlCommandInterface
{
public:
    typedef
    XorpCallback2<void, const XrlError&, const FinderDBEntry*>::RefPtr
    QueryCallback;
    typedef ref_ptr<FinderClientOp> Operation;
    typedef list<Operation> OperationQueue;

    class InstanceInfo;

    typedef map<string, FinderDBEntry>	ResolvedTable;
    typedef map<string, string>		LocalResolvedTable;
    typedef vector<InstanceInfo>	InstanceList;

public:
    /**
     * Constructor.
     */
    FinderClient();

    /**
     * Destructor.
     */
    virtual ~FinderClient();

    /**
     * Register an Xrl Target with the FinderClient and place request with
     * Finder to perform registration. The request to the Finder is
     * asynchronous and there is a delay between when the request is made
     * when it is satisfied.
     *
     * @param instance_name a unique name to be associated with the Target.
     * @param class_name the class name that the Target is an instance of.
     * @param dispatcher pointer to Xrl dispatcher that can execute the
     * command.
     *
     * @return true on success, false if @ref instance_name or @ref
     * class_name are empty.
     */
    bool register_xrl_target(const string&	  instance_name,
			     const string&	  class_name,
			     const XrlDispatcher* dispatcher);

    /**
     * Unregister Xrl Target with FinderClient and place a request with
     * the Finder to remove registration.   The request to the Finder is
     * asynchronous and there is a delay between when the request is made
     * when it is satisfied.
     *
     * @param instance_name unique name associated with Xrl Target.
     */
    bool unregister_xrl_target(const string& instance_name);

    /**
     * Register an Xrl with the Finder.
     *
     * @param instance_name unique name associated with Xrl Target and making
     * the registration.
     *
     * @param xrl string representation of the Xrl.
     * @param pf_name protocol family name that implements Xrl.
     * @param pf_args protocol family arguments to locate dispatcher for
     * registered Xrl.
     *
     * @return true if registration request is successfully enqueued, false
     * otherwise.
     */
    bool register_xrl(const string& instance_name,
		      const string& xrl,
		      const string& pf_name,
		      const string& pf_args);

    /**
     * Request Finder advertise Xrls associated with an Xrl Target instance
     * to other XORP Xrl targets.  Until the Finder has satisfied this
     * request the Xrl Target has no visibility in the Xrl universe.
     *
     * @param instance_name unique name associated with Xrl Target to
     * be advertised.
     *
     * @return true on success, false if instance_name has not previously
     * been registered with FinderClient.
     */
    bool enable_xrls(const string& instance_name);

    /**
     * Request resolution of an Xrl.
     *
     * If the Xrl to be resolved in cache exists in the FinderClients
     * cache, the callback provided as a function argument is invoked
     * immediately.  Otherwise the request is forwarded to the finder,
     * the cache updated, and callback dispatched when the Finder
     * answers the request.
     *
     * @param xrl Xrl to be resolved.
     * @param qcb callback to be dispatched when result is availble.
     */
    void query(const string&	    xrl,
	       const QueryCallback& qcb);

    /**
     * Attempt to resolve Xrl from cache.
     *
     * @param xrl Xrl to be resolved.
     *
     * @return pointer to cached entry on success, 0 otherwise.
     */
    const FinderDBEntry* query_cache(const string& xrl) const;

    /**
     * Resolve Xrl that an Xrl Target associated with the FinderClient
     * registered.
     *
     * @param incoming_xrl_command the command component of the Xrl being
     * resolved.
     * @param local_xrl_command the local name of the Xrl command being
     * resolved.
     * @return true and assign value to local_xrl_command on success, false
     * on failure.
     */
    bool query_self(const string& incoming_xrl_command,
		    string& local_xrl_command) const;

    /**
     * Send an Xrl for the Finder to dispatch.  This is the mechanism
     * that allows clients of the Finder to interrogate the Finder through
     * an Xrl interface.
     *
     * @param x Xrl to be dispatched.
     * @param cb callback to be called with dispatch result.
     * @return true on success.
     */
    bool forward_finder_xrl(const Xrl& x, const XrlPFSender::SendCallback& cb);

    /**
     * Accessor for Finder Messenger used by FinderClient instance.
     */
    FinderMessengerBase* messenger();

    /**
     * Get list of operations pending.
     */
    inline OperationQueue& todo_list()		{ return _todo_list; }

    /**
     * Get List of operations done and are repeatable.
     */
    inline OperationQueue& done_list()		{ return _done_list; }

    /**
     * Notify successful completion of an operation on the todo list.
     */
    void   notify_done(const FinderClientOp* completed);

    /**
     * Notify failed completion of an operation on the todo list.
     */
    void   notify_failed(const FinderClientOp* completed);

    /**
     * Get the Xrl Commands implemented by the FinderClient.
     */
    inline XrlCmdMap& commands()		{ return _commands; }

    /**
     * @return true if FinderClient has registered Xrls and is considered
     * operational.
     */
    inline bool ready() const			{ return _xrls_registered; }

    /**
     * @return true if a connection is established with the Finder.
     */
    inline bool connected() const		{ return _messenger != 0; }

    /**
     * Attach a FinderClientObserver instance to receive event notifications.
     *
     * @param o pointer to observer to receive notifications.
     *
     * @return true on success, false if an observer is already present.
     */
    bool attach_observer(FinderClientObserver* o);

    /**
     * Detach the FinderClientObserver instance.
     *
     * @param o pointer to the FinderClientObserver be removed.
     *
     * @return true on success, false if the FinderClientObserver
     * is not the current observer.
     */
    bool detach_observer(FinderClientObserver* o);

protected:
    // FinderMessengerManager interface
    void messenger_birth_event(FinderMessengerBase*);
    void messenger_death_event(FinderMessengerBase*);
    void messenger_active_event(FinderMessengerBase*);
    void messenger_inactive_event(FinderMessengerBase*);
    void messenger_stopped_event(FinderMessengerBase*);
    bool manages(const FinderMessengerBase*) const;

protected:
    // FinderClientXrlCommandInterface
    void uncache_xrl(const string& xrl);
    void uncache_xrls_from_target(const string& target);
    XrlCmdError dispatch_tunneled_xrl(const string& xrl);

protected:
    void crank();
    void prepare_for_restart();

protected:
    inline InstanceList::iterator find_instance(const string& instance);
    inline InstanceList::const_iterator find_instance(const string& instance) const;

protected:
    OperationQueue	 _todo_list;
    OperationQueue	 _done_list;
    ResolvedTable	 _rt;
    LocalResolvedTable	 _lrt;
    InstanceList	 _ids;

    XrlCmdMap		 _commands;

    FinderMessengerBase* _messenger;
    bool		 _pending_result;
    bool		 _xrls_registered;

    FinderClientObserver* _observer;
};

#endif // __LIBXIPC_FINDER_CLIENT_HH__
