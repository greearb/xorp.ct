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


#include "bgp4_mib_module.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "libxorp/callback.hh"
#include "xorpevents.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_1657_bgppeertable.hh"


// Local datatype
typedef struct 
{
    uint32_t peer_list_token;
    IPv4     peer_local_ip;
    uint32_t peer_local_port;
    IPv4     peer_remote_ip;
    uint32_t peer_remote_port;
    bool     more;
    bool     valid;
} PeerLoopContext, PeerDataContext;

// Local prototypes
void free_context(void *, struct netsnmp_iterator_info_s *);
void get_peer_list_start_done( const XrlError&, const uint32_t*, const bool*, 
    PeerLoopContext*);
void get_peer_list_next_done(const XrlError&, const IPv4*, const uint32_t*,
    const IPv4*, const uint32_t*, const bool*, PeerDataContext*); 
void get_peer_id_done(const XrlError&, const IPv4*, netsnmp_delegated_cache *);
void get_peer_status_done(const XrlError&, const uint32_t*, const uint32_t*, 
    netsnmp_delegated_cache *);
void get_peer_negotiated_version_done(const XrlError&, const int32_t *, 
    netsnmp_delegated_cache *);
void get_peer_as_done(const XrlError&, const uint32_t *, 
    netsnmp_delegated_cache *);
void get_peer_msg_stats_done(const XrlError&, const uint32_t *, 
    const uint32_t *, const uint32_t *, const uint32_t *, const uint32_t *,
    const uint32_t *, netsnmp_delegated_cache *);  
void get_peer_established_stats(const XrlError&, const uint32_t *,
    const uint32_t *, netsnmp_delegated_cache *); 
void get_peer_timer_config_done(const XrlError&, const uint32_t *, 
    const uint32_t *, const uint32_t *,  const uint32_t *,  const uint32_t *,
    const uint32_t *, const uint32_t *, netsnmp_delegated_cache *);  


// Initialize the bgpPeerTable table by defining its contents and how it's structured 
void
initialize_table_bgpPeerTable(void)
{
    static oid bgpPeerTable_oid[] = {1,3,6,1,2,1,15,3};
    netsnmp_table_registration_info *table_info;
    netsnmp_handler_registration *my_handler;
    netsnmp_iterator_info *iinfo;


    /* create the table structure itself */
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
     
    
    my_handler = netsnmp_create_handler_registration("bgpPeerTable",
                                             bgpPeerTable_handler,
                                             bgpPeerTable_oid,
                                             OID_LENGTH(bgpPeerTable_oid),
                                             HANDLER_CAN_RONLY);
            
    if (!my_handler || !table_info || !iinfo)
        return; /* mallocs failed */

    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                                     ASN_IPADDRESS, /*  bgpPeerRemoteAddr */
                                     0);            /*  terminate list    */

    table_info->min_column = 1;
    table_info->max_column = 24;

    /* iterator access routines */
    iinfo->get_first_data_point = bgpPeerTable_get_first_data_point;
    iinfo->get_next_data_point = bgpPeerTable_get_next_data_point;
    iinfo->free_loop_context_at_end = free_context;
    iinfo->free_loop_context = NULL;  // only free at end
    iinfo->free_data_context = free_context; 
    iinfo->table_reginfo = table_info;

    netsnmp_register_table_iterator(my_handler, iinfo);
}

/** Initializes the bgp4_mib_1657_bgppeertable module */
void
init_bgp4_mib_1657_bgppeertable(void)
{

  /* here we initialize all the tables we're planning on supporting */

    DEBUGMSGTL((BgpMib::the_instance().name(), "Initializing\n"));
    initialize_table_bgpPeerTable();
}


// bgpPeerTable_get_first_data_point - initialize the table contexts
//
// This function is called by snmpd before any access to the data in the bgpPeer
// Table.  It initializes the loop context (in this case, the peer list
// identifier returned by the bgp module, aka token).
//
// IMPLEMENTATION NOTE: 
// The table helper functions provided by Net-SNMP (*_get_first_data_point and
// *_get_next_data_point) expect to have data available by the time we return, 
// this is why we are forced to access Xorp in a syncronous way (that is,
// waiting inside the function until we get a response).  This is not great, 
// since while we wait here, we cannot respond to other SNMP events. We do this
// only on these two functions which are called once per table and once per row
// respectively.  
// For bgpPeerTable_handler we do use asyncronous calls.

