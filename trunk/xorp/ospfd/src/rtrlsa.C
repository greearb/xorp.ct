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

/* Routines dealing with the origination, parsing and unparsing
 * of router-LSAs.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"


/* Constructor for a router-LSA.
 */

rtrLSA::rtrLSA(SpfArea *a, LShdr *hdr, int blen) : TNode(a, hdr, blen)

{
    rtype = 0;
}

/* Maximum number of bytes added to a router-LSA by a
 * single interface. Overloaded when interface type is
 * point-to-point or point-to-multipoint.
 */
int SpfIfc::rl_size()

{
    return(sizeof(RtrLink));
}

// Insert information concerning a virtual link into the
// router-LSA

RtrLink *VLIfc::rl_insert(RTRhdr *rtrhdr, RtrLink *rlp)

{
    SpfNbr *np;

    if ((np = if_nlst) && np->adv_as_full()) {
	rlp->link_id = hton32(np->id());
	rlp->link_data = hton32(if_addr);
	rlp->link_type = LT_VL;
	rlp->n_tos = 0;
	rlp->metric = hton16(if_cost);
	if_area->add_to_ifmap(this);
	rlp++;
	rtrhdr->nlinks++;
    }
    return(rlp);
}

/* Insert information concerning a broadcast or NBMA interface
 * into a router-LSA
 * if_nfull includes all those neighbors that we
 * are currently helping through hitless restart.
 */

RtrLink *DRIfc::rl_insert(RTRhdr *rtrhdr, RtrLink *rlp)

{
    if ((if_state == IFS_DR && if_nfull > 0) ||
	((if_state == IFS_BACKUP || if_state == IFS_OTHER) &&
	 (if_dr_p && if_dr_p->adv_as_full()))) {
	rlp->link_id = hton32(if_dr);
	rlp->link_data = hton32(if_addr);
	rlp->link_type = LT_TNET;
	rlp->n_tos = 0;
	rlp->metric = hton16(if_cost);
	if_area->add_to_ifmap(this);
	rlp++;
	rtrhdr->nlinks++;
    }
    else {
	rlp->link_id = hton32(if_net);
	rlp->link_data = hton32(if_mask);
	rlp->link_type = LT_STUB;
	rlp->n_tos = 0;
	rlp->metric = hton16(if_cost);
	if_area->add_to_ifmap(this);
	rlp++;
	rtrhdr->nlinks++;
    }
    return(rlp);
}

// Maximum size added by point-to-point link
int PPIfc::rl_size()

{
    return(2 * sizeof(RtrLink));
}

/* How we advertise a point-to-point link's addresses depends
 * on the interface mask. If set to all ones (0xffffffff) or 0, we
 * do the traditional thing of advertising the neighbor's
 * address. If instead the mask is set to something else, we
 * advertise a route to the entire subnet.
 */

RtrLink *PPIfc::rl_insert(RTRhdr *rtrhdr, RtrLink *rlp)

{
    SpfNbr *np;

    if ((np = if_nlst) && np->adv_as_full()) {
	PPAdjAggr *adjaggr;
	uns16 adv_cost;
	adv_cost = if_cost;
	adjaggr = (PPAdjAggr *)if_area->AdjAggr.find(np->id(), 0);
	if (adjaggr && adjaggr->first_full) {
	    if (adjaggr->first_full != this)
	        goto adv_stub;
	    adv_cost = adjaggr->nbr_cost;
	}
	rlp->link_id = hton32(np->id());
	rlp->link_data = hton32(unnumbered() ? if_IfIndex : if_addr);
	rlp->link_type = LT_PP;
	rlp->n_tos = 0;
	rlp->metric = hton16(adv_cost);
	if_area->add_to_ifmap(this);
	rlp++;
	rtrhdr->nlinks++;
    }

  adv_stub: // Advertise stub link to neighbor's IP address

    if (state() == IFS_PP && !unnumbered() &&
	(np || (if_mask != 0xffffffffL && if_mask != 0))) {
        if (if_mask != 0xffffffffL && if_mask != 0) {
	    rlp->link_id = hton32(if_net);
	    rlp->link_data = hton32(if_mask);
	}
	else {
	rlp->link_id = hton32(np->addr());
	rlp->link_data = hton32(0xffffffffL);
	}
	rlp->link_type = LT_STUB;
	rlp->n_tos = 0;
	rlp->metric = hton16(if_cost);
	if_area->add_to_ifmap(this);
	rlp++;
	rtrhdr->nlinks++;
    }
    return(rlp);
}

