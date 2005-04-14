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

/* Routines dealing with group-membership-LSAs.
 * This includes originating, parsing, and
 * running routing calculations.
 */

#include "ospfinc.h"
#include "phyint.h"
#include "ifcfsm.h"

/* Constructor for a group-membership-LSA.
 */

grpLSA::grpLSA(SpfArea *a, LShdr *hdr, int blen) : LSA(0, a, hdr, blen)

{
}

/* Parse a group-membership-LSA. Just set the exception
 * flag, so that the caller will instead store
 * the body of the LSA.
 */

void grpLSA::parse(LShdr *)

{
    exception = true;
}

/* Unparse a group-LSA. NULL function.
 */

void grpLSA::unparse()

{
}

/* Build a group-membership-LSA. Since the parse function
 * always sets the exception flag, this function is never really
 * called.
 */

void grpLSA::build(LShdr *)

{
}

/* Reoriginate a group-membership-LSA.
 */

void grpLSA::reoriginate(int forced)

{
    lsa_ap->grp_orig(ls_id(), forced);
}

/* Reoriginate the group-membership-LSAs for a given
 * group -- all areas at once.
 */

void OSPF::grp_orig(InAddr group, int forced)

{
    SpfArea *ap;
    AreaIterator aiter(ospf);

    while ((ap = aiter.get_next())) {
	if (ap->n_active_if != 0)
	    ap->grp_orig(group, forced);
    }
}

/* Build the group-membership-LSA for a given multicast group
 * and area. We search through all the group entries, and
 * assume that the phyint of 0 is not used.
 */

void SpfArea::grp_orig(InAddr group, int forced)

{
    LSA *olsap;
    AVLsearch iter(&ospf->local_membership);
    bool added_self;
    int maxlen;
    LShdr *hdr;
    uns16 length;
    seq_t seqno;
    AVLitem *entry;
    GMref *gmref;

    olsap = ospf->myLSA(0, this, LST_GM, group);
    if (!ospf->mospf_enabled()) {
	lsa_flush(olsap);
	return;
    }

    // Estimate size of LSA
    maxlen = sizeof(LShdr) + sizeof(GMref);
    iter.seek(group, 0);
    while ((entry = iter.next()) && entry->index1() == group)
	maxlen += sizeof(GMref);
    // Get LS Sequence Number
    seqno = ospf->ospf_get_seqno(LST_GM, olsap, forced);
    if (seqno == InvalidLSSeq)
	return;
    // Fill in LSA header
    hdr = ospf->orig_buffer(maxlen);
    hdr->ls_opts = SPO_DC | SPO_MC;
    if (!a_stub)
	hdr->ls_opts |= SPO_EXT;
    hdr->ls_type = LST_GM;
    hdr->ls_id = hton32(group);
    hdr->ls_org = hton32(ospf->my_id());
    hdr->ls_seqno = hton32(seqno);
    // Body
    length = sizeof(LShdr);
    added_self = false;
    gmref = (GMref *) (hdr + 1);
    iter.seek(group, 0);
    while ((entry = iter.next()) && entry->index1() == group) {
        PhyInt *phyp;
	int phyint=(int)entry->index2();
	if (phyint != -1) {
	    if (!(phyp = (PhyInt *)ospf->phyints.find((uns32) phyint, 0)))
	        continue;
	    if (!phyp->mospf_ifp)
	        continue;
	    if (phyp->mospf_ifp->area() != this)
	        continue;
	    if (phyp->mospf_ifp->if_nfull > 0 &&
		phyp->mospf_ifp->is_multi_access()) {
	        if (phyp->mospf_ifp->if_state == IFS_DR) {
	            gmref->ls_type = hton32(LST_NET);
		    gmref->ls_id = hton32(phyp->mospf_ifp->if_addr);
		    gmref++;
		    length += sizeof(GMref);
		}
		continue;
	    }
	    // Fall through on stubs and point-to-point links
	}
	// Add self instead
	if (!added_self) {
	    added_self = true;
	    gmref->ls_type = hton32(LST_RTR);
	    gmref->ls_id = hton32(ospf->my_id());
	    gmref++;
	    length += sizeof(GMref);
	}
    }

    // If backbone area, may need to advertise membership from
    // other areas.
    if (!added_self && a_id == BACKBONE) {
        AreaIterator a_iter(ospf);
	SpfArea *a;
	while ((a = a_iter.get_next()) && !added_self) {
	    AVLsearch g_iter(&a->grpLSAs);
	    LSA *glsa;
	    if (a == this)
	        continue;
	    g_iter.seek(group, 0);
	    while ((glsa = (LSA *)g_iter.next()) && glsa->index1() == group) {
	        if (!glsa->parsed)
		    continue;
	        added_self = true;
		gmref->ls_type = hton32(LST_RTR);
		gmref->ls_id = hton32(ospf->my_id());
		gmref++;
		length += sizeof(GMref);
		break;
	    }
	}
    }

    // Empty?
    if (length == sizeof(LShdr))
	lsa_flush(olsap);
    else {
	// Fill in length, then reoriginate
        hdr->ls_length = hton16(length);
	(void) ospf->lsa_reorig(0, this, olsap, hdr, forced);
    }
    ospf->free_orig_buffer(hdr);
}

/* The state of an interface has changed, requiring us
 * to reoriginate all the group-LSAs with members
 * on the interface.
 */

void SpfIfc::reorig_all_grplsas()

{
    PhyInt *phyp;
    AVLsearch iter(&ospf->local_membership);
    AVLitem *entry;

    if (!(phyp = (PhyInt *)ospf->phyints.find((uns32) if_phyint, 0)))
	return;
    if (phyp->mospf_ifp != this)
	return;
    while ((entry = iter.next())) {
        if (entry->index2() == (uns32)if_phyint)
	    if_area->grp_orig(entry->index1(), 0);
    }
}

/* Determine whether there are grpup members directly
 * attached to a transit node, by looking for
 * the appropriate group-membership-LSA referencing
 * the transit node.
 * The body of the group-membership-LSA has not been
 * parsed, and is still in network-byte order.
 */

bool TNode::has_members(InAddr group)

{
    grpLSA *glsa;
    size_t len;
    GMref *vp;

    // Always act as if group members for wild card receivers
    if (is_wild_card())
        return(true);
    // look for group-membership-LSA
    glsa = (grpLSA *) ospf->FindLSA(0, lsa_ap, LST_GM, group, adv_rtr());
    if (!glsa || glsa->lsa_age() == MaxAge)
	return(false);
    // Search for vertex
    len = glsa->ls_length() - sizeof(LShdr);
    vp =  (GMref *) glsa->lsa_body;
    for (; len >= sizeof(*vp); vp++, len -= sizeof(*vp)) {
	if (ntoh32(vp->ls_id) != ls_id())
	    continue;
	if (ntoh32(vp->ls_type) == ls_type())
	    return(true);
    }

    return(false);
}

