/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
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

#include "ospfinc.h"
#include "monitor.h"
#include "system.h"
#include "ifcfsm.h"

/* If necessary, allocate an area in which to construct
 * a monitor response.
 */

MonMsg *OSPF::get_monbuf(int size)

{
    if (size > mon_size) {
	mon_size = size;
	delete [] mon_buff;
	mon_buff = new byte[size];
    }

    return((MonMsg *)mon_buff);
}

/* Get global OSPF statistics.
 */

void OSPF::global_stats(MonMsg *req, int conn_id)

{
    int mlen;
    MonMsg *msg;
    printf("----\nglobal_stats\n");

    mlen = sizeof(MonHdr) + sizeof(StatRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 0;
    msg->hdr.exact = 0;
    msg->hdr.id = req->hdr.id;

    msg->body.statrsp.router_id = hton32(myid);
    msg->body.statrsp.n_aselsas = hton32(extLSAs.size());
    msg->body.statrsp.asexsum = hton32(ase_xsum);
    msg->body.statrsp.n_ase_import = hton32(n_extImports);
    msg->body.statrsp.extdb_limit = hton32(ExtLsdbLimit);
    msg->body.statrsp.n_dijkstra = hton32(n_dijkstras);
    msg->body.statrsp.n_area = hton16(n_area);
    msg->body.statrsp.n_dbx_nbrs = hton16(n_dbx_nbrs);
    msg->body.statrsp.mospf = g_mospf_enabled ? 1 : 0;
    msg->body.statrsp.inter_area_mc = inter_area_mc ? 1 : 0;
    msg->body.statrsp.inter_AS_mc = inter_AS_mc;
    msg->body.statrsp.overflow_state = OverflowState ? 1 : 0;
    msg->body.statrsp.vmajor = vmajor;
    msg->body.statrsp.vminor = vminor;
    msg->body.statrsp.fill1 = 0;
    msg->body.statrsp.n_orig_allocs = hton32(n_orig_allocs);

    sys->monitor_response(msg, Stat_Response, mlen, conn_id);
}

/* Get area statistics.
 */

void OSPF::area_stats(class MonMsg *req, int conn_id)

{
    MonRqArea *areq;
    aid_t a_id;
    SpfArea *ap;
    int mlen;
    MonMsg *msg;

    areq = &req->body.arearq;
    a_id = ntoh32(areq->area_id);
    if (req->hdr.exact != 0)
        ap = FindArea(a_id);
    else
	ap = NextArea(a_id);

    mlen = sizeof(MonHdr) + sizeof(AreaRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (ap) {
        AreaRsp *arsp;
	IfcIterator iter(ap);
	int i;
	msg->hdr.retcode = 0;
	arsp = &msg->body.arearsp;
	arsp->area_id = hton32(ap->a_id);
	arsp->n_ifcs = hton16(ap->n_active_if);
	for (i = 0; iter.get_next(); )
	    i++;
	arsp->n_cfgifcs = hton16(i);
	arsp->n_routers = hton16(ap->n_routers);
	arsp->n_rtrlsas = hton16(ap->rtrLSAs.size());
	arsp->n_netlsas = hton16(ap->netLSAs.size());
	arsp->n_summlsas = hton16(ap->summLSAs.size());
	arsp->n_asbrlsas = hton16(ap->asbrLSAs.size());
	arsp->n_grplsas = hton16(ap->grpLSAs.size());
	arsp->dbxsum = hton32(ap->db_xsum);
	arsp->transit = ap->a_transit ? 1 : 0;
	arsp->demand = ap->donotage() ? 1 : 0;
	arsp->stub = ap->a_stub ? 1 : 0;
	arsp->import_summ = ap->a_import ? 1 : 0;
	arsp->n_ranges = hton32(ap->ranges.size());
    }

    sys->monitor_response(msg, Area_Response, mlen, conn_id);
}

/* Respond to a query for an interface's statistics.
 */

void OSPF::interface_stats(class MonMsg *req, int conn_id)

{
    MonRqIfc *ireq;
    int phyint;
    InAddr addr;
    int mlen;
    MonMsg *msg;
    SpfIfc *ip;
    printf("----\ninterface_stats\n");

    ireq = &req->body.ifcrq;
    phyint = ntoh32(ireq->phyint);
    printf("phyint = %d\n", phyint);
    addr = ntoh32(ireq->if_addr);
    printf("addr = %x, exact=%d\n", addr, req->hdr.exact);
    if (req->hdr.exact != 0)
        ip = find_ifc(addr, phyint);
    else
	ip = next_ifc(addr, phyint);

    mlen = sizeof(MonHdr) + sizeof(IfcRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (ip) {
        IfcRsp *irsp;

	msg->hdr.retcode = 0;
	irsp = &msg->body.ifcrsp;
	ip->dump_stats(irsp);

    }

    sys->monitor_response(msg, Ifc_Response, mlen, conn_id);
}

/* Dump an interface's statistics in a form that can be
 * sent to other computers for analysis.
 */

void SpfIfc::dump_stats(IfcRsp *irsp)

{
    char *ifstates(int);
    char *iftypes(int);
    char *phyname;

    printf("----\ndump_stats\n");

    irsp->if_addr = transit_area() ? hton32(*vl_endpt()) : hton32(if_addr);
    irsp->if_phyint = hton32(if_phyint);
    irsp->if_mask = hton32(if_mask);
    irsp->area_id = hton32(if_area->id());
    irsp->transit_id = transit_area() ? hton32(transit_area()->id()) : 0;
    irsp->endpt_id = vl_endpt() ? hton32(*vl_endpt()) : 0;
    irsp->if_IfIndex = hton32(if_IfIndex);
    irsp->if_dint = hton32(if_dint);
    irsp->if_pint = hton32(if_pint);
    irsp->if_dr = hton32(if_dr);
    irsp->if_bdr = hton32(if_bdr);
    irsp->mtu = hton16(if_mtu);
    irsp->if_cost = hton16(cost());
    irsp->if_hint = hton16(if_hint);
    irsp->if_autype = hton16(if_autype);
    irsp->if_rxmt = if_rxmt;
    irsp->if_xdelay = if_xdelay;
    irsp->if_drpri = if_drpri;
    irsp->if_demand = (if_demand ? 1 : 0);
    irsp->if_mcfwd = if_mcfwd;
    irsp->if_nnbrs = if_nnbrs;
    irsp->if_nfull = if_nfull;
    irsp->pad1 = 0;
    strncpy(irsp->if_state, ifstates(if_state), MON_STATELEN);
    strncpy(irsp->type, iftypes(type()), MON_ITYPELEN);
    if (!transit_area()) {
	phyname = sys->phyname(if_phyint);
	strncpy(irsp->phyname, phyname, MON_PHYLEN);
    }
    else {
        InAddr area_id;
	char *p;
	area_id = hton32(transit_area()->id());
	p = (char *) &area_id;
	sprintf(irsp->phyname, "%d.%d.%d.%d",
		(int)p[0], (int)p[1], (int)p[2], (int)p[3]);
    }
}

/* Respond to a query for a virtual link's statistics.
 */

void OSPF::vl_stats(class MonMsg *req, int conn_id)

{
    MonRqVL *vlreq;
    InAddr taid;
    InAddr endpt;
    int mlen;
    MonMsg *msg;
    SpfIfc *ip;

    vlreq = &req->body.vlrq;
    taid = ntoh32(vlreq->transit_area);
    endpt = ntoh32(vlreq->endpoint_id);
    if (req->hdr.exact != 0)
        ip = find_vl(taid, endpt);
    else
	ip = next_vl(taid, endpt);

    mlen = sizeof(MonHdr) + sizeof(IfcRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (ip) {
        IfcRsp *irsp;

	msg->hdr.retcode = 0;
	irsp = &msg->body.ifcrsp;
	ip->dump_stats(irsp);
    }

    sys->monitor_response(msg, Ifc_Response, mlen, conn_id);
}

/* Respond to a query for a neighbor's statistics.
 */

void OSPF::neighbor_stats(class MonMsg *req, int conn_id)

{
    MonRqNbr *nreq;
    int phyint;
    InAddr addr;
    int mlen;
    MonMsg *msg;
    SpfNbr *np;

    nreq = &req->body.nbrrq;
    phyint = ntoh32(nreq->phyint);
    addr = ntoh32(nreq->nbr_addr);
    if (req->hdr.exact != 0)
        np = find_nbr(addr, phyint);
    else
	np = next_nbr(addr, phyint);

    mlen = sizeof(MonHdr) + sizeof(NbrRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (np) {
        NbrRsp *nrsp;

	msg->hdr.retcode = 0;
	nrsp = &msg->body.nbrsp;
	np->dump_stats(nrsp);
    }

    sys->monitor_response(msg, Nbr_Response, mlen, conn_id);
}

/* Dump the neighbor's statistics into a form suitable
 * for shipment to another program. Used for both real
 * neighbors and virtual neighbors.
 */

void SpfNbr::dump_stats(NbrRsp *nrsp)

{
    char *nbrstates(int);
    char *phyname;

    nrsp->n_addr = hton32(addr());
    nrsp->n_id = hton32(id());
    nrsp->phyint = hton32(n_ifp->if_phyint);
    nrsp->transit_id = n_ifp->transit_area() 
		       ? hton32(n_ifp->transit_area()->id()) : 0;
    nrsp->endpt_id = n_ifp->vl_endpt() ? hton32(*n_ifp->vl_endpt()) : 0;
    nrsp->n_ddlst = hton32(n_ddlst.count());
    nrsp->n_rqlst = hton32(n_rqlst.count());
    nrsp->rxmt_count = hton32(rxmt_count);
    nrsp->n_rxmt_window = hton32(n_rxmt_window);
    nrsp->n_dr = hton32(n_dr);
    nrsp->n_bdr = hton32(n_bdr);
    nrsp->n_opts= n_opts;
    nrsp->n_imms= n_imms;
    nrsp->n_adj_pend = n_adj_pend;
    nrsp->n_pri = n_pri;
    strncpy(nrsp->n_state, nbrstates(n_state), MON_STATELEN);
    if (!n_ifp->transit_area()) {
	phyname = sys->phyname(n_ifp->if_phyint);
	strncpy(nrsp->phyname, phyname, MON_PHYLEN);
    }
    else {
        InAddr area_id;
	char *p;
	area_id = hton32(n_ifp->transit_area()->id());
	p = (char *) &area_id;
	sprintf(nrsp->phyname, "%d.%d.%d.%d",
		(int)p[0], (int)p[1], (int)p[2], (int)p[3]);
    }
}

/* Respond to a query for a virtual neighbor's statistics.
 */

void OSPF::vlnbr_stats(class MonMsg *req, int conn_id)

{
    MonRqVL *vlreq;
    InAddr taid;
    InAddr endpt;
    int mlen;
    MonMsg *msg;
    SpfIfc *ip;

    vlreq = &req->body.vlrq;
    taid = ntoh32(vlreq->transit_area);
    endpt = ntoh32(vlreq->endpoint_id);
    if (req->hdr.exact != 0)
        ip = find_vl(taid, endpt);
    else
	ip = next_vl(taid, endpt);

    mlen = sizeof(MonHdr) + sizeof(NbrRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (ip && ip->if_nlst) {
        NbrRsp *nrsp;

	msg->hdr.retcode = 0;
	nrsp = &msg->body.nbrsp;
	ip->if_nlst->dump_stats(nrsp);
    }

    sys->monitor_response(msg, Nbr_Response, mlen, conn_id);
}


/* Respond to a query to access an OSPF LSA.
 */

void OSPF::lsa_stats(class MonMsg *req, int conn_id)

{
    MonRqLsa *lsareq;
    aid_t a_id;
    byte ls_type;
    lsid_t id;
    rtid_t advrtr;
    LSA *lsap;
    int mlen;
    MonMsg *msg;

    lsareq = &req->body.lsarq;
    a_id = ntoh32(lsareq->area_id);
    ls_type = ntoh32(lsareq->ls_type);
    id = ntoh32(lsareq->ls_id);
    advrtr = ntoh32(lsareq->adv_rtr);

    lsap = 0;

    if (req->hdr.exact != 0) {
	SpfArea *ap;
	if ((ap = FindArea(a_id)) || flooding_scope(ls_type) == GlobalScope)
	    lsap = FindLSA(0, ap, ls_type, id, advrtr);
    } else
        lsap = NextLSA(a_id, ls_type, id, advrtr);

    mlen = sizeof(MonHdr) + sizeof(MonRqLsa);
    if (lsap)
        mlen += lsap->ls_length();
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (lsap) {
        MonRqLsa *lsarsp;
	LShdr *hdr;
	msg->hdr.retcode = 0;
	lsarsp = &msg->body.lsarq;
	lsarsp->area_id = lsap->lsa_ap ? hton32(lsap->lsa_ap->a_id) : 0;
	lsarsp->ls_type = hton32(lsap->lsa_type);
	lsarsp->ls_id = hton32(lsap->ls_id());
	lsarsp->adv_rtr = hton32(lsap->adv_rtr());
	hdr = BuildLSA(lsap);
	memcpy((lsarsp + 1), hdr, lsap->ls_length());
    }

    sys->monitor_response(msg, LSA_Response, mlen, conn_id);
}

/* Respond to a query to access a Link-local LSA.
 */

void OSPF::lllsa_stats(class MonMsg *req, int conn_id)

{
    MonRqLLLsa *llreq;
    InAddr if_addr;
    int phyint;
    aid_t taid;
    byte ls_type;
    lsid_t id;
    rtid_t advrtr;
    LSA *lsap;
    int mlen;
    MonMsg *msg;
    char *phyname;

    llreq = &req->body.lllsarq;
    if_addr = ntoh32(llreq->if_addr);
    phyint = ntoh32(llreq->phyint);
    taid = ntoh32(llreq->taid);
    ls_type = ntoh32(llreq->ls_type);
    id = ntoh32(llreq->ls_id);
    advrtr = ntoh32(llreq->adv_rtr);

    lsap = 0;

    if (req->hdr.exact != 0) {
	SpfIfc *ip;
	if (taid != 0)
	    ip = find_vl(taid, if_addr);
	else
	    ip = find_ifc(if_addr, phyint);
	if (ip)
	    lsap = FindLSA(ip, 0, ls_type, id, advrtr);
    } else
        lsap = NextLSA(if_addr, phyint, taid, ls_type, id, advrtr);

    mlen = sizeof(MonHdr) + sizeof(MonRqLLLsa);
    if (lsap)
        mlen += lsap->ls_length();
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (lsap) {
        MonRqLLLsa *llrsp;
	LShdr *hdr;
	SpfIfc *ip;
	ip = lsap->lsa_ifp;
	msg->hdr.retcode = 0;
	llrsp = &msg->body.lllsarq;
	if (!ip->is_virtual()) {
	    llrsp->if_addr = hton32(ip->if_addr);
	    llrsp->taid = 0;
	    phyname = sys->phyname(ip->if_phyint);
	    strncpy(llrsp->phyname, phyname, MON_PHYLEN);
	}
	else {
	    char *p;
	    llrsp->taid = hton32(ip->transit_area()->id());
	    llrsp->if_addr = hton32(*ip->vl_endpt());
	    p = (char *) &llrsp->taid;
	    sprintf(llrsp->phyname, "%d.%d.%d.%d",
		    (int)p[0], (int)p[1], (int)p[2], (int)p[3]);
	}
	llrsp->phyint = hton32(ip->if_phyint);
	llrsp->a_id = hton32(ip->area()->id());
	llrsp->ls_type = hton32(lsap->lsa_type);
	llrsp->ls_id = hton32(lsap->ls_id());
	llrsp->adv_rtr = hton32(lsap->adv_rtr());
	hdr = BuildLSA(lsap);
	memcpy((llrsp + 1), hdr, lsap->ls_length());
    }

    sys->monitor_response(msg, LLLSA_Response, mlen, conn_id);
}

/* Get information concerning a routing table entry.
 */

void OSPF::rte_stats(class MonMsg *req, int conn_id)

{
    MonRqRte *rtereq;
    InAddr net;
    InMask mask;
    INrte *rte;
    int mlen;
    MonMsg *msg;

    rtereq = &req->body.rtrq;
    net = ntoh32(rtereq->net);
    mask = ntoh32(rtereq->mask);

    if (req->hdr.exact != 0)
        rte = inrttbl->find(net, mask);
    else {
        INiterator iter(inrttbl);
	iter.seek(net, mask);
	do {
	    rte = iter.nextrte();
	} while (rte && !rte->valid());
    }

    mlen = sizeof(MonHdr) + sizeof(RteRsp);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = req->hdr.exact;
    msg->hdr.id = req->hdr.id;

    if (rte && rte->valid()) {
        RteRsp *rtersp;
	int n;
	extern char *rtt_ascii[];
	msg->hdr.retcode = 0;
	rtersp = &msg->body.rtersp;
	rtersp->net = hton32(rte->net());
	rtersp->mask = hton32(rte->mask());
	strncpy(rtersp->type, rtt_ascii[rte->type()], MON_RTYPELEN);
	if (rte->intra_AS() || rte->t2cost == Infinity) {
	    rtersp->cost = hton32(rte->cost);
	    rtersp->o_cost = 0;
	}
	else {
	    rtersp->cost = hton32(rte->t2cost);
	    rtersp->o_cost = hton32(rte->cost);
	}
	rtersp->tag = hton32(rte->tag);
	n = rte->r_mpath ? rte->r_mpath->npaths : 0;
	rtersp->npaths = hton32(n);
	for (int i = 0; i < n; i++) {
	    char *phyname;
	    phyname = sys->phyname(rte->r_mpath->NHs[i].phyint);
	    rtersp->hops[i].phyname[0] = '\0';
	    if (phyname)
	        strncpy(rtersp->hops[i].phyname, phyname, MON_PHYLEN);
	    rtersp->hops[i].gw = hton32(rte->r_mpath->NHs[i].gw);
	}
    }

    sys->monitor_response(msg, Rte_Response, mlen, conn_id);
}

/* Respond to a query to get the next Opaque-LSA.
 */

void OSPF::opq_stats(class MonMsg *req, int conn_id)

{
    aid_t a_id;
    int phyint;
    InAddr if_addr;
    LShdr *hdr;
    int mlen;
    MonMsg *msg;
    bool success;

    success = get_next_opq(conn_id, phyint, if_addr, a_id, hdr);

    mlen = sizeof(MonHdr) + sizeof(OpqRsp);
    if (success)
        mlen += ntoh16(hdr->ls_length);
    msg = get_monbuf(mlen);
    msg->hdr.version = OSPF_MON_VERSION;
    msg->hdr.retcode = 1;
    msg->hdr.exact = 1;
    msg->hdr.id = req->hdr.id;

    if (success) {
        OpqRsp *opqrsp;
	msg->hdr.retcode = 0;
	opqrsp = &msg->body.opqrsp;
	opqrsp->phyint = hton32(phyint);
	opqrsp->if_addr = hton32(if_addr);
	opqrsp->a_id = hton32(a_id);
	memcpy((opqrsp + 1), hdr, ntoh16(hdr->ls_length));
    }

    sys->monitor_response(msg, OpqLSA_Response, mlen, conn_id);
}

/* Utility routines used to "get" and "get-next".
 */

/* Find the next interface data structure given the
 * previous IP address and its physical interface.
 * Only used for real interfaces, and not virtual links.
 */

SpfIfc  *OSPF::next_ifc(uns32 xaddr, int phy)

{
    IfcIterator iter(this);
    SpfIfc *ip;
    SpfIfc *best;

    printf("ifcs = %p\n", ifcs);

    best = 0;
    while ((ip = iter.get_next())) {
	printf("next_ifc, ip=%p\n", ip);
	if (ip->is_virtual())
	    continue;
	if (phy > ip->if_phyint)
	    continue;
	if (phy == ip->if_phyint && xaddr >= ip->if_addr)
	    continue;
	if (best) {
	    if (best->if_phyint < ip->if_phyint)
	        continue;
	    if (best->if_phyint == ip->if_phyint &&
		best->if_addr <  ip->if_addr)
	        continue;
	}
	// New best
	best = ip;
    }

    return(best);
}

/* Find a virtual link based on its transit area and
 * endpoint.
 */

SpfIfc *OSPF::find_vl(aid_t transit_id, rtid_t endpt)

{
    SpfArea *tap;
    RTRrte *rte;

    if (!(tap = FindArea(transit_id)))
	return(0);
    if (!(rte = tap->find_abr(endpt)))
	return(0);
    return(rte->VL);
}

/* Find the next virtual link based on the previous virtual
 * link's transit area and endpoint.
 */

SpfIfc *OSPF::next_vl(aid_t transit_id, rtid_t endpt)

{
    AreaIterator aiter(this);
    SpfArea *tap;
    RTRrte *rte;
    VLIfc *vl;
    VLIfc *best;

    best = 0;
    while ((tap = aiter.get_next())) {
        AVLsearch avls(&tap->abr_tbl);
        if (transit_id > tap->a_id)
	    continue;
	while ((rte = (RTRrte *)avls.next())) {
	    if (!(vl = rte->VL))
	        continue;
	    if (transit_id == tap->a_id && endpt >= vl->if_nbrid)
		continue;
	    if (best) {
	        if (best->if_tap->a_id < vl->if_tap->a_id)
	            continue;
		if (best->if_tap->a_id == vl->if_tap->a_id &&
		    best->if_nbrid <  vl->if_nbrid)
		    continue;
	    }
	    // New best
	    best = vl;
	}
    }   
    return(best);
}

/* Find a neighbor based on its physical interface and
 * IP address.
 * Used for monitoring purposes only, and only for
 * non-virtual neighbors.
 */

SpfNbr *OSPF::find_nbr(InAddr nbr_addr, int phy)

{
    IfcIterator iter(this);
    SpfIfc *ip;
    SpfNbr *np;

    while ((ip = iter.get_next())) {
	NbrIterator niter(ip);
	if (ip->is_virtual())
	    continue;
	if (phy != ip->if_phyint)
	    continue;
	while ((np = niter.get_next())) {
	    if (np->n_addr == nbr_addr)
	        return(np);
	}
    }

    return(0);
}

/* Find the next neighbor based on the previous neighbor's
 * physical interface and IP address.
 * Used for monitoring purposes only, and only for
 * non-virtual neighbors.
 */

SpfNbr *OSPF::next_nbr(InAddr nbr_addr, int phy)

{
    IfcIterator iter(this);
    SpfIfc *ip;
    SpfNbr *np;
    SpfNbr *best;

    best = 0;
    while ((ip = iter.get_next())) {
	NbrIterator niter(ip);
	if (ip->is_virtual())
	    continue;
	if (phy > ip->if_phyint)
	    continue;
	while ((np = niter.get_next())) {
	    if (phy == ip->if_phyint && nbr_addr >= np->n_addr)
	        continue;
	    if (best) {
	        if (best->n_ifp->if_phyint < ip->if_phyint)
	            continue;
	        if (best->n_ifp->if_phyint == ip->if_phyint &&
		    best->n_addr < np->n_addr)
	            continue;
	    }
 	    // New best
	    best = np;
	}
    }

    return(best);
}
