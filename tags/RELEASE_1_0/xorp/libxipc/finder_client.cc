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

#ident "$XORP: xorp/libxipc/finder_client.cc,v 1.23 2004/05/24 01:22:32 hodson Exp $"

#include <functional>
#include <algorithm>

#include "finder_module.h"

#include "libxorp/callback.hh"
#include "libxorp/debug.h"

#include "xrl_error.hh"
#include "xrl_dispatcher.hh"

#include "finder_client.hh"
#include "finder_client_observer.hh"
#include "finder_xif.hh"
#include "finder_tcp_messenger.hh"

static const char* finder = "finder";

///////////////////////////////////////////////////////////////////////////////
//
// Tracing related
//

static class TraceFinderClient
{
public:
    TraceFinderClient() : _do_trace(getenv("FINDERCLIENTTRACE") != 0)
    {
    }
    inline bool on() const			{ return _do_trace; }
    inline operator bool()			{ return _do_trace; }
    inline void set_context(const string& s)	{ _context = s; }
    inline const string& context() const	{ return _context; }
protected:
    bool _do_trace;
    string _context;
} finder_tracer;

#define finder_trace(x...)						      \
do {									      \
    if (finder_tracer.on()) {						      \
	string r = c_format(x);						      \
	XLOG_INFO(c_format("%s\n", r.c_str()).c_str());			      \
    }									      \
} while (0)

#define finder_trace_init(x...)					      \
do {									      \
    if (finder_tracer.on()) finder_tracer.set_context(c_format(x));	      \
} while (0)

#define finder_trace_result(x...)					      \
do {									      \
    if (finder_tracer.on()) {						      \
	string r = c_format(x);						      \
	XLOG_INFO(c_format("%s -> %s\n",				      \
		  finder_tracer.context().c_str(), r.c_str()).c_str());	      \
    }									      \
} while(0)


///////////////////////////////////////////////////////////////////////////////
//
// FinderDBEntry
//

FinderDBEntry::FinderDBEntry(const string& key)
    : _key(key)
{
}

FinderDBEntry::FinderDBEntry(const string& key, const string& value)
    : _key(key)
{
    _values.push_back(value);
}

inline void
FinderDBEntry::clear()
{
    _values.erase(_values.begin(), _values.end());
}


///////////////////////////////////////////////////////////////////////////////
//
// Asynchronous FinderClient Operations and supporting classes
//

/**
 * Base class for operations to be executed by FinderClient.
 */
class FinderClientOp
{
public:
    FinderClientOp(FinderClient& fc) : _fc(fc) {}
    virtual ~FinderClientOp() {}

    /*
     * If execute is called with m == 0, then Messenger is down.  Execute
     * should take suitable action.
     */
    virtual void execute(FinderMessengerBase* m) = 0;

    inline FinderClient& client() { return _fc; }

protected:
    FinderClient& _fc;
};

/**
 * Base class for operations FinderClient only need execute once,
 * eg resolutions.
 */
class FinderClientOneOffOp : public FinderClientOp
{
public:
    FinderClientOneOffOp(FinderClient& fc) : FinderClientOp(fc) {}

    virtual void force_failure(const XrlError&) = 0;
};

/**
 * Base class for operations FinderClient may have to repeat, eg
 * register target, register xrl, etc.  Each repeat operation is associated
 * with an enumerated target.
 */
class FinderClientRepeatOp : public FinderClientOp
{
public:
    FinderClientRepeatOp(FinderClient& fc, uint32_t target_id)
	: FinderClientOp(fc), _tid(target_id) {}

    inline uint32_t target_id() const { return _tid; }

private:
    uint32_t _tid;
};

/**
 * Class that handles resolutions for FinderClient, and puts results
 * into FinderClient's resolved table and notifies the client.
 */
class FinderClientQuery : public FinderClientOneOffOp
{
public:
    typedef FinderClient::QueryCallback QueryCallback;
    typedef FinderClient::ResolvedTable ResolvedTable;

public:
    FinderClientQuery(FinderClient&	   fc,
		      const string&	   key,
		      ResolvedTable&	   rt,
		      const QueryCallback& qcb)
	: FinderClientOneOffOp(fc), _key(key), _rt(rt), _qcb(qcb)
    {
	finder_trace("Constructing ClientQuery \"%s\"", _key.c_str());
    }

    ~FinderClientQuery()
    {
	finder_trace("Destructing ClientQuery \"%s\"", _key.c_str());
    }