netsnmp_variable_list *
bgpPeerTable_get_first_data_point(void **my_loop_context,
				  void **my_data_context,
                                  netsnmp_variable_list *put_index_data,
				  netsnmp_iterator_info *mydata)
{
    PeerLoopContext* loop_context;
    BgpMib& bgp_mib = BgpMib::the_instance();
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance();

    DEBUGMSGTL((BgpMib::the_instance().name(), "get_first_data_point\n"));

    /* allocate loop context for this table */
    loop_context = SNMP_MALLOC_TYPEDEF(PeerLoopContext);
    if (NULL == loop_context) return NULL;

    loop_context->valid = false;
    bgp_mib.send_get_peer_list_start("bgp",
				     callback(get_peer_list_start_done,
					      loop_context));
    bool timeout = false;
    XorpTimer t = eventloop.set_flag_after_ms(1000, &timeout);
    while (!timeout && !loop_context->valid) {
	DEBUGMSGTL((BgpMib::the_instance().name(),"waiting for peer "
	    "list...\n"));
	eventloop.run(); //see note in function header if this shocks you...
    }

    if (timeout) {
	DEBUGMSGTL((BgpMib::the_instance().name(), "timeout while reading "
		     "table...\n"));
	return NULL; // timeout
    }

    (*my_loop_context) = (void *) loop_context; 

    // we are now ready to go fetch the index values for the first row 
    return bgpPeerTable_get_next_data_point(my_loop_context, my_data_context,
					    put_index_data, mydata);
}

// bgpPeerTable_get_next_data_point - update table contexts after each row
// 
// this function updates the data context (in this case the local and
// remote addresses and ports that we will need to pass as arguments to the xrls
// when accessing table data) for each row
// 
// IMPLEMENTATION NOTE: we use the loop context structure to store row context
// to avoid having to malloc the context structure for every row.  

netsnmp_variable_list *
bgpPeerTable_get_next_data_point(void **my_loop_context, void **my_data_context,
				 netsnmp_variable_list *put_index_data,
				 netsnmp_iterator_info *)
{
    BgpMib& bgp_mib = BgpMib::the_instance();
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance();
    PeerLoopContext* loop_context = (PeerLoopContext*) (*my_loop_context);
    PeerDataContext* data_context;  

    
    // allocate data context for this row
    data_context = SNMP_MALLOC_TYPEDEF(PeerDataContext);
    if (NULL == data_context) return NULL;

    DEBUGMSGTL((BgpMib::the_instance().name(), "get_next_data_point\n"));

    if (!loop_context->more) return NULL;

    data_context->valid = false; 
    bgp_mib.send_get_peer_list_next("bgp", loop_context->peer_list_token,
				    callback(get_peer_list_next_done,
					     data_context));

    bool timeout = false;
    XorpTimer t = eventloop.set_flag_after_ms(1000, &timeout);
    while (!timeout && !data_context->valid) {
	DEBUGMSGTL((BgpMib::the_instance().name(), "waiting for next row...\n"));
	eventloop.run();  
    }


    if (timeout) {
	DEBUGMSGTL((BgpMib::the_instance().name(), "timeout while reading "
		     "table...\n"));
	return NULL; // timeout
    }
    
    (*my_data_context) = data_context; 

    // data_context goes away so store state in loop_context
    loop_context->more = data_context->more;

    uint32_t raw_ip = ntohl(data_context->peer_remote_ip.addr());

    snmp_set_var_typed_value(put_index_data, ASN_IPADDRESS, 
	reinterpret_cast<const u_char *> (&raw_ip), sizeof(uint32_t));

    return put_index_data;
}

