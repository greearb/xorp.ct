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
#include "bgp4_mib_1657_bgp4pathattrtable.hh"


// Local datatype
typedef struct
{
    uint32_t list_token;
    bool     valid;
} PathAttrLoopContext;

typedef struct
{
    IPv4 peer_id; 
    IPv4Net net; 
    uint8_t best;
    uint8_t origin;
    vector<uint8_t> aspath; 
    IPv4 nexthop;
    int32_t med; 
    int32_t localpref;
    int32_t atomic_agg;
    vector<uint8_t> aggregator;
    int32_t calc_localpref;
    vector<uint8_t> attr_unknown;
    bool valid;
    bool end_of_table;
} PathAttrDataContext;

// Local prototypes
static void free_context(void *, struct netsnmp_iterator_info_s *);
static void  get_v4_route_list_start_done(const XrlError&, 
    const uint32_t*, PathAttrLoopContext *); 
static void get_v4_route_list_next_done(const XrlError& e, const IPv4* peer_id,
    const IPv4Net*, const uint32_t *, const vector<uint8_t>*, const IPv4*, 
    const int32_t*, const int32_t*, const int32_t*, const vector<uint8_t>*,
    const int32_t*, const vector<uint8_t>*, PathAttrDataContext *);

/** Initialize the bgp4PathAttrTable table by defining its contents and how it's structured */
void
initialize_table_bgp4PathAttrTable(void)
{
    static oid bgp4PathAttrTable_oid[] = {1,3,6,1,2,1,15,6};
    netsnmp_table_registration_info *table_info;
    netsnmp_handler_registration *my_handler;
    netsnmp_iterator_info *iinfo;

    /* create the table structure itself */
    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);

    /* if your table is read only, it's easiest to change the
       HANDLER_CAN_RWRITE definition below to HANDLER_CAN_RONLY */
    my_handler = netsnmp_create_handler_registration("bgp4PathAttrTable",
                                             bgp4PathAttrTable_handler,
                                             bgp4PathAttrTable_oid,
                                             OID_LENGTH(bgp4PathAttrTable_oid),
                                             HANDLER_CAN_RWRITE);
            
    if (!my_handler || !table_info || !iinfo)
        return; /* mallocs failed */

    // Setting up the table's definition
    netsnmp_table_helper_add_indexes(table_info,
	ASN_IPADDRESS, /* index: bgp4PathAttrIpAddrPrefix */
        ASN_INTEGER, /* index: bgp4PathAttrIpAddrPrefixLen */
        ASN_IPADDRESS, /* index: bgp4PathAttrPeer */
        0);

    table_info->min_column = 1;
    table_info->max_column = 14;

    // iterator access routines 
    iinfo->get_first_data_point = bgp4PathAttrTable_get_first_data_point;
    iinfo->get_next_data_point = bgp4PathAttrTable_get_next_data_point;
    iinfo->free_loop_context_at_end = free_context;
    iinfo->free_loop_context = NULL;  // only free at end
    iinfo->free_data_context = free_context; 

    iinfo->table_reginfo = table_info;

    // registering the table with the master agent
    netsnmp_register_table_iterator(my_handler, iinfo);
}

/** Initializes the bgp4_mib_1657_bgp4pathattrtable module */
void
init_bgp4_mib_1657_bgp4pathattrtable(void)
{

    DEBUGMSGTL((BgpMib::the_instance().name(),
	"Initializing bgp4PathAttrTable...\n"));

  /* here we initialize all the tables we're planning on supporting */
    initialize_table_bgp4PathAttrTable();
}

// bgp4PathAttrTable_get_first_data_point - initialize the table contexts
//
// This function is called by snmpd before any access to the data in the
// bgp4PathAttrTable.  It initializes the loop context (in this case, the peer
// list identifier returned by the bgp module, aka token).