    void
    execute(FinderMessengerBase* m)
    {
	finder_trace_init("executing ClientQuery \"%s\"", _key.c_str());
	XrlFinderV0p2Client cl(m);
	if (!cl.send_resolve_xrl(finder, _key,
		callback(this, &FinderClientQuery::query_callback))) {
	    _qcb->dispatch(XrlError::RESOLVE_FAILED(), 0);
	    XLOG_ERROR("Failed on send_resolve_xrl");
	    finder_trace_result("failed (send)");
	    client().notify_failed(this);
	    return;
	}
	finder_trace_result("okay");
    }

    void
    query_callback(const XrlError& e, const XrlAtomList* al)
    {
	finder_trace_init("ClientQuery callback \"%s\"", _key.c_str());
	if (e != XrlError::OKAY()) {
	    finder_trace_result("failed on \"%s\" (%s) -> RESOLVE_FAILED",
				_key.c_str(), e.str().c_str());
	    _qcb->dispatch(XrlError::RESOLVE_FAILED(), 0);

	    if (e == XrlError::COMMAND_FAILED()) {
		// Probably something that doesn't resolve, not a catastrophic
		// transport failure
		client().notify_done(this);
	    } else {
		// Probably a catastrophic transport failure
		client().notify_failed(this);
	    }
	    return;
	}

	pair<ResolvedTable::iterator, bool> result =
	    _rt.insert(std::make_pair(_key, FinderDBEntry(_key)));

	if (result.second == false && result.first == _rt.end()) {
	    // Out of memory (?)
	    XLOG_ERROR("Failed to add entry for %s to resolve table.\n",
		       _key.c_str());
	    XrlError e(RESOLVE_FAILED, "Out of memory");
	    _qcb->dispatch(e, 0); // :-(
	    finder_trace_result("failed (unknown)");
	    client().notify_failed(this);
	    return;
	}

	// NB if result.second == false and result.first != _rt.end we've
	// made back to back requests for something that is not in cache
	// and both results have come in.  WE take the more recent result
	// as the one to use.

	ResolvedTable::iterator& rt_entry = result.first;
	if (false == rt_entry->second.values().empty()) {
	    rt_entry->second.clear();
	}

	for (size_t i = 0; i < al->size(); i++) {
	    try {
		debug_msg("Adding resolved \"%s\"\n",
			  al->get(i).text().c_str());
		rt_entry->second.values().push_back(al->get(i).text());
	    } catch (const XrlAtom::NoData&) {
		_rt.erase(rt_entry);
		_qcb->dispatch(XrlError::RESOLVE_FAILED(), 0);
		finder_trace_result("failed (corrupt response)");
		client().notify_done(this);
		return;
	    } catch (const XrlAtom::WrongType&) {
		_rt.erase(rt_entry);
		_qcb->dispatch(XrlError::RESOLVE_FAILED(), 0);
		finder_trace_result("failed (corrupt response)");
		client().notify_done(this);
		return;
	    }
	}
	_qcb->dispatch(e, &rt_entry->second);
	finder_trace_result("okay");
	client().notify_done(this);
    }

    void force_failure(const XrlError& e)
    {
	finder_trace("ClientQuery force_failure \"%s\"", _key.c_str());
	_qcb->dispatch(e, 0);
    }

protected:
    string	   _key;
    ResolvedTable& _rt;
    QueryCallback  _qcb;
};

/**
 * Class that handles the forwarding of Xrl's targetted at the finder.
 */
class FinderForwardedXrl : public FinderClientOneOffOp
{
public:
    typedef XrlPFSender::SendCallback XrlCallback;
public:
    FinderForwardedXrl(FinderClient&		fc,
		       const Xrl&		xrl,
		       const XrlCallback&	xcb)
	: FinderClientOneOffOp(fc), _xrl(xrl), _xcb(xcb)
    {
	finder_trace("Constructing ForwardedXrl \"%s\"", _xrl.str().c_str());
    }

    ~FinderForwardedXrl()
    {
	finder_trace("Destructing ForwardedXrl \"%s\"", _xrl.str().c_str());
    }

    void
    execute(FinderMessengerBase* m)
    {
	finder_trace_init("executing ForwardedXrl \"%s\"", _xrl.str().c_str());
	if (m->send(_xrl,
		    callback(this, &FinderForwardedXrl::execute_callback))) {
	    finder_trace_result("okay");
	    return;
	}
	_xcb->dispatch(XrlError::SEND_FAILED(), 0);
	XLOG_ERROR("Failed to send forwarded Xrl to Finder.");
	client().notify_failed(this);
	finder_trace_result("failed (send)");
	return;
    }

