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

#ident "$XORP: xorp/rib/register_server.cc,v 1.7 2003/03/16 07:18:57 pavlin Exp $"

//#define DEBUG_LOGGING
#include "rib_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxipc/xrl_router.hh"
#include "register_server.hh"

NotifyQueue::NotifyQueue(const string& modname)
    : _modname(modname), _active(false), _response_sender(NULL)
{
}

void
NotifyQueue::add_entry(NotifyQueueEntry *e) 
{
    _queue.push_back(e);
}


void
NotifyQueue::send_next() 
{
    XrlCompleteCB cb = callback(this, &NotifyQueue::xrl_done);
    _queue.front()->send(_response_sender, _modname, cb);
    _queue.pop_front();
    if (_queue.empty()) {
	_active = false;
	_response_sender = NULL;
    }
}

void
NotifyQueue::flush(ResponseSender* response_sender) 
{
    debug_msg("NQ: flush\n");
    if (_queue.empty()) {
	debug_msg("flush called on empty queue\n");
	return;
    }
    _response_sender = response_sender;
    // this isn't really the best way to do this, because when the
    // queue is active it won't force transaction batching, but it's
    // better than nothing
    if (_active) {
	debug_msg("queue is already active\n");
	return;
    }
    _active = true;
    send_next();
}

void NotifyQueue::xrl_done(const XrlError& e) 
{
    debug_msg("NQ: xrl_done\n");
    if (e == XrlError::OKAY()) {
	if (!_queue.empty() && _active)
	    send_next();
    } else {
	XLOG_ERROR("Failed to send registration update to RIB client\n");
    }
}

void
NotifyQueueChangedEntry<IPv4>::send(ResponseSender* response_sender,
				    const string& modname,
				    NotifyQueue::XrlCompleteCB& cb) 
{
    response_sender->send_route_info_changed4(modname.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), _nexthop,
					      _metric, _admin_distance,
					      _protocol_origin.c_str(), cb);
}

void
NotifyQueueChangedEntry<IPv6>::send(ResponseSender* response_sender,
				    const string& modname,
				    NotifyQueue::XrlCompleteCB& cb) 
{
    response_sender->send_route_info_changed6(modname.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), _nexthop,
					      _metric, _admin_distance,
					      _protocol_origin.c_str(), cb);
}

void
NotifyQueueInvalidateEntry<IPv4>::send(ResponseSender* response_sender,
				       const string& modname,
				       NotifyQueue::XrlCompleteCB& cb) 
{
    printf("Sending route_info_invalid4\n");
    response_sender->send_route_info_invalid4(modname.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), cb);
}

void
NotifyQueueInvalidateEntry<IPv6>::send(ResponseSender* response_sender,
				       const string& modname,
				       NotifyQueue::XrlCompleteCB& cb) 
{
    response_sender->send_route_info_invalid6(modname.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), cb);
}


RegisterServer::RegisterServer(XrlRouter* xrl_router)
    : _response_sender(xrl_router)
{
}

void
RegisterServer::add_entry_to_queue(const string& modname,
				   NotifyQueueEntry *e) 
{
    debug_msg("REGSERV: add_entry_to_queue\n");
    NotifyQueue *queue;
    map <string, NotifyQueue*>::iterator qmi;
    bool new_queue;
    
    qmi = _queuemap.find(modname);
    if (qmi == _queuemap.end()) {
	_queuemap[modname] = new NotifyQueue(modname);
	queue = _queuemap[modname];
	new_queue = true;
    } else {
	new_queue = false;
	queue = qmi->second;
    }
    queue->add_entry(e);
}

void
RegisterServer::send_route_changed(const string& modname,
				   const IPNet<IPv4>& net,
				   const IPv4& nexthop,
				   uint32_t metric,
				   uint32_t admin_distance,
				   const string& protocol_origin,
				   bool multicast) 
{
    NotifyQueueChangedEntry<IPv4>* q_entry;
    q_entry = new NotifyQueueChangedEntry<IPv4>(net, nexthop,
						metric, admin_distance,
						protocol_origin, multicast);
    add_entry_to_queue(modname, (NotifyQueueEntry*)q_entry);
}

void
RegisterServer::send_invalidate(const string& modname,
				const IPNet<IPv4>& net,
				bool multicast) 
{
    NotifyQueueInvalidateEntry<IPv4>* q_entry;
    q_entry = new NotifyQueueInvalidateEntry<IPv4>(net, multicast);
    add_entry_to_queue(modname, (NotifyQueueEntry*)q_entry);
}


void
RegisterServer::send_route_changed(const string& modname,
				   const IPNet<IPv6>& net,
				   const IPv6& nexthop,
				   uint32_t metric,
				   uint32_t admin_distance,
				   const string& protocol_origin,
				   bool multicast) 
{
    NotifyQueueChangedEntry<IPv6>* q_entry;
    q_entry = new NotifyQueueChangedEntry<IPv6>(net, nexthop,
						metric, admin_distance,
						protocol_origin, multicast);
    add_entry_to_queue(modname, (NotifyQueueEntry*)q_entry);
}

void
RegisterServer::send_invalidate(const string& modname,
				const IPNet<IPv6>& net,
				bool multicast) 
{
    NotifyQueueInvalidateEntry<IPv6>* q_entry;
    q_entry = new NotifyQueueInvalidateEntry<IPv6>(net, multicast);
    add_entry_to_queue(modname, (NotifyQueueEntry*)q_entry);
}

void
RegisterServer::flush() 
{
    debug_msg("REGSERV: flush\n");
    map <string, NotifyQueue*>::iterator i;
    for (i = _queuemap.begin(); i != _queuemap.end(); i++) {
	i->second->flush(&_response_sender);
    }
}