netsnmp_variable_list *
bgp4PathAttrTable_get_first_data_point(void **my_loop_context, 
				       void **my_data_context,
				       netsnmp_variable_list *put_index_data,
				       netsnmp_iterator_info *mydata)
{
    PathAttrLoopContext* loop_context;
    BgpMib& bgp_mib = BgpMib::the_instance();
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance();

    DEBUGMSGTL((BgpMib::the_instance().name(), "get_first_data_point\n"));

    /* allocate loop context for this table */
    loop_context = SNMP_MALLOC_TYPEDEF(PathAttrLoopContext);
    if (NULL == loop_context) return NULL;

    BgpMib::CB24 cb24;    // see bgp_xif.hh for this prototype
    loop_context->valid = false;
    cb24 = callback(get_v4_route_list_start_done, loop_context);
    bgp_mib.send_get_v4_route_list_start("bgp", cb24);
    bool timeout = false;
    XorpTimer t = eventloop.set_flag_after_ms(1000, &timeout);
    while (!timeout && !loop_context->valid) {
	DEBUGMSGTL((BgpMib::the_instance().name(),"waiting for v4 route "
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
    return bgp4PathAttrTable_get_next_data_point(my_loop_context, 
	my_data_context, put_index_data, mydata);
}

// bgp4PathAttrTable_get_next_data_point - update table contexts after each row
// 
// This function updates the data context for each row and writes the values of
// the index columns for this row.  

netsnmp_variable_list *
bgp4PathAttrTable_get_next_data_point(void **my_loop_context, 
				      void **my_data_context,
				      netsnmp_variable_list *put_index_data,
				      netsnmp_iterator_info * /* mydata */)
{
    BgpMib& bgp_mib = BgpMib::the_instance();
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance();
    PathAttrLoopContext* loop_context = 
	(PathAttrLoopContext*) (*my_loop_context);
    PathAttrDataContext* data_context;  

    
    // allocate data context for this row
    data_context = SNMP_MALLOC_TYPEDEF(PathAttrDataContext);
    if (NULL == data_context) return NULL;

    DEBUGMSGTL((BgpMib::the_instance().name(), "get_next_data_point\n"));

    BgpMib::CB26 cb26;    // see bgp_xif.hh for this prototype

    cb26 = callback(get_v4_route_list_next_done, data_context);
    data_context->valid = data_context->end_of_table = false;

    bgp_mib.send_get_v4_route_list_next("bgp", loop_context->list_token,
	cb26);

    bool timeout = false;
    XorpTimer t = eventloop.set_flag_after_ms(1000, &timeout);
    while (!timeout && !data_context->valid) {
	DEBUGMSGTL((BgpMib::the_instance().name(), 
	    "waiting for next row...\n"));
	eventloop.run();  
    }

    if (timeout || data_context->end_of_table) {
	if (timeout) DEBUGMSGTL((BgpMib::the_instance().name(), 
	    "timeout while reading table...\n"));
	return NULL; 
    }
    
    (*my_data_context) = data_context; 

    netsnmp_variable_list * vptr = put_index_data; // bgp4PathAttrIpAddrPrefix 
    uint32_t raw_ip = ntohl(data_context->net.masked_addr().addr());
    snmp_set_var_value(vptr, reinterpret_cast<const u_char *> (&raw_ip), 
	sizeof(uint32_t));

    vptr = vptr->next_variable;		 // bgp4PathAttrIpAddrPrefixLen 
    size_t len = data_context->net.prefix_len();
    snmp_set_var_typed_value(vptr,  ASN_INTEGER,
	reinterpret_cast<const u_char *> (&len), 
	sizeof(size_t));

    vptr = vptr->next_variable;         // bgp4PathAttrPeer 
    raw_ip = ntohl(data_context->peer_id.addr());
    snmp_set_var_value(vptr, reinterpret_cast<const u_char *>
	(&raw_ip), sizeof(uint32_t));

    return put_index_data;

}

/** handles requests for the bgp4PathAttrTable table, if anything else needs to be done */
int
bgp4PathAttrTable_handler(
    netsnmp_mib_handler               * /* handler */,
    netsnmp_handler_registration      * /* reginfo */,
    netsnmp_agent_request_info        * reqinfo,
    netsnmp_request_info              * requests) 
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_variable_list *var;

    DEBUGMSGTL((BgpMib::the_instance().name(), 
	"bgp4PathAttrTable_handler called...\n"));
    
    for(request = requests; request; request = request->next) {
        var = request->requestvb;
        if (request->processed != 0)
            continue;

        // extract the my_data_context pointer set in the loop functions above
	
        PathAttrDataContext* cntxt = 
	    (PathAttrDataContext*) netsnmp_extract_iterator_context(request);
        if (cntxt == NULL) {
            if (reqinfo->mode == MODE_GET) {
                netsnmp_set_request_error(reqinfo, request, 
		    SNMP_NOSUCHINSTANCE);
                continue;
            }
        }

        /* extracts the information about the table from the request */
        table_info = netsnmp_extract_table_info(request);
        if (table_info==NULL) {
            continue;
        }

	DEBUGMSGTL((BgpMib::the_instance().name(), "col: %d\n", 
	    table_info->colnum));
        switch(reqinfo->mode) {
            /* the table_iterator helper should change all GETNEXTs
               into GETs for you automatically, so you don't have to
               worry about the GETNEXT case.  Only GETs and SETs need
               to be dealt with here */
            case MODE_GET:
                switch(table_info->colnum) {
                    case COLUMN_BGP4PATHATTRPEER:
			{
                        uint32_t raw_ip = cntxt->peer_id.addr();
                        snmp_set_var_typed_value(var, ASN_IPADDRESS,
                            reinterpret_cast<const u_char *> (&raw_ip),
                            sizeof(raw_ip));
                        break;
			}
                    case COLUMN_BGP4PATHATTRIPADDRPREFIXLEN:
			{
			uint32_t len = cntxt->net.prefix_len();
                        snmp_set_var_typed_value(var, ASN_INTEGER,
			    reinterpret_cast<const u_char *> (&len), 
			    sizeof(len));
                        break;
			}
                    case COLUMN_BGP4PATHATTRIPADDRPREFIX:
			{
			uint32_t raw_ip = cntxt->net.masked_addr().addr();	
                        snmp_set_var_typed_value(var, ASN_IPADDRESS, 
			    reinterpret_cast<const u_char *> (&raw_ip), 
			    sizeof(raw_ip));
                        break;
			}
                    case COLUMN_BGP4PATHATTRORIGIN:
			{
			uint32_t origin = static_cast<uint32_t>(cntxt->origin);
                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<const u_char *> (&origin),
			    sizeof(origin)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRASPATHSEGMENT:
			{
			u_char * aspath = &(cntxt->aspath[0]); 	
                        snmp_set_var_typed_value(var, ASN_OCTET_STR, 
			    reinterpret_cast<const u_char *> (aspath),
			    cntxt->aspath.size());
                        break;
			}
                    case COLUMN_BGP4PATHATTRNEXTHOP:
			{
			uint32_t raw_ip = cntxt->nexthop.addr();	
                        snmp_set_var_typed_value(var, ASN_IPADDRESS, 
			    reinterpret_cast<const u_char *> (&raw_ip), 
			    sizeof(raw_ip));
                        break;
			}
                    case COLUMN_BGP4PATHATTRMULTIEXITDISC:
			{
			uint32_t med = cntxt->med;
                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<const u_char *> (&med),
			    sizeof(med)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRLOCALPREF:
			{
			uint32_t localpref = cntxt->localpref;
                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<const u_char *> (&localpref),
			    sizeof(localpref)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRATOMICAGGREGATE:
			{
			uint32_t atomic_agg = cntxt->atomic_agg;
                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<const u_char *> (&atomic_agg),
			    sizeof(atomic_agg)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRAGGREGATORAS:
			{
			// see BGPMain::extract_attributes in bgp/main.cc 
			// for details on this encoding
			uint32_t aggr_as = 0;
			if (cntxt->aggregator.size()) {
			    aggr_as |= (cntxt->aggregator[4] << 8);
			    aggr_as |= (cntxt->aggregator[5]);
			} 

                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<u_char *> (&aggr_as), 
			    sizeof(uint32_t)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRAGGREGATORADDR:
			{
			// see BGPMain::extract_attributes in bgp/main.cc 
			// for details on this encoding
			uint32_t aggregator = 0;
			if (cntxt->aggregator.size()) {
			    aggregator |= (cntxt->aggregator[0] << 24);
			    aggregator |= (cntxt->aggregator[1] << 16);
			    aggregator |= (cntxt->aggregator[2] << 8);
			    aggregator |= (cntxt->aggregator[3]);
			}
                        snmp_set_var_typed_value(var, ASN_IPADDRESS, 
			    reinterpret_cast<u_char *> (&aggregator), 
			    sizeof(uint32_t)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRCALCLOCALPREF:
			{
			uint32_t lp = static_cast<uint32_t>(cntxt->localpref);
                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<const u_char *> (&lp),
			    sizeof(lp)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRBEST:
			{
			uint32_t best = static_cast<uint32_t>(cntxt->best);
                        snmp_set_var_typed_value(var, ASN_INTEGER, 
			    reinterpret_cast<const u_char *> (&best),
			    sizeof(best)); 
                        break;
			}
                    case COLUMN_BGP4PATHATTRUNKNOWN:
			{
			uint32_t unknown = 0;
                        snmp_set_var_typed_value(var, ASN_OCTET_STR,
			    reinterpret_cast<const u_char *> (&unknown),
			    sizeof(unknown)); 
                        break;
			}
                    default:
                        /* We shouldn't get here */
                        snmp_log(LOG_ERR, "problem encountered in bgp4PathAttrTable_handler: unknown column\n");
                }
                break;

            case MODE_SET_RESERVE1:
                /* set handling... */

            default:
                snmp_log(LOG_ERR, "problem encountered in bgp4PathAttrTable_handler: unsupported mode\n");
        }
    }
    return SNMP_ERR_NOERROR;
}


static void free_context(void * context, struct netsnmp_iterator_info_s *)
{
    DEBUGMSGTL((BgpMib::the_instance().name(),"freeing context %x\n", context));
    if (NULL != context) free(context);
    context = NULL;
}

static void  
get_v4_route_list_start_done(
    const XrlError& e,
    const uint32_t* token, 
    PathAttrLoopContext * loop_context) 
{
    if (e == XrlError::OKAY()) {
	loop_context->list_token = (*token);
	loop_context->valid = true;
	DEBUGMSGTL((BgpMib::the_instance().name(),
                "token: %ud \n", *token));		 
    } else {
    // XXX: deal with retries
    }


}

static void
get_v4_route_list_next_done(const XrlError& e, 
			    const IPv4* peer_id, 
			    const IPv4Net* net, 
			    const uint32_t *best_and_origin, 
			    const vector<uint8_t>* aspath, 
			    const IPv4* nexthop, 
			    const int32_t* med, 
			    const int32_t* localpref, 
			    const int32_t* atomic_agg, 
			    const vector<uint8_t>* aggregator, 
			    const int32_t* calc_localpref, 
			    const vector<uint8_t>* attr_unknown,
			    PathAttrDataContext * data_context)
{
    data_context->valid = true;
    if (e != XrlError::OKAY()) {
	data_context->end_of_table = true;
	return;
    }
    DEBUGMSGTL((BgpMib::the_instance().name(), 
	"get_v4_route_list_next_done\n"));
    data_context->peer_id = (*peer_id);
    data_context->net = (*net);
    data_context->best = (*best_and_origin)>>16;
    data_context->origin = (*best_and_origin) & 0xFF;
    data_context->med = (*med);
    data_context->localpref = (*localpref);
    data_context->aspath = (*aspath);
    data_context->nexthop = (*nexthop);
    data_context->atomic_agg = (*atomic_agg);
    data_context->aggregator = (*aggregator);
    data_context->calc_localpref = (*calc_localpref);
    data_context->attr_unknown = (*attr_unknown);
    data_context->aspath = (*aspath);
    DEBUGMSGTL((BgpMib::the_instance().name(), 
	"peer_id:%s net:%s\n", peer_id->str().c_str(),
	net->masked_addr().str().c_str()));
    
}