    void
    execute_callback(const XrlError& e, XrlArgs* args)
    {
	finder_trace_init("ForwardedXrl callback \"%s\"", _xrl.str().c_str());
	_xcb->dispatch(e, args);
	client().notify_done(this);
	finder_trace_result("%s", e.str().c_str());
    }

    void force_failure(const XrlError& e)
    {
	finder_trace("ForwardedXrl force_failure \"%s\"", _xrl.str().c_str());
	_xcb->dispatch(e, 0);
    }


protected:
    Xrl		_xrl;
    XrlCallback	_xcb;
};

/**
 * Class to register client with Finder.
 */
class FinderClientRegisterTarget : public FinderClientRepeatOp
{
public:
    FinderClientRegisterTarget(FinderClient&	fc,
			       uint32_t		target_id,
			       const string&	instance_name,
			       const string&	class_name)
	: FinderClientRepeatOp(fc, target_id),
	  _iname(instance_name), _cname(class_name), _cookie("")
    {
    }

    void execute(FinderMessengerBase* m)
    {
	FinderTcpMessenger* ftm = dynamic_cast<FinderTcpMessenger*>(m);
	XLOG_ASSERT(ftm != 0);
	XrlFinderV0p2Client cl(m);
	if (!cl.send_register_finder_client(finder, _iname, _cname,
					    false, _cookie,
		callback(this, &FinderClientRegisterTarget::reg_callback))) {
	    XLOG_ERROR("Failed on send_register_xrl");
	    client().notify_failed(this);
	} else {
	    debug_msg("Sent register_finder_client(\"%s\", \"%s\")\n",
		      _iname.c_str(), _cname.c_str());
	}
    }

    void reg_callback(const XrlError& e, const string* out_cookie)
    {
	if (e == XrlError::OKAY()) {
	    _cookie = *out_cookie;
	    debug_msg("register_finder_client gave cookie = %s\n",
		      _cookie.c_str());
	    client().notify_done(this);
	    return;
	}

	XLOG_ERROR("Failed to register client named %s of class %s: \"%s\"\n",
		   _iname.c_str(), _cname.c_str(), e.str().c_str());
	client().notify_failed(this);
    }

protected:
    string _iname;
    string _cname;
    string _cookie;
};

/**
 * Class to register an Xrl with the Finder.
 */
class FinderClientRegisterXrl : public FinderClientRepeatOp
{
public:
    typedef FinderClient::LocalResolvedTable LocalResolvedTable;

public:
    FinderClientRegisterXrl(FinderClient&	fc,
			    LocalResolvedTable&	lrt,
			    uint32_t		target_id,
			    const string&	xrl,
			    const string&	pf_name,
			    const string&	pf_args)
	: FinderClientRepeatOp(fc, target_id), _lrt(lrt),
	  _xrl(xrl), _pf(pf_name), _pf_args(pf_args)
    {}

    void execute(FinderMessengerBase* m)
    {
	XrlFinderV0p2Client cl(m);
	debug_msg("Sending add_xrl(\"%s\", \"%s\", \"%s\")\n",
		  _xrl.c_str(), _pf.c_str(), _pf_args.c_str());
	if (!cl.send_add_xrl(finder, _xrl, _pf, _pf_args,
		callback(this, &FinderClientRegisterXrl::reg_callback))) {
	    XLOG_ERROR("Failed on send_add_xrl");
	    client().notify_failed(this);
	}
    }

    void
    reg_callback(const XrlError& e, const string* result)
    {
	if (XrlError::OKAY() == e) {
	    Xrl x(_xrl.c_str());
	    debug_msg("Registering command mapping \"%s\" -> \"%s\"\n",
		      result->c_str(), x.command().c_str());
	    _lrt[*result] = x.command();
	    client().notify_done(this);
	} else {
	   XLOG_ERROR("Failed to register xrl %s: %s\n",
		      _xrl.c_str(), e.str().c_str());
	   client().notify_failed(this);
	}
    }

protected:
    LocalResolvedTable& _lrt;
    string _xrl;
    string _pf;
    string _pf_args;
};