// Maximum size added by Point-to-MultiPoint interface
int P2mPIfc::rl_size()

{
    return((if_nnbrs+1) * sizeof(RtrLink));
}

RtrLink *P2mPIfc::rl_insert(RTRhdr *rtrhdr, RtrLink *rlp)

{
    NbrIterator iter(this);
    SpfNbr *np;

    // Add stub link for interface address
    rlp->link_id = hton32(if_addr);
    rlp->link_data = hton32(0xffffffffL);
    rlp->link_type = LT_STUB;
    rlp->n_tos = 0;
    rlp->metric = hton16(0);
    if_area->add_to_ifmap(this);
    rlp++;
    rtrhdr->nlinks++;

    // Then add link for each FULL neighbor
    while ((np = iter.get_next())) {
	if (!np->adv_as_full())
	    continue;
	rlp->link_id = hton32(np->id());
	rlp->link_data = hton32(if_addr);
	rlp->link_type = LT_PP;
	rlp->n_tos = 0;
	rlp->metric = hton16(if_cost);
	if_area->add_to_ifmap(this);
	rlp++;
	rtrhdr->nlinks++;
    }

    return(rlp);
}

/* Build the router-LSAs of all areas at once. This is
 * done when, for example, our area border router or
 * ASBR status changes. We laso check whether we now
 * want to originate default routes into stub areas,
 * as that logic also needs to be performed when area
 * border router status changes.
 */

void OSPF::rl_orig()

{
    AreaIterator iter(this);
    SpfArea *ap;

    while ((ap = iter.get_next())) {
	ap->rl_orig(false);
	if (ap->a_stub)
	    ap->sl_orig(default_route);
    }
}

/* Build a router LSA. First estimate how big the router-LSA
 * will be, to verify that we have enough space in the
 * temporary build area.
 *
 * Keep a map of links in our own router-LSA to interfaces (ifmap),
 * for later use in the Dijkstra calculation.
 */

void SpfArea::rl_orig(int forced)

