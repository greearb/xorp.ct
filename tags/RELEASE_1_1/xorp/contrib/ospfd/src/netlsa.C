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

/* Routines dealing with network-LSAs: originating, parsing and
 * unparsing. Also routine to refresh a network-LSA, or to
 * flush an old one when, for example, an interface has been
 * deleted or its area has changed.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"

/* Originate a network-LSA. Must be the Designated Router for the
 * network, and be fully adjacent to at least one other
 * router.
 * if_nfull includes all those neighbors that we
 * are currently helping through hitless restart.
 */

void SpfIfc::nl_orig(int forced)

{
    LSA *olsap;
    LShdr *hdr;

    // If in hitless restart, queue exit check
    if (ospf->in_hitless_restart())
        ospf->check_htl_termination = true;

    olsap = ospf->myLSA(0, if_area, LST_NET, if_addr);
    hdr = nl_raw_orig();

    if (hdr == 0)
	lsa_flush(olsap);
    else {
        seq_t seqno;
	seqno = ospf->ospf_get_seqno(LST_NET, olsap, forced);
	if (seqno != InvalidLSSeq) {
	    // Fill in rest of LSA contents
	    hdr->ls_seqno = hton32(seqno);
	    (void) ospf->lsa_reorig(0, if_area, olsap, hdr, forced);
	}
	ospf->free_orig_buffer(hdr);
    }
}

/* Build the contents of the network-LSA that should be
 * originated for the interface. returning a pointer to the
 * link-state header (built in a static memory space). If no
 * network-LSA should be originated, 0 is returned.
 * LS Sequence Number must be filled in by the caller, who also
 * must call OSPF::free_orig_buffer() when done with the
 * constructed LSA.
 */

LShdr *SpfIfc::nl_raw_orig()

{
	LShdr *hdr;
	NetLShdr *nethdr;
	uns16 length;
	rtid_t *nbr_ids;
	NbrIterator iter(this);
	SpfNbr *np;

    if (if_state != IFS_DR)
	return(0);
    else if (if_nfull == 0)
	return(0);

	// Build new LSA
	length = sizeof(NetLShdr) + (if_nfull+1)*sizeof(rtid_t);
	length += sizeof(LShdr);
	// Fill in LSA contents
    hdr = ospf->orig_buffer(length);
	hdr->ls_opts = SPO_DC;
	if (!if_area->a_stub)
	    hdr->ls_opts |= SPO_EXT;
	if (mospf_enabled())
	    hdr->ls_opts |= SPO_MC;
	hdr->ls_type = LST_NET;
	hdr->ls_id = hton32(if_addr);
	hdr->ls_org = hton32(ospf->my_id());
	hdr->ls_length = hton16(length);
	// Body
	nethdr = (NetLShdr *) (hdr + 1);
	nethdr->netmask = hton32(if_mask);
	// Fill in fully adjacent neighbors
	nbr_ids = (rtid_t *) (nethdr + 1);
	*nbr_ids = hton32(ospf->my_id());
	while ((np = iter.get_next()) != 0) {
	if (np->adv_as_full())
		*(++nbr_ids) = hton32(np->id());
	}
    return(hdr);
}

/* Constructor for a network-LSA (internal representation).
 * Simply call the generic LSA constructor.
 * All the work is performed by the parse() routine.
 */

netLSA::netLSA(SpfArea *ap, LShdr *hdr, int blen) : TNode(ap, hdr, blen)

{
}

/* Reoriginate a network-LSA. Find the correct interface, and
 * force the network-LSA to be rebuilt.
 */

void netLSA::reoriginate(int forced)

{
    SpfIfc *ip;

    if ((ip = ospf->find_ifc(ls_id())) == 0 ||
	ip->area() != lsa_ap)
	lsa_flush(this);
    else
	ip->nl_orig(forced);
}

/* Parse a network-LSA. Calculate a routing table entry for the net/mask
 * described by the network-LSA, and then install pointers for
 * each of the links to routers within the body of
 * the LSA.
 */

void netLSA::parse(LShdr *hdr)

{
    uns32 net;
    uns32 mask;
    NetLShdr *nethdr;
    int len;
    rtid_t *idp;
    Link *lp;
    TLink **tlpp;
    TLink *tlp;
    Link *nextl;
    
    nethdr = (NetLShdr *) (hdr + 1);
    mask = ntoh32(nethdr->netmask);
    net = ls_id() & mask;

    if (!(t_dest = inrttbl->add(net, mask)))
	exception = true;

    tlpp = (TLink **) &t_links;

    // Install pointers to router-LSAs iff bidirectional
    len = ntoh16(hdr->ls_length);
    len -= sizeof(LShdr);
    len -= sizeof(NetLShdr);
    idp = (rtid_t *) (nethdr + 1);
    for (; len >= (int) sizeof(rtid_t); tlpp = (TLink **)&tlp->l_next) {
	rtid_t rtid;
	rtid = ntoh32(*idp);
	// Allocate link. if necessary
	if (!(tlp = *tlpp)) {
	    tlp = new TLink;
	    // Link into list
	    *tlpp = tlp;
	}
	// Fill in transit link parameters
	tlp->tl_nbr = 0;
	tlp->l_ltype = LT_PP;	// Router on other end
	tlp->l_id = rtid;
	tlp->l_fwdcst = 0;
	tlp->tl_rvcst = MAX_COST;
	// Link into database
	tlp_link(tlp);
	// Progress to next link
	len -= sizeof(rtid_t);

	idp++;
    }

    // Have to save LShdr?
    if (len != 0)
	exception = true;

    // Clean up unused transit links
    lp = *tlpp;
    // Zero terminate list
    *tlpp = 0;
    for (; lp; lp = nextl) {
	nextl = lp->l_next;
	delete lp;
    }
}

/* Unparse a network-LSA. Reset all the transit link pointers,
 * but don't free and transit links, because they might be
 * used later when the network-LSA is parsed again. If they
 * should be freed, this will happen either in the parse or
 * the LSA free routine.
 */ 

void netLSA::unparse()

{
    // Reset routing table pointer
    t_dest = 0;

    // Unlink neighbor LSAs
    unlink();
}

/* When removing a network-LSA you must
 * reparse and network-LSA having the same Link State ID.
 * This fixes and incomplete reference made by the OSPF
 * Dijkstra calculation.
 */

void netLSA::delete_actions()

{
    netLSA *olsap;
    AVLtree *tree;

    // Reparse any other network-LSA with same Link State ID
    tree = ospf->FindLSdb(0, lsa_ap, LST_NET);
    olsap = (netLSA *) tree->previous(ls_id()+1, 0);
    while (olsap) {
	LShdr *hdr;
	if (olsap->ls_id() != ls_id())
	    break;
	else if (olsap != this) {
	    hdr = ospf->BuildLSA(olsap);
	    olsap->parse(hdr);
	    break;
	}
	olsap = (netLSA *) tree->previous(ls_id(), adv_rtr());
    }
}

/* Build a network-LSA in network format, based on the internal
 * parsed version. Only called if "exception" not set, meaning that
 * there was nothing unusual about the network-LSA.
 * Link state header, including length, has already been filled
 * in by the caller.
 */

void netLSA::build(LShdr *hdr)

{
    rtid_t *idp;
    NetLShdr *nethdr;
    Link *lp;
    
    nethdr = (NetLShdr *) (hdr + 1);
    // Fill in mask
    nethdr->netmask = hton32(t_dest->index2());

    // Fill in list of routers
    idp = (rtid_t *) (nethdr + 1);
    for (lp = t_links; lp; lp = lp->l_next, idp++)
	*idp = hton32(lp->l_id);
}
