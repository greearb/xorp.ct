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

#ident "$XORP: xorp/rtrmgr/xorp_client.cc,v 1.8 2003/04/22 23:43:02 mjh Exp $"

//#define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "xorp_client.hh"
#include "unexpanded_xrl.hh"
#include "module_manager.hh"
#include "template_commands.hh"

XorpBatch::XorpBatch(XorpClient& xclient, uint tid)
    : _xclient(xclient)
{
    _tid = tid;
}

EventLoop&
XorpBatch::eventloop() const 
{
    return _xclient.eventloop();
}

XorpClient&
XorpBatch::client() const
{
    return _xclient;
}

int
XorpBatch::start(CommitCallback ending_cb) {
    debug_msg("Transaction Start: %u Xrls in this transaction\n",
	   (uint32_t)_batch_items.size());
    _list_complete_callback = ending_cb;
    if (_batch_items.empty()) {
	_xclient.remove_transaction(_tid);
	if (!_list_complete_callback.is_empty())
	    _list_complete_callback->dispatch(XORP_OK, "");
	delete this;
	return XORP_OK;
    }
    string errmsg;
    try {
	if (_batch_items.front()->execute(_xclient, this, errmsg) 
	    == XORP_ERROR) {
	    abort_transaction(errmsg);
	    return XORP_ERROR;
	}
    } catch (UnexpandedVariable& uvar) {
	//clean up, then re-throw
	abort_transaction(uvar.str());
	throw uvar;
    }
    return XORP_OK;
}


int
XorpBatch::add_xrl(const UnexpandedXrl&   xrl,
		   XrlRouter::XrlCallback cb,
		   bool			  do_exec,
		   uint32_t		  retries,
		   uint32_t		  retry_ms)
{
    debug_msg("XorpBatch::add_xrl\n");
    XorpBatchXrlItem *batch_item;
    batch_item = new XorpBatchXrlItem(xrl, cb, do_exec, retries, retry_ms);
    _batch_items.push_back(batch_item);
    debug_msg("XorpBatch::add_xrl done\n");
    return XORP_OK;
}

int
XorpBatch::add_module_start(ModuleManager& mmgr, Module* module,
			    XCCommandCallback cb, 
			    bool do_exec) {
    debug_msg("XorpBatch::add_module_start\n");
    XorpBatchModuleItem *batch_item;
    batch_item = new XorpBatchModuleItem(mmgr, module, /*start*/true, 
					 cb, do_exec);
    _batch_items.push_back(batch_item);
    return XORP_OK;
}



void
XorpBatch::batch_item_done(XorpBatchItem* batch_item, bool success,
			   const string& errmsg) {
    assert(batch_item == _batch_items.front());
    if (success) {
	//the action succeeded
	delete batch_item;
	_batch_items.pop_front();

	if (_batch_items.empty()) {
	    _xclient.remove_transaction(_tid);
	    if (!_list_complete_callback.is_empty())
		_list_complete_callback->dispatch(XORP_OK, "");
	    delete this;
	    return;
	}

	//Call the next Xrl
	string errmsg;
	if (_batch_items.front()->execute(_xclient, this, errmsg) == XORP_ERROR)
	    abort_transaction(errmsg);
    } else {
	abort_transaction(errmsg);
    }
}

void XorpBatch::abort_transaction(const string& errmsg) {
    //something went wrong - terminate the whole transaction set
    while (!_batch_items.empty()) {
	delete _batch_items.front();
	_batch_items.pop_front();
    }
    _xclient.remove_transaction(_tid);
    if (!_list_complete_callback.is_empty())
	_list_complete_callback->dispatch(XORP_ERROR, errmsg);
    delete this;
}

XorpBatchItem::XorpBatchItem(XrlRouter::XrlCallback cb,
			     bool do_exec) 
    : _callback(cb), _do_exec(do_exec) 
{
}

XorpBatchXrlItem::XorpBatchXrlItem(const UnexpandedXrl&		 uxrl,
				   const XrlRouter::XrlCallback& cb,
				   bool				 do_exec,
				   uint32_t			 retries,
				   uint32_t 			 retry_ms) 
    : XorpBatchItem(cb, do_exec), _unexpanded_xrl(uxrl),
      _retries(retries), _retry_ms(retry_ms)
{
}