{
    LSA *olsap;
    LShdr *hdr;
    RTRhdr *rtrhdr;
    int maxlen;
    int maxifc;
    int length;
    SpfIfc *ip;
    IfcIterator iiter(this);
    seq_t seqno;
    RtrLink *rlp;

    // If in hitless restart, queue exit check
    if (ospf->in_hitless_restart())
        ospf->check_htl_termination = true;

    // Current LSA in database
    olsap = ospf->myLSA(0, this, LST_RTR, ospf->my_id());

    // Calculate maximum size for LSA
    maxlen = sizeof(LShdr) + sizeof(RTRhdr);
    while ((ip = iiter.get_next()))
	maxlen += ip->rl_size();
    // Add in host records
    maxlen += hosts.size() * sizeof(RtrLink);
    // If first active area, advertise orphaned hosts
    if (this == ospf->first_area) {
        AreaIterator aiter(ospf);
	SpfArea *ap;
	while ((ap = aiter.get_next())) {
	    if (ap->n_active_if == 0)
	        maxlen += ap->hosts.size() * sizeof(RtrLink);
	}
    }
    // Get sequence number to originate with
    seqno = ospf->ospf_get_seqno(LST_RTR, olsap, forced);
    if (seqno == InvalidLSSeq)
	return;
    // Convert bytes to number of links, to make sure
    // we have enough room to store interface map
    maxifc = maxlen/sizeof(RtrLink);
    if (maxifc > sz_ifmap) {
	delete [] ifmap;
	ifmap = new SpfIfc* [maxifc];
	sz_ifmap = maxifc;
    }

    // Start with empty interface map
    n_ifmap = 0;
    ifmap_valid = true;

    // Build LSA header
    hdr = ospf->orig_buffer(maxlen);
    hdr->ls_opts = SPO_DC;
    if (!a_stub)
	hdr->ls_opts |= SPO_EXT;
    if (ospf->mospf_enabled())
	hdr->ls_opts |= SPO_MC;
    hdr->ls_type = LST_RTR;
    hdr->ls_id = hton32(ospf->my_id());
    hdr->ls_org = hton32(ospf->my_id());
    hdr->ls_seqno = (seq_t) hton32((uns32) seqno);
    // Router-LSA specific portion
    rtrhdr = (RTRhdr *) (hdr+1);
    rtrhdr->rtype = 0;
    if (ospf->n_area > 1)
	rtrhdr->rtype |= RTYPE_B;
    if (ospf->n_extImports != 0)
	rtrhdr->rtype |= RTYPE_E;
    if (n_VLs > 0)
	rtrhdr->rtype |= RTYPE_V;
    if (ospf->mospf_enabled()) {
	if (a_id != 0 && ospf->mc_abr())
	    rtrhdr->rtype |= RTYPE_W;
	else if (ospf->inter_AS_mc)
	    rtrhdr->rtype |= RTYPE_W;
    }
    rtrhdr->zero = 0;
    rtrhdr->nlinks = 0;

    // Build body of router-LSA
    rlp = (RtrLink *) (rtrhdr+1);
    iiter.reset();
    while ((ip = iiter.get_next())) {
	if (ip->state() == IFS_DOWN)
	    continue;
	else if (ip->state() == IFS_LOOP) {
	    rlp->link_id = hton32(ip->if_addr);
	    rlp->link_data = hton32(0xffffffffL);
	    rlp->link_type = LT_STUB;
	    rlp->n_tos = 0;
	    rlp->metric = hton16(ip->cost());
	    add_to_ifmap(ip);
	    rlp++;
	    rtrhdr->nlinks++;
	}
	else
	    rlp = ip->rl_insert(rtrhdr, rlp);
    }

    // Add area's host routes
    if (n_active_if != 0)
        rlp = rl_insert_hosts(this, rtrhdr, rlp);

    /* If no active interfaces to area, just flush
     * LSA. Host addresses will get added to other areas
     * automatically.
     */
    if (rtrhdr->nlinks == 0) {
        lsa_flush(olsap);
	delete [] ifmap;
	ifmap = 0;
	sz_ifmap = 0;
	ospf->free_orig_buffer(hdr);
	return;
    }

    // If first active area, advertise orphaned hosts
    if (this == ospf->first_area) {
        AreaIterator aiter(ospf);
	SpfArea *ap;
	while ((ap = aiter.get_next())) {
	    if (ap->n_active_if == 0)
	        rlp = ap->rl_insert_hosts(this, rtrhdr, rlp);
	}
    }

    // Finish (re)origination
    length = ((byte *) rlp) - ((byte *) hdr);
    hdr->ls_length = hton16(length);
    rtrhdr->nlinks = hton16(rtrhdr->nlinks);
    /* If we can't originate for some reason, we will need
     * to delete the interface map so that the routing
     * calculation won't get confused during initialization.
     * This situation only occurs during hitless restart.
     */
    if (ospf->lsa_reorig(0, this, olsap, hdr, forced) == 0 &&
	olsap &&
	ospf->in_hitless_restart()) {
        LShdr *dbcopy;
	int size;
	RTRhdr *rhdr;	
	RTRhdr *dbrhdr;
	rhdr = (RTRhdr *)(hdr + 1);
	dbcopy = ospf->BuildLSA(olsap);
	dbrhdr = (RTRhdr *)(dbcopy + 1);
	// Compare body of advertisements
	size = olsap->lsa_length - sizeof(LShdr) - sizeof(RTRhdr);
	if (hdr->ls_length != dbcopy->ls_length ||
	    memcmp((rhdr + 1), (dbrhdr + 1), size) != 0) {
	    delete [] ifmap;
	    ifmap = 0;
	    sz_ifmap = 0;
	}
	else
	    ospf->full_sched = true;
    }

    ospf->free_orig_buffer(hdr);
}

/* Add an area's configured hosts to a router-LSA.
 */

RtrLink *SpfArea::rl_insert_hosts(SpfArea *home, RTRhdr *rtrhdr, RtrLink *rlp)

{
    AVLsearch h_iter(&hosts);	
    HostAddr *hp;

    // Add a stub link for each host record in the area
    while ((hp = (HostAddr *)h_iter.next())) {
	rlp->link_id = hton32(hp->r_rte->net());
	rlp->link_data = hton32(hp->r_rte->mask());
	rlp->link_type = LT_STUB;
	rlp->n_tos = 0;
	rlp->metric = hton16(hp->r_cost);
	home->add_to_ifmap(hp->ip);
	rlp++;
	rtrhdr->nlinks++;
    }

    return(rlp);
}