/** handles requests for the bgpPeerTable table, if anything else needs to be done */
int
bgpPeerTable_handler(
    netsnmp_mib_handler               * handler,
    netsnmp_handler_registration      * reginfo,
    netsnmp_agent_request_info        * reqinfo,
    netsnmp_request_info              * requests) {

    BgpMib& bgp_mib = BgpMib::the_instance();

    DEBUGMSGTL((BgpMib::the_instance().name(), "bgpPeerTable_handler\n"));

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_variable_list *var;
    
    PeerDataContext * cntxt;
    
    // this is needed for delegated requests, which are most of the columns in
    // this table, as we'll further down

    netsnmp_delegated_cache* req_cache = netsnmp_create_delegated_cache(handler,
	reginfo, reqinfo, requests, NULL);

    for(request = requests; request; request = request->next) {
        var = request->requestvb;
        if (request->processed != 0)
            continue;

        /* the following extracts the row_data_context pointer set in the loop 
           functions above.  You can then use the results to help return data 
	   for the columns of the bgpPeerTable table in question */

        cntxt = (PeerDataContext*) netsnmp_extract_iterator_context(request);

        if (cntxt == NULL) {
            if (reqinfo->mode == MODE_GET) {
                netsnmp_set_request_error(reqinfo, request,SNMP_NOSUCHINSTANCE);
                continue;
            }
            /* XXX: no row existed, if you support creation and this is a
               set, start dealing with it here, else continue */
        }

        /* extracts the information about the table from the request */
        table_info = netsnmp_extract_table_info(request);
        /* table_info->colnum contains the column number requested */
        /* table_info->indexes contains a linked list of snmp variable
           bindings for the indexes of the table.  Values in the list
           have been set corresponding to the indexes of the
           request */
        if (table_info==NULL) {
            continue;
        }

        switch(reqinfo->mode) {
            /* the table_iterator helper should change all GETNEXTs
               into GETs for you automatically, so you don't have to
               worry about the GETNEXT case.  Only GETs and SETs need
               to be dealt with here */
            case MODE_GET:
                switch(table_info->colnum) {
                    case COLUMN_BGPPEERIDENTIFIER:
			{
			bgp_mib.send_get_peer_id("bgp", cntxt->peer_local_ip, 
			    cntxt->peer_local_port, cntxt->peer_remote_ip,
			    cntxt->peer_remote_port,
				       callback(get_peer_id_done, req_cache));
			requests->delegated++;
			break;
			}
                    case COLUMN_BGPPEERSTATE:  // since they use the same XRL we
                    case COLUMN_BGPPEERADMINSTATUS: // can use the same callback
			{
			bgp_mib.send_get_peer_status("bgp",
			    cntxt->peer_local_ip, cntxt->peer_local_port, 
			    cntxt->peer_remote_ip, cntxt->peer_remote_port, 
			    callback(get_peer_status_done, 
				     req_cache));
			requests->delegated++;
			break;
			}
                    case COLUMN_BGPPEERNEGOTIATEDVERSION:
 			{
			bgp_mib.send_get_peer_negotiated_version("bgp", 
			    cntxt->peer_local_ip, cntxt->peer_local_port, 
			    cntxt->peer_remote_ip, cntxt->peer_remote_port, 
			    callback(get_peer_negotiated_version_done,
				     req_cache));
			requests->delegated++;
			break;
			}
                    case COLUMN_BGPPEERLOCALADDR:	// not delegated
 			{
			uint32_t raw_ip = cntxt->peer_local_ip.addr();
                        snmp_set_var_typed_value(var, ASN_IPADDRESS, 
			    reinterpret_cast<const u_char *> (&raw_ip),
			    sizeof(raw_ip));
			break;
			}
                    case COLUMN_BGPPEERLOCALPORT:	// not delegated
 			{  
			uint32_t port = cntxt->peer_local_port;
                        snmp_set_var_typed_value(var, ASN_INTEGER,
			    reinterpret_cast<const u_char *>(&port),
			    sizeof(uint32_t)); 
			break;
			}
                    case COLUMN_BGPPEERREMOTEADDR:	// not delegated
 			{
			uint32_t raw_ip = cntxt->peer_remote_ip.addr();
                        snmp_set_var_typed_value(var, ASN_IPADDRESS, 
			    reinterpret_cast<const u_char *>(&raw_ip), 
			    sizeof(uint32_t));
			break;
			}
                    case COLUMN_BGPPEERREMOTEPORT:	// not delegated
			{
			uint32_t port = cntxt->peer_remote_port;
                        snmp_set_var_typed_value(var, ASN_INTEGER,
			    reinterpret_cast<const u_char *>(&port), 
			    sizeof(uint32_t)); 
			break;
			}
                    case COLUMN_BGPPEERREMOTEAS:
 			{
			bgp_mib.send_get_peer_as("bgp", cntxt->peer_local_ip, 
			    cntxt->peer_local_port, cntxt->peer_remote_ip,
			    cntxt->peer_remote_port,
			    callback(get_peer_as_done,
				     req_cache));
			requests->delegated++;
			break;
			}
                    case COLUMN_BGPPEERINUPDATES:
                    case COLUMN_BGPPEEROUTUPDATES:
                    case COLUMN_BGPPEERINTOTALMESSAGES:
                    case COLUMN_BGPPEEROUTTOTALMESSAGES:
                    case COLUMN_BGPPEERLASTERROR:
                    case COLUMN_BGPPEERINUPDATEELAPSEDTIME:
 			{
			bgp_mib.send_get_peer_msg_stats("bgp", 
			    cntxt->peer_local_ip, 
			    cntxt->peer_local_port, cntxt->peer_remote_ip,
			    cntxt->peer_remote_port,
			    callback(get_peer_msg_stats_done,
				     req_cache));
			requests->delegated++;
			break;
			}
                    case COLUMN_BGPPEERFSMESTABLISHEDTRANSITIONS:
                    case COLUMN_BGPPEERFSMESTABLISHEDTIME:
 			{
			bgp_mib.send_get_peer_established_stats("bgp", 
			    cntxt->peer_local_ip, cntxt->peer_local_port, 
			    cntxt->peer_remote_ip, cntxt->peer_remote_port,
			    callback(get_peer_established_stats, 
				     req_cache));
			requests->delegated++;
			break;
			}
                    case COLUMN_BGPPEERCONNECTRETRYINTERVAL:
                    case COLUMN_BGPPEERHOLDTIME:
                    case COLUMN_BGPPEERKEEPALIVE:
                    case COLUMN_BGPPEERHOLDTIMECONFIGURED:
                    case COLUMN_BGPPEERKEEPALIVECONFIGURED:
                    case COLUMN_BGPPEERMINASORIGINATIONINTERVAL:
                    case COLUMN_BGPPEERMINROUTEADVERTISEMENTINTERVAL:
 			{
			bgp_mib.send_get_peer_timer_config ("bgp", 
			    cntxt->peer_local_ip, cntxt->peer_local_port, 
			    cntxt->peer_remote_ip, cntxt->peer_remote_port, 
			    callback(get_peer_timer_config_done, 
						   req_cache));
			requests->delegated++;
			break;
			}
                    default:
                        /* We shouldn't get here */
                        snmp_log(LOG_ERR, "problem encountered in"
			    "bgpPeerTable_handler: unknown column\n");
                }
                break;

            case MODE_SET_RESERVE1:
                /* set handling... */

            default:
                snmp_log(LOG_ERR, "problem encountered in bgpPeerTable_handler:"
		    "unsupported mode\n");
        }
    }
    return SNMP_ERR_NOERROR;
}