class FinderClientEnableXrls : public FinderClientRepeatOp
{
public:
    FinderClientEnableXrls(FinderClient& fc,
			   uint32_t	 target_id,
			   const string& instance_name,
			   bool		 en,
			   bool&	 update_var,
			   FinderClientObserver*& fco)
	: FinderClientRepeatOp(fc, target_id), _iname(instance_name),
	  _en(en), _update_var(update_var), _fco(fco)
    {
	finder_trace("Constructing EnableXrls \"%s\"", _iname.c_str());
    }

    ~FinderClientEnableXrls()
    {
	finder_trace("Destructing EnableXrls \"%s\"", _iname.c_str());
    }

    void execute(FinderMessengerBase* m)
    {
	finder_trace_init("execute EnableXrls \"%s\"", _iname.c_str());
	FinderTcpMessenger* ftm = dynamic_cast<FinderTcpMessenger*>(m);
	XLOG_ASSERT(ftm != 0);
	XrlFinderV0p2Client cl(m);
	if (!cl.send_set_finder_client_enabled(finder, _iname, _en,
		callback(this, &FinderClientEnableXrls::en_callback))) {
	    XLOG_ERROR("Failed on send_set_finder_client_enabled");
	    finder_trace_result("failed (send)");
	    client().notify_failed(this);
	}
	finder_trace_result("okay");
    }

    void en_callback(const XrlError& e)
    {
	finder_trace_init("EnableXrls callback \"%s\"", _iname.c_str());

	if (e == XrlError::OKAY()) {
	    _update_var = _en;
	    finder_trace_result("okay");
	    client().notify_done(this);
	    if (true == _en && _fco) {
		_fco->finder_ready_event(_iname);
	    }
	    return;
	}

	XLOG_ERROR("Failed to enable client \"%s\": %s\n",
		   _iname.c_str(), e.str().c_str());
	finder_trace_result("failed");
	client().notify_failed(this);
    }

protected:
    string		   _iname;
    bool		   _en;
    bool&		   _update_var;
    FinderClientObserver*& _fco;
};

///////////////////////////////////////////////////////////////////////////////
//
// FinderClient::InstanceInfo
//

class FinderClient::InstanceInfo
{
public:
    InstanceInfo(const string&	      instance_name,
		 const string&	      class_name,
		 const XrlDispatcher* dispatcher)
	: _ins_name(instance_name),
	  _cls_name(class_name),
	  _dispatcher(dispatcher),
	  _id(_s_id++)
    {}
    inline const string& instance_name() const		{ return _ins_name; }
    inline const string& class_name() const		{ return _cls_name; }
    inline const XrlDispatcher* dispatcher() const	{ return _dispatcher; }
    inline const uint32_t id() const			{ return _id; }

private:
    string		 _ins_name;
    string		 _cls_name;
    const XrlDispatcher* _dispatcher;
    uint32_t		 _id;

    static uint32_t	 _s_id;
};

uint32_t FinderClient::InstanceInfo::_s_id = 0;


///////////////////////////////////////////////////////////////////////////////
//
// FinderClient methods
//

FinderClient::FinderClient()
    : _messenger(0), _pending_result(false), _xrls_registered(false),
      _observer(0)
{
    finder_trace("Constructing FinderClient (%p)", this);
}

FinderClient::~FinderClient()
{
    finder_trace("Destructing FinderClient (%p)", this);
    if (_messenger) {
	_messenger->unhook_manager();
	delete _messenger;
    }
}

FinderClient::InstanceList::iterator
FinderClient::find_instance(const string& instance_name)
{
    InstanceList::iterator i = _ids.begin();
    for (; i != _ids.end(); i++) {
	if (i->instance_name() == instance_name)
	    break;
    }
    return i;
}

FinderClient::InstanceList::const_iterator
FinderClient::find_instance(const string& instance_name) const
{
    InstanceList::const_iterator i = _ids.begin();
    for (; i != _ids.end(); i++) {
	if (i->instance_name() == instance_name)
	    break;
    }
    return i;
}