/* Add to the interface map, so that next hops can be easily
 * calculated during Dijkstra.
 */

void SpfArea::add_to_ifmap(SpfIfc *ip)

{
    ifmap[n_ifmap++] = ip;
}


/* When an interface is deleted, it must be removed
 * from the interface map - it may be up to 5 seconds
 * before the router-LSA is updated and the interface
 * map along with it.
 */

void OSPF::delete_from_ifmap(SpfIfc *ip)

{
    AreaIterator iter(this);
    SpfArea *ap;

    while ((ap = iter.get_next())) {
	int i;
	for (i= 0; i < ap->n_ifmap; i++)
            if (ap->ifmap[i] == ip) {
	        ap->ifmap[i] = 0;
		ap->ifmap_valid = false;
	    }
    }
}


/* Reoriginate a router-LSA. Force a router-LSA to be
 * rebuilt for the given area.
 */

void rtrLSA::reoriginate(int forced)

{
    lsa_ap->rl_orig(forced);
}

/* Parse a received router-LSA.
 * Can be called repeatedly without an intervening unparse,
 * which is why we don't check the parsed bit.
 */

void rtrLSA::parse(LShdr *hdr)

{
    RTRhdr  *rhdr;
    RtrLink *rtlp;
    Link *lp;
    Link **lpp;
    byte *end;
    int i;
    Link *nextl;
    TOSmetric *mp;
    
    rhdr = (RTRhdr *) (hdr+1);
    rtlp = (RtrLink *) (rhdr+1);
    n_links = ntoh16(rhdr->nlinks);
    end = ((byte *) hdr) + ntoh16(hdr->ls_length);

    rtype = rhdr->rtype;
    t_dest = lsa_ap->add_abr(ls_id());
    // Virtual link address calculation might change
    t_dest->changed = true;
    if (rhdr->zero != 0)
	exception = true;

    lpp = &t_links;
    for (i = 0; i < n_links; i++) {
	if (((byte *) rtlp) > end) {
	    exception = true;
	    break;
	}
	// (re)allocate link. if necessary
	if ((!(lp = *lpp)) || lp->l_ltype != rtlp->link_type) {
	    Link *nextl;
	    nextl = lp;
	    if (rtlp->link_type == LT_STUB)
		lp = new SLink;
	    else
		lp = new TLink;
	    // Link into list
	    *lpp = lp;
	    lp->l_next = nextl;
	}
	// Fill in link parameters
	lp->l_ltype = rtlp->link_type;
	lp->l_id = ntoh32(rtlp->link_id);
	lp->l_fwdcst = ntoh16(rtlp->metric);
	lp->l_data = ntoh32(rtlp->link_data);
	switch (rtlp->link_type) {
	    INrte *srte;
	    TLink *tlp;
	  case LT_STUB:
	    srte = inrttbl->add(lp->l_id, lp->l_data);
	    ((SLink *) lp)->sl_rte = srte;
	    break;
	  case LT_PP:
	  case LT_TNET:
	  case LT_VL:
	    tlp = (TLink *) lp;
	    tlp->tl_nbr = 0;
	    tlp->tl_rvcst = MAX_COST;
	    // Link into database
	    tlp_link(tlp);
	    break;
	  default:
	    break;
	}
	// If TOS metrics, must keep unparsed form
	if (rtlp->n_tos)
	    exception = true;
	// Advance link pointer
	lpp = &lp->l_next;
	// Step over non-zero TOS metrics
	mp = (TOSmetric *)(rtlp+1);
	mp += rtlp->n_tos;
	rtlp = (RtrLink *)mp;
    }

    // Need to keep LSA, or rebuild from parsed copy?
    if (((byte *) rtlp) != end)
	exception = true;

    // Clean up unused transit links
    lp = *lpp;
    // Null terminate list
    *lpp = 0;
    for (; lp; lp = nextl) {
	nextl = lp->l_next;
	delete lp;
    }
}

/* Link a transit link into the link-state database by setting
 * pointers to and from neighboring LSAs.
 */

void TNode::tlp_link(TLink *tlp)

