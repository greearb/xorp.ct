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

/* Implementation of the LSA base class, which is the internal
 * representation of OSPF link state advertisements.
 */

#include "ospfinc.h"


/* Constructor for an LSA. Always called with a link-state-header.
 * If the body length is non-zero, the body of the link
 * state advertisement (directly succeeding the link state header)
 * is also copied into the LSA.
 */

LSA::LSA(SpfIfc *ip, SpfArea *ap, LShdr *lshdr, int blen)
: AVLitem(ntoh32(lshdr->ls_id), ntoh32(lshdr->ls_org)),
  lsa_type(lshdr->ls_type)

{
    AVLtree *btree;

    hdr_parse(lshdr);
    lsa_body = 0;
    lsa_ifp = ip;
    lsa_ap = ap;
    lsa_agefwd = 0;
    lsa_agerv = 0;
    lsa_agebin = 0;
    lsa_rxmt = 0;

    // Reset flags
    in_agebin = false;
    deferring = false;
    changed = true;
    exception = false;
    rollover = false;
    e_bit = false;
    parsed = false;
    sent_reply = false;
    checkage = false;
    min_failed = false;
    we_orig = false;

    // Fake LSAs aren't install in database
    if (blen) {
	// Add to per-type AVL tree
	btree = ospf->FindLSdb(ip, ap, lsa_type);
	btree->add((AVLitem *) this);
	if (flooding_scope(lsa_type) != GlobalScope)
	    source = ap->add_abr(adv_rtr());
	else
	    source = ospf->add_asbr(adv_rtr());
    }
}

/* Destructor for an LSA. Has already been removed from
 * the database and age bins. Need only delete the
 * appended body, if any.
 */

LSA::~LSA()

{
    delete [] lsa_body;
}

/* Null base functions for the build, parse, and unparse
 * functions, which are overriden by most derived classes.
 */

void LSA::parse(LShdr *)
{
}
void LSA::unparse()
{
}
void LSA::build(LShdr *)
{
}
void LSA::delete_actions()
{
}
RTE *LSA::rtentry()
{
    return(0);
}
bool TNode::is_wild_card()
{
    return(false);
}
RTE *rteLSA::rtentry()
{
    return(rte);
}
bool rtrLSA::is_wild_card()
{
    return((rtype & RTYPE_W) != 0);
}
RTE *asbrLSA::rtentry()
{
    return(asbr);
}

/* Constructor for the LSAs used in the Dijkstra calculation:
 * router-LSAs and network-LSAs.
 */

TNode::TNode(class SpfArea *ap, LShdr *lshdr, int blen)
: LSA(0, ap, lshdr, blen), PriQElt()

{
    t_links = 0;
    dijk_run = ospf->n_dijkstras & 1;
    t_state = DS_UNINIT;
    t_mpath = 0;
    in_mospf_cache = false;
}

/* Constructor for stub LSAs (summary-LSAs and AS external
 * LSAs). These are processed at the end of the routing
 * table calculation.
 */

rteLSA::rteLSA(SpfArea *ap, LShdr *hdr, int blen) : LSA(0, ap, hdr, blen)

{
    rte = 0;
    orig_rte = 0;
}

/* Parse a received link-state header, and convert it to machine
 * format for storage within an LSA class. Called by the LSA
 * constructor, and laso by AddLSA() when updating an LSA
 * in place.
 *
 * LS type, Link State ID and Advertising router are not handled by
 * this routine, as they are instead dealt with diecrtly by the
 * LSA constructor. 
 */

void LSA::hdr_parse(LShdr *lshdr)

{
    lsa_rcvage = ntoh16(lshdr->ls_age);
    if ((lsa_rcvage & ~DoNotAge) >= MaxAge)
	lsa_rcvage = MaxAge;
    lsa_opts = lshdr->ls_opts;
    lsa_seqno = (seq_t) ntoh32((uns32) lshdr->ls_seqno);
    lsa_xsum = ntoh16(lshdr->ls_xsum);
    lsa_length = ntoh16(lshdr->ls_length);
}

/* Copy the link state header of a parsed, database copy
 * into a network-byte order link state header for transmission
 * in Link State Updates, Database description packets and
 * Link State Acknowledgments.
 */

LShdr& LShdr::operator=(class LSA &lsa)

{
    ls_age = hton16(lsa.lsa_age());
    ls_opts = lsa.lsa_opts;
    ls_type = lsa.lsa_type;
    ls_id = hton32(lsa.ls_id());
    ls_org = hton32(lsa.adv_rtr());
    ls_seqno = (seq_t) hton32((uns32) lsa.ls_seqno());
    ls_xsum = hton16(lsa.lsa_xsum);
    ls_length = hton16(lsa.lsa_length);

    return(*this);
}

/* Build a network-ready LSA from a parsed database copy. If
 * not parsed, simply copy the stored body. Else, call a
 * type-specific build routine.
 * In any case, copy the LSA header into place. making sure
 * to put it into network byte order.
 * Builds LSA in temporary location, which if not big enough gets
 * reallocated. Since there is only one such build area, this
 * routine is not reentrant.
 */

LShdr *OSPF::BuildLSA(LSA *lsap, LShdr *hdr)

{
    int	blen;

    if (hdr == 0) {
    if (lsap->lsa_length > build_size) {
	delete [] build_area;
	build_size = lsap->lsa_length;
	build_area = new byte[lsap->lsa_length];
    }
    hdr = (LShdr *) ospf->build_area;
    }
    // Fill in standard Link state header
    *hdr = *lsap;
    // Fill in body of LSA
    if (!lsap->exception)
	lsap->build(hdr);
    else {
	blen = lsap->lsa_length - sizeof(LShdr);
	memcpy((hdr + 1), lsap->lsa_body, blen);
    }
    
    return(hdr);
}