bool
FinderClient::register_xrl_target(const string&		instance_name,
				  const string&		class_name,
				  const XrlDispatcher*	dispatcher)
{
    if (instance_name.empty() || class_name.empty())
	return false;

    const InstanceList::iterator existing = find_instance(instance_name);
    if (existing != _ids.end()) {
	if (existing->class_name() != class_name) {
	    XLOG_FATAL("Re-registering instance with different class (now %s"
		       " was %s)",
		       class_name.c_str(),
		       existing->class_name().c_str());
	}
	XLOG_WARNING("Attempting to re-register xrl target \"%s\"",
		     instance_name.c_str());
	return true;
    }

    _ids.push_back(InstanceInfo(instance_name, class_name, dispatcher));

    // We should check whether item exists on queue already.
    Operation op(new FinderClientRegisterTarget(*this, _ids.back().id(),
						instance_name, class_name));
    _todo_list.push_back(op);
    crank();
    return true;
}

bool
FinderClient::register_xrl(const string& instance_name,
			   const string& xrl,
			   const string& pf_name,
			   const string& pf_args)
{
    InstanceList::const_iterator ii = find_instance(instance_name);
    if (ii == _ids.end()) {
	return false;
    }

    Operation op(new FinderClientRegisterXrl(*this, _lrt, ii->id(), xrl,
					     pf_name, pf_args));
    _todo_list.push_back(op);
    crank();
    return true;
}

bool
FinderClient::enable_xrls(const string& instance_name)
{
    InstanceList::const_iterator ii = find_instance(instance_name);
    if (ii == _ids.end()) {
	return false;
    }

    Operation op(new FinderClientEnableXrls(*this, ii->id(),
					    ii->instance_name(), true,
					    _xrls_registered,
					    _observer));
    _todo_list.push_back(op);
    crank();
    return true;
}

void
FinderClient::query(const string&	 key,
		    const QueryCallback& qcb)
{
    Operation op(new FinderClientQuery(*this, key, _rt, qcb));
    _todo_list.push_back(op);
    crank();
}

const FinderDBEntry*
FinderClient::query_cache(const string& key) const
{
    ResolvedTable::const_iterator i = _rt.find(key);
    return (_rt.end() == i) ? 0 : &i->second;
}

void
FinderClient::uncache_result(const FinderDBEntry* fdbe)
{
    if (fdbe) {
	ResolvedTable::iterator i = _rt.find(fdbe->key());
	if (i == _rt.end())
	    return;
	_rt.erase(i);
    }
}

bool
FinderClient::query_self(const string& incoming_xrl_cmd,
			 string&       local_xrl) const
{
    LocalResolvedTable::const_iterator i = _lrt.find(incoming_xrl_cmd);
    if (_lrt.end() == i)
	return false;
    local_xrl = i->second;
    return true;
}

bool
FinderClient::forward_finder_xrl(const Xrl&			  xrl,
				 const XrlPFSender::SendCallback& scb)
{
    Operation op(new FinderForwardedXrl(*this, xrl, scb));
    _todo_list.push_back(op);
    crank();
    return true;
}

FinderMessengerBase*
FinderClient::messenger()
{
    return _messenger;
}

void
FinderClient::crank()
{
    if (_pending_result)
	return;

    if (0 == _messenger)
	return;

    if (_todo_list.empty())
	return;

    _pending_result = true;
    _todo_list.front()->execute(_messenger);
}

void
FinderClient::notify_done(const FinderClientOp* op)
{
    // These assertions probably want revising.
    XLOG_ASSERT(_todo_list.empty() == false);
    XLOG_ASSERT(_todo_list.front().get() == op);
    XLOG_ASSERT(_pending_result == true);

    // If item is repeatable, ie we'd need to repeat it after a failure
    // put in the done list so we can recover it later.
    if (dynamic_cast<const FinderClientRepeatOp*>(op))
	_done_list.push_back(_todo_list.front());
    _todo_list.erase(_todo_list.begin());
    _pending_result = false;
    crank();
}

void
FinderClient::notify_failed(const FinderClientOp* op)
{
    debug_msg("Client op failed, restarting...\n");

    // These assertions probably want revising.
    XLOG_ASSERT(_todo_list.empty() == false);
    XLOG_ASSERT(_todo_list.front().get() == op);
    XLOG_ASSERT(_pending_result == true);

    if (dynamic_cast<const FinderClientRepeatOp*>(op)) {
	_done_list.push_back(_todo_list.front());
    }
    _todo_list.erase(_todo_list.begin());

    // Walk todo list and clear up one off requests.
    OperationQueue::iterator i = _todo_list.begin();
    while (i != _todo_list.end()) {
	OperationQueue::iterator curr = i++;
	FinderClientOneOffOp* o =
	    dynamic_cast<FinderClientOneOffOp*>(curr->get());
	if (o) {
	    o->force_failure(XrlError::NO_FINDER());
	}
	_todo_list.erase(curr);
    }

    _pending_result = false;

    // Assume messenger is dead. Note well: deletion of messenger triggers a message to
    // be bubbled up announcing its death.
    FinderMessengerBase* m = _messenger;
    _messenger = 0;
    delete m;
}