{
    TNode *nbr;
    uns32 nbr_id;
    Link *nlp;

    nbr_id = tlp->l_id;

    if (tlp->l_ltype == LT_TNET) {
	nbr = (TNode *) lsa_ap->netLSAs.previous(nbr_id+1);
	if (nbr == 0 || nbr->ls_id() != nbr_id)
	    return;
    }
    else if (!(nbr = (TNode *) lsa_ap->rtrLSAs.find(nbr_id, nbr_id)))
	return;

    // Neighbor must be parsed also
    if (!nbr->parsed)
	return;
    // Virtual link address calculation might change
    if (nbr->ls_type() == LST_RTR)
        nbr->t_dest->changed = true;
    // Check for bidirectionality
    for (nlp = nbr->t_links; nlp; nlp = nlp->l_next) {
	TLink *ntlp;
	if (nlp->l_ltype == LT_STUB)
	    continue;
	if (nlp->l_id != ls_id())
	    continue;
	if (nlp->l_ltype == LT_TNET && lsa_type != LST_NET)
	    continue;
	if (nlp->l_ltype != LT_TNET && lsa_type == LST_NET)
	    continue;
	// Fill in link pointers
	ntlp = (TLink *) nlp;
	tlp->tl_nbr = nbr;
	ntlp->tl_nbr = this;
	if (ntlp->l_fwdcst < tlp->tl_rvcst)
	    tlp->tl_rvcst = ntlp->l_fwdcst;
	if (tlp->l_fwdcst < ntlp->tl_rvcst)
	    ntlp->tl_rvcst = tlp->l_fwdcst;
    }
}

/* Unparse a router-LSA. Simply unlink it from its neighbors
 * in the link-state database.
 */

void rtrLSA::unparse()

{
    unlink();
}

/* Unlink a transit node (router or network-LSA) from its
 * neighbors.
 */

void TNode::unlink()

{
    Link *lp;

    for (lp = t_links; lp; lp = lp->l_next) {
	TLink *tlp;
	TNode *nbr;
	Link *nlp;
	if (lp->l_ltype == LT_STUB)
	    continue;
	tlp = (TLink *) lp;
	// Reset neighbor pointers
	if (!(nbr = tlp->tl_nbr))
	    continue;
	// Virtual link address calculation might change
	if (nbr->ls_type() == LST_RTR)
	    nbr->t_dest->changed = true;
	for (nlp = nbr->t_links; nlp; nlp = nlp->l_next) {
	    TLink *ntlp;
	    if (nlp->l_ltype == LT_STUB)
		continue;
	    ntlp = (TLink *) nlp;
	    if (ntlp->l_id != ls_id())
		continue;
	    if (ntlp->l_ltype == LT_TNET && lsa_type != LST_NET)
		continue;
	    if (ntlp->l_ltype != LT_TNET && lsa_type == LST_NET)
		continue;
	    // Reset neighbor pointer
	    ntlp->tl_nbr = 0;
	    ntlp->tl_rvcst = MAX_COST;
	}
	// Reset own pointer
	tlp->tl_nbr = 0;
    }
}

/* Destructor for transit nodes. Must return all the transit
 * and stub links to the heap.
 */

TNode::~TNode()

{
    Link *lp;
    Link *nextl;

    for (lp = t_links; lp; lp = nextl) {
	nextl = lp->l_next;
	delete lp;
    }
}

/* Build a router-LSA in network format, based on the internal
 * parsed version. Only called if "exception" not set, meaning that
 * there was nothing unusual about the router-LSA.
 * Link state header, including length, has already been filled
 * in by the caller.
 */

void rtrLSA::build(LShdr *hdr)

{
    RTRhdr *rtrhdr;
    RtrLink *rtrlink;
    Link *lp;
    
    rtrhdr = (RTRhdr *) (hdr + 1);
    // Fill router type
    rtrhdr->rtype = rtype;
    rtrhdr->zero = 0;
    rtrhdr->nlinks = hton16(n_links);

    // Fill in links
    rtrlink = (RtrLink *) (rtrhdr + 1);
    for (lp = t_links; lp; lp = lp->l_next, rtrlink++) {
	rtrlink->link_id = hton32(lp->l_id);;
	rtrlink->link_data = hton32(lp->l_data);
	rtrlink->link_type = lp->l_ltype;
	rtrlink->n_tos = 0;
	rtrlink->metric = hton16(lp->l_fwdcst);
    }
}