void free_context(void * context, struct netsnmp_iterator_info_s * /* iinfo */)
{
    DEBUGMSGTL((BgpMib::the_instance().name(),"freeing context %x\n", context));
    if (NULL != context) free(context);
    context = NULL;
}

/* ==================== XRL callbacks == ===================================*/
void
get_peer_list_start_done(
    const XrlError& e, 
    const uint32_t* token,
    const bool* more,
    PeerLoopContext* loop_context)
{
    if (e == XrlError::OKAY()) {
	loop_context->peer_list_token = (*token);
	loop_context->more = (*more);
	loop_context->valid = true;
	DEBUGMSGTL((BgpMib::the_instance().name(),
                "token: %ud more: %d\n", *token, *more));		 
    } else {
    // XXX: deal with retries
    }

}

void get_peer_list_next_done(
    const XrlError& e, 
    const IPv4* local_ip, 
    const uint32_t* local_port, 
    const IPv4* remote_ip, 
    const uint32_t* remote_port,
    const bool* more,
    PeerDataContext* data_context)
{
    if (e == XrlError::OKAY()) {
	data_context->peer_local_ip = (*local_ip);
	data_context->peer_local_port = (*local_port);
	data_context->peer_remote_ip = (*remote_ip);
	data_context->peer_remote_port = (*remote_port); 
	data_context->more = (*more);
	data_context->valid = true;
	DEBUGMSGTL((BgpMib::the_instance().name(),
                "local_ip: %s more: %d\n", local_ip->str().c_str(), *more));		 
    } else {
    // XXX: deal with retries
    }
}

