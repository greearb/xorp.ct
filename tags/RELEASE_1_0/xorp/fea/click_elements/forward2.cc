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

#ident "$XORP: xorp/fea/click_elements/forward2.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
// ALWAYS INCLUDE click/config.h
#include <click/config.h>
// ALWAYS INCLUDE click/package.hh
#include <click/package.hh>
#include <click/error.hh>
#include <click/confparse.hh>

#include "fea/fti.hh"
#include "forward2.hh"
#include "rtable2.hh"

Forward2::Forward2()
{
    // CONSTRUCTOR MUST MOD_INC_USE_COUNT
    MOD_INC_USE_COUNT;

    route_swap();
    _intransaction = false;
    _transactionID = 0;

    add_input();
}

Forward2::~Forward2()
{
    // DESTRUCTOR MUST MOD_DEC_USE_COUNT
    MOD_DEC_USE_COUNT;
}

int 
Forward2::configure(const Vector<String> &conf, ErrorHandler *errh)
{
    int max_port = -1;

    for(int i = 0; i < conf.size(); i++) {
//         click_chatter((String)conf[i]);

        unsigned int dst, mask;
        unsigned int gw = 0;
        int port;

        Vector<String> words;
        cp_spacevec(conf[i], words);

        /*
        ** The options are:
        ** address/mask   [gw optional] port
        ** address/prefix [gw optional] port
        */
        if(!((2 == words.size()) || (3 == words.size()))) {
            String tmp = conf[i];
            errh->warning("bad argument line %d: %s", i + 1, tmp.cc());
            return -1;
        }

        /*
        ** Pick out the destination
        */
        if(!cp_ip_prefix(words[0], (unsigned char *)&dst, 
                         (unsigned char *)&mask, true, this)) {
            errh->warning("bad argument line %d: %s should be addr/mask",
                          i + 1, words[0].cc());
            return -1;
        }

        /*
        ** If there are three words pick out the optional gateway.
        */
        if((3 == words.size())) {
            if(!cp_ip_address(words[1], (unsigned char *)&gw, this)) {
                errh->warning("bad argument line %d: %s should be addr", 
                              i + 1, words[1].cc());
                return -1;
            }
        }
        
        /*
        ** port
        */
        if(!cp_integer(words.back(), &port)) {
                errh->warning(
                              "bad argument line %d: %s should be port number",
                              i + 1, words.back().cc());
                return -1;
        }

        _rt_current->add(dst, mask, gw, port);
        
        max_port = port > max_port ? port : max_port;
    }

    set_noutputs(max_port + 1);

    return 0;
}

int
Forward2::initialize(ErrorHandler *)
{
//     errh->message("Successfully linked with package!");

    //    Rtable2 rt;

    _rt_current->test();

    return 0;
}

void
Forward2::push(int, Packet *p)
{
    IPV4address a = p->dst_ip_anno().addr();
    IPV4address gw;
    int oport;
    
    if(!_rt_current->lookup(a, gw, oport)) {
        click_chatter("Forward2::push: No route for %s",
                      (const char *)a.string());
        p->kill();
        return;
    }

//     click_chatter("Forward2::push: %s", (const char *)a.string());

    output(oport).push(p);
    return;
}

void
Forward2::add_handlers()
{
//     click_chatter("Forward2::add_handlers()");

    /*
    ** Handlers to communicate with the fea
    */
    add_write_handler("start_transaction", write_handler_start_transaction,
		      (void *)this);
    add_read_handler("start_transaction", read_handler_start_transaction,
		      (void *)this);

    add_write_handler("commit_transaction", write_handler_commit_transaction,
		      (void *)this);
    add_write_handler("abort_transaction", write_handler_abort_transaction,
		      (void *)this);
    add_write_handler("break_transaction", write_handler_break_transaction,
		      (void *)this);
    add_write_handler("delete_all_entries", write_handler_delete_all_entries,
		      (void *)this);
    add_write_handler("delete_entry", write_handler_delete_entry,
		      (void *)this);

    add_write_handler("add_entry", write_handler_add_entry,
		      (void *)this);

    add_read_handler("table", read_handler_table, (void *)this);

    add_write_handler("lookup_route", write_handler_lookup_route,
		      (void *)this);
    add_read_handler("lookup_route", read_handler_lookup_route, (void *)this);
    
    add_write_handler("lookup_entry", write_handler_lookup_entry,
		      (void *)this);
    add_read_handler("lookup_entry", read_handler_lookup_entry,
		     (void *)this);
}

/*
** XXX - This function should be moved into a utility file somewhere.
** Used to split a string on newlines into a vector of strings.
*/
static void split_on_newline(const String &str, Vector<String> &conf)
{
    for(int i = 0, pos = 0, newpos;; i++, pos = newpos + 1) {
// 	click_chatter("%d index %d\n", i, str.find_left('\n', pos));
	if(-1 == (newpos = str.find_left('\n', pos)))
	    break;
	conf.push_back(str.substring(pos, newpos - pos));
// 	click_chatter("conf[%d] = <%s>\n", i, conf[i].cc());
    }
}

