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

// $XORP: xorp/libxipc/finder_client.hh,v 1.6 2003/04/23 20:50:45 hodson Exp $

#ifndef __LIBXIPC_FINDER_NG_CLIENT_HH__
#define __LIBXIPC_FINDER_NG_CLIENT_HH__

#include <list>
#include <map>
#include <string>

#include "finder_client_base.hh"
#include "finder_messenger.hh"

#include "xrl_pf.hh"

class FinderClientOp;

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

class FinderClientXrlCommandInterface
{
public:
    virtual void uncache_xrl(const string& xrl) = 0;
    virtual void uncache_xrls_from_target(const string& target) = 0;
    virtual XrlCmdError dispatch_tunneled_xrl(const string& xrl) = 0;
};

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
    FinderClient();
    virtual ~FinderClient();

    bool register_xrl_target(const string&	  instance_name,
			     const string&	  class_name,
			     const XrlDispatcher* dispatcher);

    bool unregister_xrl_target(const string& instance_name);
    
    bool register_xrl(const string& instance_name,
		      const string& xrl,
		      const string& pf_name,
		      const string& pf_args);

    bool enable_xrls(const string& instance_name);
    
    void query(const string&	    key,
	       const QueryCallback& qcb);

    bool query_self(const string& incoming_xrl_command,
		    string& local_xrl_command) const;

    bool forward_finder_xrl(const Xrl&, const XrlPFSender::SendCallback&);
    
    FinderMessengerBase* messenger();

    inline OperationQueue& todo_list()		{ return _todo_list; }
    inline OperationQueue& done_list()		{ return _done_list; }
    void   notify_done(const FinderClientOp* completed);
    void   notify_failed(const FinderClientOp* completed);

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
    OperationQueue	_todo_list;
    OperationQueue	_done_list;
    ResolvedTable	_rt;
    LocalResolvedTable  _lrt;
    InstanceList	_ids;

    XrlCmdMap		_commands;
    
    FinderMessengerBase* _messenger;
    bool		 _pending_result;
    bool		 _xrls_registered;
};

#endif // __LIBXIPC_FINDER_NG_CLIENT_HH__