void get_peer_id_done(const XrlError& e, const IPv4* peer_id, 
			  netsnmp_delegated_cache* cache)
{
    if (e != XrlError::OKAY()) {
	// XXX: deal with retries
    } 
    
    DEBUGMSGTL((BgpMib::the_instance().name(), 
                "peer_id %s\n", peer_id->str().c_str()));		 

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERIDENTIFIER:       
	    {
	    uint32_t raw_id = peer_id->addr();
	    snmp_set_var_typed_value(request->requestvb, ASN_IPADDRESS, 
	    	reinterpret_cast<const u_char *> (&raw_id), sizeof(raw_id));
	    break;
	    }
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(), "get_peer_id_done"
		" called for the wrong column (%d)", table_info->colnum));
	    assert(0);
            return;
    }
    return;
}
void 
get_peer_status_done(const XrlError& e, const uint32_t* state, 
		    const uint32_t* admstus, netsnmp_delegated_cache * cache)
{

    if (e != XrlError::OKAY()) {
        // XXX: deal with retries
    }

    DEBUGMSGTL((BgpMib::the_instance().name(),
                "state %d admin status %d\n", * state, * admstus));

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERSTATE:       
            snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
		reinterpret_cast<const u_char *>(state), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEERADMINSTATUS: 
            snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
		reinterpret_cast<const u_char *>(admstus), sizeof(uint32_t)); 
	    break;
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(), "get_peer_state_done"
		" called for the wrong column (%d)", table_info->colnum));
	    assert(0);
            return;
    }
    return;
}

void
get_peer_negotiated_version_done(const XrlError& e, const int32_t * negver, 
    netsnmp_delegated_cache * cache)
{
    if (e != XrlError::OKAY()) {
        // XXX: deal with retries
    }

    DEBUGMSGTL((BgpMib::the_instance().name(), "negotd version %d\n", * negver));

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERNEGOTIATEDVERSION:       
            snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
		reinterpret_cast<const u_char *>(negver), sizeof(int32_t));
	    break;
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(), 
		"get_peer_state_done called for the wrong column (%d)", 
		table_info->colnum));
	    assert(0);
            return;
    }
    return;
}

void get_peer_as_done(const XrlError& e, const uint32_t * asnum, 
    netsnmp_delegated_cache * cache)
{
    if (e != XrlError::OKAY()) {
        // XXX: deal with retries
    }

    DEBUGMSGTL((BgpMib::the_instance().name(), "as number %d\n", * asnum));

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERREMOTEAS:       
            snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
		reinterpret_cast<const u_char *>(asnum), sizeof(uint32_t));
	    break;
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(), "get_peer_as_done called"
			 "for the wrong column (%d)", table_info->colnum));
	    assert(0);
            return;
    }
    return;
}