int
XorpBatchXrlItem::execute(XorpClient& xclient, 
			  XorpBatch *batch,
			  string &errmsg)
{
    _batch = batch;
    Xrl *xrl = _unexpanded_xrl.xrl();
    if (xrl == NULL) {
	errmsg = "Failed to expand XRL " + _unexpanded_xrl.str();
	return XORP_ERROR;
    }

    string xrl_return_spec = _unexpanded_xrl.xrl_return_spec();
    return xclient.
	send_now(*xrl, callback(this,
				&XorpBatchXrlItem::response_callback),
		 xrl_return_spec, 
		 _do_exec);
}

void
XorpBatchXrlItem::retry_execution()
{
    string tmp_error;
    debug_msg("Retrying xrl send\n");
    if (execute(_batch->client(), _batch, tmp_error) != XORP_OK) {
	abort();
    }
}

void
XorpBatchXrlItem::response_callback(const XrlError& err, 
				    XrlArgs* xrlargs) {

    if (err != XrlError::OKAY() && _retries > 0) {
	_retry_timer =
	    _batch->eventloop().new_oneoff_after_ms(_retry_ms,
		callback(this, &XorpBatchXrlItem::retry_execution));
	_retries--;
	debug_msg("Attempting resend (retries left %d)", _retries);
	return;
    }

    if (!_callback.is_empty())
	_callback->dispatch(err, xrlargs);
    bool success = true;
    string errmsg;
    if (err != XrlError::OKAY()) {
	success = false;
	errmsg = err.str();
    }
    _batch->batch_item_done(this, success, errmsg);
}

XorpBatchModuleItem::XorpBatchModuleItem(ModuleManager& mmgr, Module* module,
					 bool start,
					 XCCommandCallback cb, 
					 bool do_exec) 
    :XorpBatchItem(cb, do_exec), _mmgr(mmgr), _module(module), 
    _start(start)
{
    debug_msg("new XorpBatchModuleItem %s\n", _module->str().c_str());
}

int 
XorpBatchModuleItem::execute(XorpClient& /*xclient*/, XorpBatch *batch, 
			     string& errmsg) {
    debug_msg("XorpBatchModuleItem::execute (start %d)", _start);
    _batch = batch;
    if (_start) {
	debug_msg("XorpBatchModuleItem::execute %s\n", _module->str().c_str());
	if (_module->run(_do_exec) == XORP_OK) {
	    XorpCallback0<void>::RefPtr cb;
	    cb = callback(this, &XorpBatchModuleItem::response_callback,
			  true, string(""));
	    _startup_timer 
		= _mmgr.eventloop().new_oneoff_after_ms(2000, cb);
	    return XORP_OK;
	} else {
	    //we failed - trigger the callback immediately
	    errmsg = "Execution failed\n";
	    response_callback(false, errmsg);
	    return XORP_ERROR;
	}
    } else {
	//stop
	//XXX
	abort();
    }
    return XORP_ERROR;
}

void 
XorpBatchModuleItem::response_callback(bool success, string errmsg) {
    debug_msg("XorpBatchModuleItem::response_callback %s\n", 
	      _module->str().c_str());
    if (_start) {
	_module->module_run_done(success);
	if (!_callback.is_empty())
	    _callback->dispatch(XrlError::OKAY(), 0);
    } else {
	//XXX
	abort();
    }
    _batch->batch_item_done(this, success, errmsg);
}

/***********************************************************************/
/* XorpClient                                                          */
/***********************************************************************/

XorpClient::XorpClient(EventLoop& eventloop, XrlRouter& xrlrouter) 
    : _eventloop(eventloop), _xrlrouter(xrlrouter)
{
}

int
XorpClient::send_now(const Xrl &xrl, XrlRouter::XrlCallback cb, 
		     const string& xrl_return_spec,
		     bool do_exec) {
    if (do_exec) {
	debug_msg("send_sync before sending\n");
	_xrlrouter.send(xrl, cb);
	debug_msg("send_sync after sending\n");
    } else {
	debug_msg("send_sync before sending\n");
	debug_msg("DUMMY SEND: immediate callback dispatch\n");
	if (!cb.is_empty()) {
	    XrlArgs args = fake_return_args(xrl_return_spec);
	    cb->dispatch(XrlError::OKAY(), &args);
	}
	debug_msg("send_sync after sending\n");
    }
    return XORP_OK;
}

