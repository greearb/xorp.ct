/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998,1999 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

const int MON_RTYPELEN = 8;
const int MON_PHYLEN = 16;	// Must fit Area ID
const int MON_STATELEN = 8;
const int MON_ITYPELEN = 8;

/* Definition of ospfd monitor requests and responses.
 * Requests just give enough information to uniquely define the
 * element.
 */

struct MonRqArea {
    InAddr area_id;
};

struct MonRqIfc {
    int32 phyint;
    InAddr if_addr;
};

struct MonRqVL {
    InAddr transit_area;
    InAddr endpoint_id;
};

struct MonRqNbr {
    int32 phyint;
    InAddr nbr_addr;
};

struct MonRqLsa {
    InAddr area_id;
    uns32 ls_type;
    uns32 ls_id;
    uns32 adv_rtr;
};

struct MonRqLLLsa {
    // Interface specification
    InAddr if_addr;
    int phyint;
    aid_t taid;	// Transit area ID for virtual links
    // Informational only
    aid_t a_id;
    char phyname[MON_PHYLEN]; // Interface name
    // Link-local LSA specification
    uns32 ls_type;
    uns32 ls_id;
    uns32 adv_rtr;
};

struct MonRqRte {
    InAddr net;
    InAddr mask;
};

/* Response to a request for global statistics.
 */

struct StatRsp {
    InAddr router_id;
    uns32 n_aselsas;    
    uns32 asexsum;
    uns32 n_ase_import;
    uns32 extdb_limit;
    uns32 n_dijkstra;
    uns16 n_area;
    uns16 n_dbx_nbrs;
    byte mospf;
    byte inter_area_mc;
    byte inter_AS_mc;
    byte overflow_state;
    byte vmajor;
    byte vminor;
    uns16 fill1;
    uns32 n_orig_allocs;
};

/* Response to a request for area statistics.
 */

struct AreaRsp {
    InAddr area_id;

    uns16 n_ifcs;	// Number of active interfaces
    uns16 n_cfgifcs;	// # configured interfaces
    uns16 n_routers;	// Number of reachable routers
    uns16 n_rtrlsas;
    uns16 n_netlsas;
    uns16 n_summlsas;
    uns16 n_asbrlsas;
    uns16 n_grplsas;
    uns32 dbxsum;	// Area database checksum
    byte transit;
    byte demand;
    byte stub;
    byte import_summ;
    uns32 n_ranges;
};

/* Response to an interface query.
 */

struct IfcRsp {
    InAddr if_addr;	// Interface IP address
    int32 if_phyint;	// The physical interface
    InMask if_mask;	// Interface address mask
    uns32 area_id;	// Interface's Area ID
    uns32 transit_id;	// Transit area, for virtual links
    uns32 endpt_id;	// Endpoint of virtual link
    int32 if_IfIndex;	// MIB-II IfIndex
    uns32 if_dint;	// Router dead interval
    uns32 if_pint;	// Poll interval (NBMA only)
    InAddr if_dr; 	// Designated router
    InAddr if_bdr; 	// Backup designated router
    uns16 mtu;		// Max IP datagram in bytes
    uns16 if_cost;	// Cost
    uns16 if_hint;	// Hello interval
    uns16 if_autype;	// Authentication type
    byte if_rxmt;	// Retransmission interval
    byte if_xdelay;	// Transmission delay, in seconds
    byte if_drpri;	// Router priority
    byte if_demand;	// Demand-based flooding?
    byte if_mcfwd;	// Multicast forwardimg
    byte if_nnbrs;	// Number of neighbors
    byte if_nfull;	// Number of fully adjacent neighbors
    byte pad1;		// Padding
    char if_state[MON_STATELEN]; // Current interface state
    char type[MON_ITYPELEN]; // Interface type
    char phyname[MON_PHYLEN]; // Interface name
};

/* Response to a neighbor query.
 */

struct NbrRsp {
    InAddr n_addr; 	// Its IP address
    rtid_t n_id; 	// Its Router ID
    int32 phyint;	// The physical interface
    InAddr transit_id;  // Transit area ID (virtual neighbors)
    uns32 endpt_id;	// Endpoint of virtual link
    uns32 n_ddlst;	// Database summary list
    uns32 n_rqlst;	// Request list
    uns32 rxmt_count;	// Count of all rxmt queues together
    uns32 n_rxmt_window;// # consecutive retransmissions allowed
    InAddr n_dr;	// Neighbors idea of DR
    InAddr n_bdr; 	// Neighbors idea of Backup DR
    byte n_opts; 	// Options advertised by neighbor
    byte n_imms; 	// Bits rcvd in last DD packet
    byte n_adj_pend;
    byte n_pri;		// Its Router Priority
    char n_state[MON_STATELEN];	// Current neighbor state
    char phyname[MON_PHYLEN]; // Interface name
};

/* Response to routing table entry query.
 */

struct RteRsp {
    InAddr net;
    InAddr mask;
    char type[MON_RTYPELEN];
    uns32 cost;
    uns32 o_cost;
    uns32 tag;
    uns32 npaths;
    struct {
	char phyname[MON_PHYLEN];
	InAddr gw;
    } hops[MAXPATH];
};

/* Response to request for next Opaque-LSA.
 * Fixed length structure followed by Opaque-LSA in
 * its entireity.
 */

struct OpqRsp {
    int32 phyint;
    InAddr if_addr;
    aid_t a_id;
};

/* Overall format of monitoring requests and responses.
 */

struct MonHdr {
    byte version;
    byte retcode;
    byte exact;
    byte id;
};

struct MonMsg {
    MonHdr hdr;
    union {
        MonRqArea arearq;// Requests
        MonRqIfc ifcrq;
        MonRqVL vlrq;
	MonRqNbr nbrrq;
	MonRqLsa lsarq;
        MonRqLLLsa lllsarq;
	MonRqRte rtrq;

        StatRsp statrsp;// Responses
	AreaRsp arearsp;
	IfcRsp ifcrsp;
	NbrRsp nbrsp;
	RteRsp rtersp;
        OpqRsp opqrsp;
    } body;
};

/* Encoding of the type field
 * Can be either a request or a response.
 */

enum {
    MonReq_Stat = 1,	// Global statistics
    MonReq_Area,	// Area statistics
    MonReq_Ifc,		// Interface statistics
    MonReq_VL,		// Virtual link statistics
    MonReq_Nbr,		// Neighbor statistics
    MonReq_VLNbr,	// Virtual neighbor statistics
    MonReq_LSA,		// Dump LSA contents
    MonReq_Rte,		// Dump routing table entry
    MonReq_OpqReg,	// Register for Opaque-LSAs
    MonReq_OpqNext,	// Get next Opaque-LSA
    MonReq_LLLSA,	// Dump Link-local LSA contents

    Stat_Response = 100, // Global statistics response
    Area_Response,	// Area response
    Ifc_Response,	// Interface response
    Nbr_Response,	// Neighbor response
    LSA_Response,	// LSA
    Rte_Response,	// Routing table entry
    OpqLSA_Response,	// Opaque-LSA response
    LLLSA_Response,	// Link-local LSA

    OSPF_MON_VERSION = 1, // Version of monitoring messages
};