void get_peer_msg_stats_done(const XrlError& e, const uint32_t * inupd, 
    const uint32_t * outupd, const uint32_t * inmsgs, const uint32_t * outmsgs, 
    const uint32_t * lasterr, const uint32_t * inupdelps, 
    netsnmp_delegated_cache * cache)  
{
    if (e != XrlError::OKAY()) {
        // XXX: deal with retries
    }

    DEBUGMSGTL((BgpMib::the_instance().name(), "in upds %d out upds %d"
	"in msgs %d out msgs %d last err %d in updt elapsed %d\n", *inupd, 

	*outupd, *inmsgs, *outmsgs, *lasterr, *inupdelps));

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERINUPDATES:
	    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER, 
		reinterpret_cast<const u_char *>(inupd), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEEROUTUPDATES:
	   snmp_set_var_typed_value(request->requestvb, ASN_COUNTER, 
		reinterpret_cast<const u_char *>(outupd), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEERINTOTALMESSAGES:
	   snmp_set_var_typed_value(request->requestvb, ASN_COUNTER, 
		reinterpret_cast<const u_char *>(inmsgs), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEEROUTTOTALMESSAGES:
	   snmp_set_var_typed_value(request->requestvb, ASN_COUNTER, 
		reinterpret_cast<const u_char *>(outmsgs), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEERLASTERROR:
	   snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, 
		reinterpret_cast<const u_char *>(lasterr), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEERINUPDATEELAPSEDTIME:
	   snmp_set_var_typed_value(request->requestvb, ASN_GAUGE, 
		reinterpret_cast<const u_char *>(inupdelps), sizeof(uint32_t));
	    break;
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(), "get_peer_msg_stats_done"
			"called for the wrong column(%d)", table_info->colnum));
	    assert(0);
            return;
    }
    return;
}
void 
get_peer_established_stats(const XrlError& e, const uint32_t * trans, 
    const uint32_t * negtime, netsnmp_delegated_cache * cache)
{
    if (e != XrlError::OKAY()) {
        // XXX: deal with retries
    }

    DEBUGMSGTL((BgpMib::the_instance().name(), "transitions %d neg time %d\n", 
	* trans, * negtime));

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERFSMESTABLISHEDTRANSITIONS:
	    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER, 
		reinterpret_cast<const u_char *>(trans), sizeof(uint32_t));
	    break;
	case COLUMN_BGPPEERFSMESTABLISHEDTIME:
	    snmp_set_var_typed_value(request->requestvb, ASN_GAUGE, 
		reinterpret_cast<const u_char *>(negtime), sizeof(uint32_t));
	    break;
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(),
		"get_peer_established_stats for the wrong column (%d)", 
		table_info->colnum));
	    assert(0);
            return;
    }
    return;
}

void 
get_peer_timer_config_done(const XrlError& e, const uint32_t * retryint,  
    const uint32_t * holdtime, const uint32_t * keepalive,  
    const uint32_t * holdtimeconf, const uint32_t * keepaliveconf, 
    const uint32_t * minasorig, const uint32_t * minroutead,
    netsnmp_delegated_cache * cache)  
{
    if (e != XrlError::OKAY()) {
        // XXX: deal with retries
    }

    DEBUGMSGTL((BgpMib::the_instance().name(), "connect retry intvl  %d"
	"hold time %d keep alive %d hold time conf %d\n keep alive conf %d"
	"min as origin %d min rout adv intvl %d\n", *retryint, *holdtime,
	*keepalive, *holdtimeconf, *keepaliveconf, *minasorig, *minroutead)); 

    netsnmp_request_info *request;
    netsnmp_agent_request_info *reqinfo;

    
    if (!cache) {
        snmp_log(LOG_ERR, "illegal call to return delayed response\n");
        return;
    }

    // re-establish the previous pointers we are used to having 
    reqinfo = cache->reqinfo;
    request = cache->requests;

    // extracts the information about the table from the request 
    netsnmp_table_request_info *table_info;
    table_info = netsnmp_extract_table_info(request);

    // no longer delegated, since we'll complete the request down below
    request->delegated--;

    switch(table_info->colnum) {
	case COLUMN_BGPPEERCONNECTRETRYINTERVAL:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(retryint),
	       sizeof(uint32_t));
		break;
	case COLUMN_BGPPEERHOLDTIME:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(holdtime), 
	       sizeof(uint32_t));
		break;
	case COLUMN_BGPPEERKEEPALIVE:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(keepalive),
	       sizeof(uint32_t));
		break;
	case COLUMN_BGPPEERHOLDTIMECONFIGURED:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(holdtimeconf), 
	       sizeof(uint32_t));
		break;
	case COLUMN_BGPPEERKEEPALIVECONFIGURED:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(keepaliveconf),
	       sizeof(uint32_t));
		break;
	case COLUMN_BGPPEERMINASORIGINATIONINTERVAL:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(minasorig),
	       sizeof(uint32_t));
		break;
	case COLUMN_BGPPEERMINROUTEADVERTISEMENTINTERVAL:
	   snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, 
	       reinterpret_cast<const u_char *>(minroutead),
	       sizeof(uint32_t));
	    break;
	default:
            // we should never get here, so this is a really bad error 
	    DEBUGMSGTL((BgpMib::the_instance().name(),
		"get_peer_timer_config_done called for the wrong column (%d)", 
		table_info->colnum));
	    assert(0);
            return;
    }
    return;
}