/*
** Start a transaction.
** In order the start a transaction write the transaction type into
** the "start_transaction".
** 
** Then read back from the interface to get the transaction ID.
*/
int
Forward2::write_handler_start_transaction(const String &addr, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;

    if(me->_intransaction)
	return errh->error("In transaction");

    me->_transactionID++;
    click_chatter("Setting transaction ID to %d\n", me->_transactionID);
    me->_intransaction = true;

    /*
    ** Take a copy of the current routing table and place it into the
    ** shadow copy.
    */
    me->route_sync();

    return 0;
}

String
Forward2::read_handler_start_transaction(Element *, void *thunk)
{
    Forward2 *me = (Forward2 *)thunk;

    return String(me->_transactionID) + String("\n");
}

/*
** Commit transaction.
** Perform a write with the correct transactionID
*/
int
Forward2::write_handler_commit_transaction(const String &addr, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;
    int tid;

    if(!me->_intransaction)
	return errh->error("Not in transaction");

    if(!cp_integer(cp_uncomment(addr), (int *)&tid))
	return errh->error("transaction ID required <%s>",
			   String(addr).cc());

    if(tid != me->_transactionID)
	return errh->error("transaction ID supplied <%s> does not match <%d>",
			   String(addr).cc(), me->_transactionID);
	
    me->_intransaction = false;

    /*
    ** Make the shadow table that we have been operating on the
    ** current routing table.
    */
    me->route_swap();

    return 0;
}

/*
** Abort transaction.
** Perform a write with the correct transactionID
*/
int
Forward2::write_handler_abort_transaction(const String &addr, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;
    int tid;

    if(!me->_intransaction)
	return errh->error("Not in transaction");

    if(!cp_integer(cp_uncomment(addr), (int *)&tid))
	return errh->error("transaction ID required <%s>",
			   String(addr).cc());

    if(tid != me->_transactionID)
	return errh->error("transaction ID supplied <%s> does not match <%d>",
			   String(addr).cc(), me->_transactionID);
	
    me->_intransaction = false;

    return 0;
}

/*
** Break transaction.
** Perform a write with the correct transactionID
*/
int
Forward2::write_handler_break_transaction(const String &addr, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;
    int tid;

    if(!me->_intransaction)
	return errh->error("Not in transaction");

    if(!cp_integer(cp_uncomment(addr), (int *)&tid))
	return errh->error("transaction ID required <%s>",
			   String(addr).cc());

    if(tid != me->_transactionID)
	return errh->error("transaction ID supplied <%s> does not match <%d>",
			   String(addr).cc(), me->_transactionID);
	
    me->_intransaction = false;

    return 0;
}

/*
** Delete all entries
** Perform a write with the correct transactionID
*/
int
Forward2::write_handler_delete_all_entries(const String &addr, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;
    int tid;

    if(!me->_intransaction)
	return errh->error("Not in transaction");

    if(!cp_integer(cp_uncomment(addr), (int *)&tid))
	return errh->error("transaction ID required <%s>",
			   String(addr).cc());

    if(tid != me->_transactionID)
	return errh->error("transaction ID supplied <%s> does not match <%d>",
			   String(addr).cc(), me->_transactionID);
	
    /*
    ** Zap the shadow routing table entry.
    */
    me->_rt_shadow->zero();

    return 0;
}

/*
** Delete entry
** Perform a write with the correct transactionID.
** Followed by at least one entry to delete.
*/
int
Forward2::write_handler_delete_entry(const String &str, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;
    int tid;

    if(!me->_intransaction)
	return errh->error("Not in transaction");

    Vector<String> conf;
    split_on_newline(str, conf);

    if(conf.size() < 1 || !cp_integer(conf[0], (int *)&tid))
	return errh->error("transaction ID required <%s>",
			   String(str).cc());

    if(tid != me->_transactionID)
	return errh->error("transaction ID supplied <%s> does not match <%d>",
			   String(str).cc(), me->_transactionID);

    /*
    ** Start from 1 as the first line was the TID.
    */
    for(int i = 1; i < conf.size(); i++) {
        click_chatter("Deleting %s\n", conf[i].cc());

        unsigned int dst, mask;

        Vector<String> words;
        cp_spacevec(conf[i], words);

        /*
        ** The options are:
        ** address/mask
        ** address/prefix
        */
	
        /*
        ** Pick out the destination
        */
        if(!cp_ip_prefix(words[0], (unsigned char *)&dst, 
                         (unsigned char *)&mask, true, me)) {
            errh->warning("bad argument line %d: %s should be addr/mask",
                          i + 1, words[0].cc());
            return -1;
        }

	/* XXX
	** This method should return success or failure that needs to
	** be propagated to the caller.
	*/
	me->_rt_shadow->del(dst, mask);
    }

    return 0;
}

