// vim:set sts=4 ts=8:

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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "policy_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "filter_manager.hh"


FilterManager::FilterManager(const CodeMap& imp, 
			     const CodeMap& sm, 
			     const CodeMap& exp,
			     const SetMap& sets, 
			     const TagMap& tagmap, 
			     XrlStdRouter& rtr, 
			     ProcessWatch& pw) :

	_import(imp), _sourcematch(sm), _export(exp), 
	_sets(sets), _tagmap(tagmap), 
	_eventloop(rtr.eventloop()),
	_push_timeout(2000),
	_process_watch(pw), 
	_policy_backend(&rtr), 
	_rib(&rtr),
	_rib_name("rib") // FIXME: rib name hardcoded
{
}

void 
FilterManager::update_filter(const Code::Target& t) {
    switch(t.filter) {
	case filter::IMPORT:
	    update_import_filter(t.protocol);
	    break;

	case filter::EXPORT_SOURCEMATCH:
	    update_sourcematch_filter(t.protocol);
	    break;

	case filter::EXPORT:
	    update_export_filter(t.protocol);
	    break;
    }
}

void 
FilterManager::update_import_filter(const string& protocol) {
    update_queue(protocol,_import,_import_queue);
}
    
void 
FilterManager::update_sourcematch_filter(const string& protocol) {
    update_queue(protocol,_sourcematch,_sourcematch_queue);
}

void 
FilterManager::update_export_filter(const string& protocol) {
    update_queue(protocol,_export,_export_queue);
}

void 
FilterManager::update_tagmap(const string& protocol) {
    TagMap::const_iterator i = _tagmap.find(protocol);

    // no tags for this protocol, no update needed.
    if(i  == _tagmap.end())
	return;

    const TagSet* ts = (*i).second;

    // convert tags to xrlatomlist
    XrlAtomList al;

    for(TagSet::iterator iter = ts->begin();
	iter != ts->end(); ++iter) {
   
        al.append(XrlAtom(*iter));	    
    }

    debug_msg("[POLICY] Updating tagmap proto: %s, tags: %s\n",
	      protocol.c_str(),al.str().c_str());

    // send out update
    _rib.send_insert_policy_redist_tags(_rib_name.c_str(),protocol,al,
	        callback(this,&FilterManager::policy_backend_cb));
}

void 
FilterManager::policy_backend_cb(const XrlError& e) {
    if(e != XrlError::OKAY())
	throw FMException(e.str()); // XXX: what else can we do ?
}

void 
FilterManager::flush_export_queue() {
    
    debug_msg("[POLICY] Flushing export filter queue...\n");

    // commit all updates on export queue
    for(ConfQueue::iterator i = _export_queue.begin();
	i != _export_queue.end(); ++i) {

        const string& protocol = (*i).first;
        const string& conf = (*i).second;

	debug_msg("[POLICY] Protocol: %s, Conf:\n%s\nEnd...\n",
		  protocol.c_str(),conf.c_str());

	// if configuration is empty, reset the filter
        if(!conf.length())
	   _policy_backend.send_reset(protocol.c_str(),filter::EXPORT,
		callback(this,&FilterManager::policy_backend_cb));
	// else configure it	
	else
	    _policy_backend.send_configure(protocol.c_str(),filter::EXPORT,
		conf,callback(this,&FilterManager::policy_backend_cb));

	// export filters may change tagmap
	update_tagmap(protocol);

	// we need to push routes for this protocol [export filter changed].
	_push_queue.insert(protocol);
    }

    _export_queue.clear();
}

void 
FilterManager::flush_queue(ConfQueue& queue, filter::Filter f) {

    debug_msg("[POLICY] Flushing %s queue...\n",
	      filter::filter2str(f).c_str());

    // flush all updates on queue
    for(ConfQueue::iterator i = queue.begin(); i != queue.end(); ++i) {
	const string& protocol = (*i).first;
	const string& conf = (*i).second;

	debug_msg("[POLICY] Protocol: %s, Conf:\n%s\nEnd...\n",
		  protocol.c_str(),conf.c_str());

	// if conf is empty, reset filter.
	if(!conf.length())
	    _policy_backend.send_reset(protocol.c_str(),f,
		callback(this,&FilterManager::policy_backend_cb));
	// else configure filter normally.	
	else
	    _policy_backend.send_configure(protocol.c_str(),f, conf,
		callback(this,&FilterManager::policy_backend_cb));

	// need to push routes for protocol [filters changed].
	_push_queue.insert(protocol);

	// tagmap is updated on export filter flush [even if we may deal with
	// source match filters here... there is no need to worry about tags].
    }

    queue.clear();
}