/* fake_return_args is purely needed for testing, so that XRLs that
   should return a value don't completely fail */
XrlArgs 
XorpClient::fake_return_args(const string& xrl_return_spec) {
    if (xrl_return_spec.empty()) {
	return XrlArgs();
    }
    debug_msg("fake_return_args %s\n", xrl_return_spec.c_str());
    list <string> args;
    string s = xrl_return_spec;
    while (1) {
	string::size_type start = s.find("&");
	if (start == string::npos) {
	    args.push_back(s);
	    break;
	}
	args.push_back(s.substr(0, start));
	s = s.substr(start+1, s.size()-(start+1));
    }
    XrlArgs xargs;
    list <string>::const_iterator i;
    for(i = args.begin(); i!= args.end(); i++) {
	debug_msg("ARG: %s\n", i->c_str());
	string::size_type eq = i->find("=");
	XrlAtom atom;
	if (eq == string::npos) {
	    debug_msg("ARG2: %s\n", i->c_str());
	    atom = XrlAtom(i->c_str());
	} else {
	    debug_msg("ARG2: >%s<\n", i->substr(0, eq).c_str());
	    atom = XrlAtom(i->substr(0, eq).c_str());
	}
	switch (atom.type()) {
	case xrlatom_no_type:
	    abort();
	case xrlatom_int32:
	    xargs.add(XrlAtom(atom.name().c_str(), (int32_t)0) );
	    break;
	case xrlatom_uint32:
	    xargs.add(XrlAtom(atom.name().c_str(), (uint32_t)0) );
	    break;
	case xrlatom_ipv4:
	    xargs.add(XrlAtom(atom.name().c_str(), IPv4("0.0.0.0")) );
	    break;
	case xrlatom_ipv4net:
	    xargs.add(XrlAtom(atom.name().c_str(), IPv4Net("0.0.0.0/0")) );
	    break;
	case xrlatom_text:
	    xargs.add(XrlAtom(atom.name().c_str(), string("")) );
	    break;
	case xrlatom_ipv6:
	case xrlatom_ipv6net:
	case xrlatom_mac:
	case xrlatom_list:
	case xrlatom_boolean:
	case xrlatom_binary:
	    //XXX TBD
	    abort();
	    break;
	}
    }
    debug_msg("ARGS: %s\n", xargs.str().c_str());
    return xargs;
}

int
XorpClient::send_xrl(uint		  tid,
		     const UnexpandedXrl& xrl, 
		     XCCommandCallback	  cb,
		     bool 		  do_exec,
		     uint32_t		  retries,
		     uint32_t		  retry_ms)
{
    XorpBatch* transaction;
    map<uint, XorpBatch*>::iterator i;
    i = _transactions.find(tid);
    assert(i != _transactions.end());

    transaction = i->second;
    return transaction->add_xrl(xrl, cb, do_exec, retries, retry_ms);
}

int XorpClient::start_module(uint tid, 
			     ModuleManager& module_manager, 
			     Module *module,
			     XCCommandCallback cb,
			     bool do_exec) {
    XorpBatch* transaction;
    map<uint, XorpBatch*>::iterator i;
    i = _transactions.find(tid);
    assert(i != _transactions.end());

    transaction = i->second;
    return transaction->add_module_start(module_manager, module,
					 cb, do_exec);
}

uint 
XorpClient::begin_transaction() {
    _max_tid++; 
    _transactions[_max_tid] = new XorpBatch(*this, _max_tid);
    return _max_tid;
}

int
XorpClient::end_transaction(uint tid, 
			    XorpBatch::CommitCallback ending_cb) {
    XorpBatch* transaction;
    map<uint, XorpBatch*>::iterator i;
    i = _transactions.find(tid);
    assert(i != _transactions.end());
    transaction = i->second;
    return transaction->start(ending_cb);
}

void
XorpClient::remove_transaction(uint tid) {
    map<uint, XorpBatch*>::iterator i;
    i = _transactions.find(tid);
    if (i != _transactions.end())
	_transactions.erase(i);
}