/* XXX - A cut and paste of delete entry. Should write a common routing.
** Add entry
** Perform a write with the correct transactionID.
** Followed by at least one entry to delete.
*/
int
Forward2::write_handler_add_entry(const String &str, Element *,
					  void *thunk, ErrorHandler *errh)
{
    Forward2 *me = (Forward2 *)thunk;
    int tid;

    if(!me->_intransaction)
	return errh->error("Not in transaction");

    Vector<String> conf;
    split_on_newline(str, conf);

    if(conf.size() < 1 || !cp_integer(conf[0], (int *)&tid))
	return errh->error("transaction ID required <%s>",
			   String(str).cc());

    if(tid != me->_transactionID)
	return errh->error("transaction ID supplied <%s> does not match <%d>",
			   String(str).cc(), me->_transactionID);

    /*
    ** Start from 1 as the first line was the TID.
    */
    click_chatter("Size %d\n", conf.size());
    for(int i = 1; i < conf.size(); i++) {
        click_chatter("Adding %s\n", conf[i].cc());

        unsigned int dst, mask;
        unsigned int gw = 0;
        int port;

        Vector<String> words;
        cp_spacevec(conf[i], words);

        /*
        ** The options are:
        ** address/mask   [gw optional] port
        ** address/prefix [gw optional] port
        */
        if(!((2 == words.size()) || (3 == words.size()))) {
            String tmp = conf[i];
            errh->warning("bad argument line %d: %s", i + 1, tmp.cc());
            return -1;
        }

        /*
        ** Pick out the destination
        */
        if(!cp_ip_prefix(words[0], (unsigned char *)&dst, 
                         (unsigned char *)&mask, true, me)) {
            errh->warning("bad argument line %d: %s should be addr/mask",
                          i + 1, words[0].cc());
            return -1;
        }

        /*
        ** If there are three words pick out the optional gateway.
        */
        if((3 == words.size())) {
            if(!cp_ip_address(words[1], (unsigned char *)&gw, me)) {
                errh->warning("bad argument line %d: %s should be addr", 
                              i + 1, words[1].cc());
                return -1;
            }
        }

        /*
        ** port
        */
        if(!cp_integer(words.back(), &port)) {
                errh->warning(
                              "bad argument line %d: %s should be port number",
                              i + 1, words.back().cc());
                return -1;
        }

	/* XXX
	** This method should return success or failure that needs to
	** be propagated to the caller.
	*/
	me->_rt_shadow->add(dst, mask, gw, port);
    }

    return 0;
}

/*
** This handler gives the whole routing table. The click_fti can
** serialize it.
**
** start_reading and read_entry should be built on this interface.
*/
String
Forward2::read_handler_table(Element *, void *thunk)
{
    String result;

    Forward2 *me = (Forward2 *)thunk;

    if(false == me->_rt_current->start_table_read())
	return String("Bogey");

    IPV4address dst, mask, gw;
    int index;
    Rtable2::status status;

    while(me->_rt_current->next_table_read(dst, mask, gw, index, status)) {
// 	click_chatter("Forward2::read_handler_table", dst.string().cc());
	result += dst + "/" + String(mask.mask_length()) + "\t" +
	    mask + "\t" +
	    gw +  " \t" + String(index) + "\n";
    }

    return result;
}

int
Forward2::write_handler_lookup_route(const String &addr, Element *,
				     void *thunk, ErrorHandler *errh)
{
    IPV4address a;
    IPV4address dst;
    IPV4address mask;
    IPV4address gw;
    int oport;

    Forward2 *me = (Forward2 *)thunk;
    
    if(!cp_ip_address(cp_uncomment(addr), a.data()))
	return errh->error("bogus address <%s>", String(addr).cc());

    if(!me->_rt_current->lookup(a, dst, mask, gw, oport))
	return errh->error("lookup failed %s", String(addr).cc());

    me->lookup_result = a + "\t" + dst + "/" + String(mask.mask_length()) + 
	"\t" + gw + "\t" + String(oport) + String("\n");

    return 0;
}

String
Forward2::read_handler_lookup_route(Element *, void *thunk)
{
    Forward2 *me = (Forward2 *)thunk;

    return me->lookup_result;
}

int
Forward2::write_handler_lookup_entry(const String &/*addr*/, Element *,
				     void */*thunk*/, ErrorHandler */*errh*/)
{
#if	0
    IPV4address a;
    IPV4address dst;
    IPV4address mask;
    IPV4address gw;
    int oport;

    Forward2 *me = (Forward2 *)thunk;
    
    if(!cp_ip_address(cp_uncomment(addr), a.data()))
	return errh->error("bogus address <%s>", String(addr).cc());

    if(!me->_rt_current->lookup(a, dst, mask, gw, oport))
	return errh->error("lookup failed %s", String(addr).cc());

    me->lookup_result = a + "\t" + dst + "/" + String(mask.mask_length()) + 
	"\t" + gw + "\t" + String(oport) + String("\n");
#endif
    return 0;
}

String
Forward2::read_handler_lookup_entry(Element *, void */*thunk*/)
{
#if	0
    Forward2 *me = (Forward2 *)thunk;

    return me->lookup_result;
#endif
    return String("Not implemented");
}

EXPORT_ELEMENT(Forward2)
ELEMENT_REQUIRES(HoopyFrood)