void
FinderClient::prepare_for_restart()
{
    // Take all of operations on the done list and put at front of the todo
    // list.
    size_t old_size = _todo_list.size();
    _todo_list.splice(_todo_list.begin(), _done_list);
    XLOG_ASSERT(_todo_list.size() >= old_size);

    // Clear resolved table
    _rt.clear();

    // Clear local resolutions
    _lrt.clear();

    _pending_result = false;
    _xrls_registered = false;
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderMessengerManager Interface
//

void
FinderClient::messenger_birth_event(FinderMessengerBase* m)
{
    finder_trace("messenger %p birth\n", m);
    XLOG_ASSERT(0 == _messenger);
    prepare_for_restart();
    _messenger = m;
    if (_observer)
	_observer->finder_connect_event();
    crank();
}

void
FinderClient::messenger_death_event(FinderMessengerBase* m)
{
    finder_trace("messenger %p death\n", m);
    XLOG_ASSERT(m == _messenger);
    _messenger = 0;
    if (_observer)
	_observer->finder_disconnect_event();
}

void
FinderClient::messenger_active_event(FinderMessengerBase* m)
{
    // nothing to do - only have one messenger
    debug_msg("messenger %p active\n", m);
    XLOG_ASSERT(m == _messenger);
}

void
FinderClient::messenger_inactive_event(FinderMessengerBase* m)
{
    // nothing to do - only have one messenger
    debug_msg("messenger %p inactive\n", m);
    XLOG_ASSERT(m == _messenger);
}

void
FinderClient::messenger_stopped_event(FinderMessengerBase* m)
{
    debug_msg("messenger %p stopped (closing connecting)\n", m);
    XLOG_ASSERT(m == _messenger);
    //    delete _messenger;
    //    _messenger = 0;
}

bool
FinderClient::manages(const FinderMessengerBase* m) const
{
    return m == _messenger;
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderClientXrlCommandInterface
//

void
FinderClient::uncache_xrl(const string& xrl)
{
    finder_trace_init("Request to uncache xrl \"%s\"\n", xrl.c_str());

    ResolvedTable::iterator i = _rt.find(xrl);
    if (_rt.end() != i) {
	finder_trace_result("Request fulfilled.\n");
	_rt.erase(i);
	return;
    }
    finder_trace_result("Request not fulfilled - not in cache.\n");
}

void
FinderClient::uncache_xrls_from_target(const string& target)
{
    finder_trace_init("uncache_xrls_from_target");
    size_t n = 0;
    ResolvedTable::iterator i = _rt.begin();
    while (_rt.end() != i) {
	if (Xrl(i->first.c_str()).target() == target) {
	    _rt.erase(i++);
	    n++;
	} else {
	    ++i;
	}
    }
    finder_trace_result("Uncached %u Xrls relating to target \"%s\"\n",
			(uint32_t)n, target.c_str());
}

XrlCmdError
FinderClient::dispatch_tunneled_xrl(const string& xrl_str)
{
    finder_trace_init("dispatch_tunneled_xrl(\"%s\")", xrl_str.c_str());
    Xrl xrl;
    try {
	xrl = Xrl(xrl_str.c_str());
	InstanceList::iterator i = find_instance(xrl.target());
	if (i == _ids.end()) {
	    finder_trace_result("target not found");
	    return XrlCmdError::COMMAND_FAILED("target not found");
	}

	XrlArgs ret_vals;
	i->dispatcher()->dispatch_xrl(xrl.command(),
				      xrl.args(), ret_vals);
	finder_trace_result("success");
	return XrlCmdError::OKAY();
    } catch (InvalidString&) {
	return XrlCmdError::COMMAND_FAILED("Bad Xrl string");
    }
}

bool
FinderClient::attach_observer(FinderClientObserver* fco)
{
    if (0 == _observer && 0 != fco) {
	_observer = fco;
	if (connected())
	    _observer->finder_connect_event();
	return true;
    }
    return false;
}

bool
FinderClient::detach_observer(FinderClientObserver* fco)
{
    if (fco == _observer) {
	_observer = 0;
	return true;
    }
    return false;
}
