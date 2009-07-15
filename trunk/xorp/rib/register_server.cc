// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "register_server.hh"


NotifyQueue::NotifyQueue(const string& module_name)
    : _module_name(module_name),
      _active(false),
      _response_sender(NULL)
{
}

void
NotifyQueue::add_entry(NotifyQueueEntry* e) 
{
    _queue.push_back(e);
}

void
NotifyQueue::send_next() 
{
    XrlCompleteCB cb = callback(this, &NotifyQueue::xrl_done);

    _queue.front()->send(_response_sender, _module_name, cb);
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
    //
    // This isn't really the best way to do this, because when the
    // queue is active it won't force transaction batching, but it's
    // better than nothing.
    //
    if (_active) {
	debug_msg("queue is already active\n");
	return;
    }
    _active = true;
    send_next();
}

void
NotifyQueue::xrl_done(const XrlError& e) 
{
    debug_msg("NQ: xrl_done\n");
    if (e == XrlError::OKAY()) {
	if (!_queue.empty() && _active)
	    send_next();
    } else {
	XLOG_ERROR("Failed to send registration update to RIB client");
    }
}

template <>
void
NotifyQueueChangedEntry<IPv4>::send(ResponseSender* response_sender,
				    const string& module_name,
				    NotifyQueue::XrlCompleteCB& cb) 
{
    response_sender->send_route_info_changed4(module_name.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), _nexthop,
					      _metric, _admin_distance,
					      _protocol_origin.c_str(), cb);
}

template <>
void
NotifyQueueChangedEntry<IPv6>::send(ResponseSender* response_sender,
				    const string& module_name,
				    NotifyQueue::XrlCompleteCB& cb) 
{
    response_sender->send_route_info_changed6(module_name.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), _nexthop,
					      _metric, _admin_distance,
					      _protocol_origin.c_str(), cb);
}

template <>
void
NotifyQueueInvalidateEntry<IPv4>::send(ResponseSender* response_sender,
				       const string& module_name,
				       NotifyQueue::XrlCompleteCB& cb) 
{
    debug_msg("Sending route_info_invalid4\n");
    response_sender->send_route_info_invalid4(module_name.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), cb);
}

template <>
void
NotifyQueueInvalidateEntry<IPv6>::send(ResponseSender* response_sender,
				       const string& module_name,
				       NotifyQueue::XrlCompleteCB& cb) 
{
    response_sender->send_route_info_invalid6(module_name.c_str(),
					      _net.masked_addr(),
					      _net.prefix_len(), cb);
}


RegisterServer::RegisterServer(XrlRouter* xrl_router)
    : _response_sender(xrl_router)
{
}

void
RegisterServer::add_entry_to_queue(const string& module_name,
				   NotifyQueueEntry* e) 
{
    debug_msg("REGSERV: add_entry_to_queue\n");
    NotifyQueue* queue;
    map<string, NotifyQueue* >::iterator qmi;
    bool new_queue;
    
    qmi = _queuemap.find(module_name);
    if (qmi == _queuemap.end()) {
	_queuemap[module_name] = new NotifyQueue(module_name);
	queue = _queuemap[module_name];
	new_queue = true;
    } else {
	new_queue = false;
	queue = qmi->second;
    }
    queue->add_entry(e);
}

void
RegisterServer::send_route_changed(const string& module_name,
				   const IPv4Net& net,
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
    add_entry_to_queue(module_name,
		       reinterpret_cast<NotifyQueueEntry *>(q_entry));
}

void
RegisterServer::send_invalidate(const string& module_name,
				const IPv4Net& net,
				bool multicast) 
{
    NotifyQueueInvalidateEntry<IPv4>* q_entry;
    q_entry = new NotifyQueueInvalidateEntry<IPv4>(net, multicast);
    add_entry_to_queue(module_name,
		       reinterpret_cast<NotifyQueueEntry *>(q_entry));
}

void
RegisterServer::send_route_changed(const string& module_name,
				   const IPv6Net& net,
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
    add_entry_to_queue(module_name,
		       reinterpret_cast<NotifyQueueEntry* >(q_entry));
}

void
RegisterServer::send_invalidate(const string& module_name,
				const IPv6Net& net,
				bool multicast) 
{
    NotifyQueueInvalidateEntry<IPv6>* q_entry;
    q_entry = new NotifyQueueInvalidateEntry<IPv6>(net, multicast);
    add_entry_to_queue(module_name,
		       reinterpret_cast<NotifyQueueEntry *>(q_entry));
}

void
RegisterServer::flush() 
{
    debug_msg("REGSERV: flush\n");
    map<string, NotifyQueue* >::iterator iter;
    for (iter = _queuemap.begin(); iter != _queuemap.end(); ++iter) {
	iter->second->flush(&_response_sender);
    }
}