void
FilterManager::push_routes_now() {
    for(set<string>::iterator i = _push_queue.begin();
	i != _push_queue.end(); ++i) {
	
	const string& proto = *i;

	debug_msg("[POLICY] Pushing routes for %s\n",
		  proto.c_str());

	_policy_backend.send_push_routes(proto.c_str(),
		callback(this,&FilterManager::policy_backend_cb));
    }

    _push_queue.clear();
}

void 
FilterManager::flush_updates_now() {
    // flush all queues
    flush_export_queue();
    flush_queue(_sourcematch_queue,filter::EXPORT_SOURCEMATCH);
    flush_queue(_import_queue,filter::IMPORT);

    // push routes [may get overwritten, its ok for now].
    _push_timer = _eventloop.new_oneoff_after_ms(_push_timeout,
			callback(this,&FilterManager::push_routes_now));
}

void 
FilterManager::flush_updates(uint32_t msec) {
    // delayed flush
    _flush_timer = _eventloop.new_oneoff_after_ms(msec,
			callback(this,&FilterManager::flush_updates_now));
}

void 
FilterManager::birth(const string& protocol) {
    debug_msg("[POLICY] Protocol born: %s\n",protocol.c_str());

    // resend configuration to new born process.
    update_export_filter(protocol);
    update_sourcematch_filter(protocol);
    update_import_filter(protocol);

    
    // FIXME: need a mechanism to make routes from RIB reach the new born
    // process. Consider if source match filter was setup before the export
    // filter was alive... the routes will be sitting in the RIB, and never go
    // to the process.
    
    // This is a HACK [as this problem was discovered quite late]. So it looks
    // ugly on purpose.
    CodeMap::const_iterator cmi = _export.find(protocol);
    if(cmi == _export.end())
	return;

    const Code* export_code = (*cmi).second;

    for(set<string>::iterator i = export_code->_source_protos.begin();
	i != export_code->_source_protos.end(); ++i) {

	const string& push_proto = *i;
	
	if(push_proto == protocol)
	    continue;
	
	if(!_process_watch.alive(push_proto))
	    continue;

	// LUCKY!!!!
	if(_push_queue.find(protocol) != _push_queue.end())
	    continue;

	XLOG_WARNING("XXX HACK: PUSHING ROUTES OF %s FOR %s",
		     push_proto.c_str(),protocol.c_str());
	
	_push_queue.insert(push_proto);
    }

    // EOH [end of hack]
   
    flush_updates_now();

    // XXX: protocol was just born, so no need to push routes for itself... we
    // may remove the pushes from the queue since they are delayed.
    _push_queue.erase(protocol);
}

void 
FilterManager::death(const string& protocol) {
    // do not send any updates to dead process.
    delete_queue_protocol(_export_queue,protocol);
    delete_queue_protocol(_sourcematch_queue,protocol);
    delete_queue_protocol(_import_queue,protocol);
    _push_queue.erase(protocol);

    // XXX: might want to delete policytags in rib... but the tagmap in rib is
    // quite unflexible now.

    debug_msg("[POLICY] Protocol death: %s\n",protocol.c_str());
}

void 
FilterManager::delete_queue_protocol(ConfQueue& queue, 
				     const string& protocol) {
    ConfQueue::iterator i = queue.find(protocol);

    if(i == queue.end())
	return;

    queue.erase(i);	    
}

void 
FilterManager::update_queue(const string& protocol, 
			    const CodeMap& cm,
			    ConfQueue& queue) {

    // if a process is dead, erase it from the queue if it is there, and then do
    // nothing.
    bool alive = _process_watch.alive(protocol);
    if(!alive) {
	debug_msg("[POLICY] clearing update queue for dead protocol: %s\n",
		  protocol.c_str());

        ConfQueue::iterator i = queue.find(protocol);

        if(i != queue.end())
	   queue.erase(i);

        return;
    }

    // check if there is any code present for this protocol, if not reset
    // filter.
    CodeMap::const_iterator i = cm.find(protocol);
    if(i == cm.end()) {
        // reset filter
        queue[protocol] = "";
        return;
    }

    // get the code
    Code* code = (*i).second;

    string conf = code->_code;

    set<string>& sets = code->_sets;

    // expand the sets
    for(set<string>::iterator iter = sets.begin();
        iter != sets.end(); ++iter) {

	conf += "SET " + *iter + " \"";
    
        const Element& e = _sets.getSet(*iter);

        conf += e.str();

        conf += "\"\n";
    }

    // send it the complete configuration [code + sets]
    queue[protocol] = conf;
}
