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

/* Routines that manipulate the OSPF link-state database. The
 * database is represented by the LSdb class.
 */


#include "ospfinc.h"
#include "system.h"
#include "opqlsa.h"

/* Find the AVL tree associated with this particular area
 * and LS type.
 */

AVLtree *OSPF::FindLSdb(SpfIfc *ip, SpfArea *ap, byte lstype)

{
    if (lstype == LST_ASL)	// AS-external_LSAs
	return(&ospf->extLSAs);
    if (lstype == LST_AS_OPQ)	// AS-scoped Opaque-LSAs
	return(&ospf->ASOpqLSAs);

    if (ap) {
	switch(lstype) {
          case LST_RTR:		// Router-LSAs
	    return(&ap->rtrLSAs);
          case LST_NET:		// Network-LSAs
	    return(&ap->netLSAs);
          case LST_SUMM:	// Summary-link LSAs (inter-area routes)
	    return(&ap->summLSAs);
          case LST_ASBR:	// ASBR-summaries
	    return(&ap->asbrLSAs);
          case LST_GM:		// Group-membership-LSA (MOSPF)
	    return(&ap->grpLSAs);
          case LST_AREA_OPQ:	// Area-scoped Opaque-LSAs
	    return(&ap->AreaOpqLSAs);
          default:
	    break;
        }
    }

    if (ip && lstype == LST_LINK_OPQ)	// Link-scoped Opaque-LSAs
	return(&ip->LinkOpqLSAs);

    return(0);
}

/* Return the flooding scope of an LSA, based on its LS type.
 */

int flooding_scope(byte lstype)

{
    switch(lstype) {
      case LST_LINK_OPQ:// Link-scoped Opaque-LSAs
	return(LocalScope);
      case LST_RTR: 	// Router-LSAs
      case LST_NET: 	// Network-LSAs
      case LST_SUMM:	// Summary-link LSAs (inter-area routes)
      case LST_ASBR:	// ASBR-summaries (inter-area)
      case LST_GM:	// Group-membership-LSA (MOSPF)
      case LST_AREA_OPQ:// Area-scoped Opaque-LSAs
	return(AreaScope);
      case LST_ASL: 	// AS-external_LSAs
      case LST_AS_OPQ:	// AS-scoped Opaque-LSAs
	return(GlobalScope);
      default:
	break;
    }

    return(0);
}

/* Return the number of LSAs currently in the
 * area's link-state database.
 */

int SpfArea::n_LSAs()

{
    int n_lsas;

    n_lsas = rtrLSAs.size();
    n_lsas += netLSAs.size();
    n_lsas += summLSAs.size();
    n_lsas += asbrLSAs.size();
    n_lsas += grpLSAs.size();
    return(n_lsas);
}


/* Find an LSA in the link state database, given the LSA's
 * LS type, Link State ID and Originating Router.
 * If this is a network-LSA, and the passed rtid is 0, stop
 * after matching the Link State ID. This is necessary when
 * performing the Dijkstra.
 */

LSA *OSPF::FindLSA(SpfIfc *ip,SpfArea *ap,byte lstype,lsid_t lsid, rtid_t rtid)

{
    AVLtree *btree;

    if (!(btree = FindLSdb(ip, ap, lstype)))
	return(0);

    return((LSA *) btree->find((uns32) lsid, (uns32) rtid));
}

/* Find the LSA of a given type and Link State ID that we
 * ourselves have originated, if any.
 */

LSA *OSPF::myLSA(SpfIfc *ip, SpfArea *ap, byte lstype, lsid_t lsid)

{
    return(FindLSA(ip, ap, lstype, lsid, myid));
}

/* Append all the LSAs of a particular type to a given lsalist.
 * Use for example when creating a Database Summary list at the
 * beginning of the Database Description process.
 * When in hitless restart, we don't add our own router-LSAs
 * to the list, as they will look like topology changes
 * to the neighboring routers. Instead, we reflood the router-LSAs
 * at the conclusion of hitless restart.
 */

void SpfIfc::AddTypesToList(byte lstype, LsaList *lp)

{
    AVLtree *btree;
    LSA *lsap;
    SpfArea *ap;

    ap = area();
    if (!(btree = ospf->FindLSdb(this, ap, lstype)))
	return;

    lsap = (LSA *) btree->sllhead;
    for (; lsap; lsap = (LSA *) lsap->sll)
	lp->addEntry(lsap);
}

/* Add an LSA to the database. If there is already a database copy, and
 * it's not on any lists, it can just be updated in place. As a special
 * case, if it is a simple refresh, we can just return after updating
 * the stored link state header.
 *
 * Otherwise, we allocate a new database copy, install it in the database
 * and parse it for ease of later routing calculations. In the process,
 * the old database copy is unparsed. NOTE: this may cause the old data
 * copy to be freed in the process, so the caller should no longer
 * reference "current" after calling this routine.
 */

LSA *OSPF::AddLSA(SpfIfc *ip,SpfArea *ap,LSA *current,LShdr *hdr,bool changed)

{
    LSA *lsap;
    int blen;
    RTE *old_rte = 0;
    bool min_failed=false;

    blen = ntoh16(hdr->ls_length) - sizeof(LShdr);
    if (current) {
        min_failed = current->since_received() < MinArrival;
	old_rte = current->rtentry();
	current->stop_aging();
	update_lsdb_xsum(current, false);
    }

    if (current && current->refct == 0) {
	// Update in place
	if (changed)
	    UnParseLSA(current);
	lsap = current;
	lsap->hdr_parse(hdr);
	lsap->start_aging();
	lsap->changed = changed;
	lsap->deferring = false;
	lsap->rollover = current->rollover;
	lsap->min_failed = min_failed;
	if (!changed) {
	    update_lsdb_xsum(lsap, true);
	    return(lsap);
	}
    }
    else {
	switch (hdr->ls_type) {
	  case LST_RTR:
	    lsap = new rtrLSA(ap, hdr, blen);
	    break;
	  case LST_NET:
	    lsap = new netLSA(ap, hdr, blen);
	    break;
	  case LST_SUMM:
	    lsap = new summLSA(ap, hdr, blen);
	    break;
	  case LST_ASBR:
	    lsap = new asbrLSA(ap, hdr, blen);
	    break;
	  case LST_ASL:
	    lsap = new ASextLSA(hdr, blen);
	    break;
	  case LST_GM:
	    lsap = new grpLSA(ap, hdr, blen);
	    break;
	  case LST_LINK_OPQ:
	  case LST_AREA_OPQ:
	  case LST_AS_OPQ:
	    lsap = new opqLSA(ip, ap, hdr, blen);
	    break;
	  default:
	    lsap = 0;
	    sys->halt(HALT_LSTYPE, "Bad LS type");
	    break;
	}

	// If database copy, unparse it
	if (!current)
	    lsap->changed = true;
	else {
	    lsap->changed = changed;
	    lsap->rollover = current->rollover;
	    lsap->min_failed = min_failed;
	    if (current->lsa_rxmt != 0)
		lsap->changed |= current->changed;
	    lsap->update_in_place(current);
	    UnParseLSA(current);
	}
	lsap->start_aging();
    }
    
    // Parse the new body contents
    ParseLSA(lsap, hdr);
    update_lsdb_xsum(lsap, true);
    // Provide Opaque-LSAs to requesters
    if (hdr->ls_type >= LST_LINK_OPQ && hdr->ls_type <= LST_AS_OPQ)
        upload_opq(lsap);
    // If changes, schedule new routing calculations
    if (changed) {
	rtsched(lsap, old_rte);
	cancel_help_sessions(lsap);
	if (in_hitless_restart())
	    htl_check_consistency(ap, hdr);
    }

    return(lsap);
}

/* When overwriting the database copy, copy the needed state
 * into the nely installed LSA.
 */

void LSA::update_in_place(LSA *)

{
}

void rteLSA::update_in_place(LSA *lsap)

{
    rteLSA *rtelsap;

    rtelsap = (rteLSA *) lsap;
    orig_rte = rtelsap->orig_rte;
}

/* Flush the locally-originated LSAs of a particular type.
 */

void OSPF::flush_self_orig(AVLtree *tree)

{
    AVLsearch iter(tree);
    LSA *lsap;

    while ((lsap = (LSA *)iter.next()))
	if (lsap->adv_rtr() == my_id())
	    lsa_flush(lsap);
}

/* Delete LSAs from an area's link-state database. This is done
 * silently, without reflooding, as this routine is only called when
 * there are no operational interfaces left connecting to the
 * area (this could be as a result of something like a change in
 * the area's stub status). However, we do inform local applications
 * that Opaque-LSAs have been deleted.
 *
 * This routine must perform the logic included in OSPF::DeleteLSA()
 * for each LSA, although we choose to do it here in a more
 * efficient manner.
 *
 * All AS-external-LSAs are left alone, as they belong to all
 * areas. Link-local-LSAs are deleted in SpfIfc::delete_lsdb().
 */

void SpfArea::delete_lsdb()

{
    byte lstype;
    SpfArea *bb;

    bb = ospf->FindArea(BACKBONE);

    for (lstype = 0; lstype <= MAX_LST; lstype++) {
	AVLtree *tree;
	AVLsearch *iter;
	LSA *lsap;
	if (flooding_scope(lstype) != AreaScope)
	    continue;
	if (!(tree = ospf->FindLSdb(0, this, lstype)))
	    continue;
	iter = new AVLsearch(tree);
	while ((lsap = (LSA *)iter->next())) {
	    lsap->stop_aging();
	    ospf->UnParseLSA(lsap);
	    switch(lstype) {
  	      case LST_AREA_OPQ:
		// Notify applications of Opaque-LSA deletion?
		if (lsap->lsa_rcvage != MaxAge) {
		    lsap->lsa_rcvage = MaxAge;
		    ospf->upload_opq(lsap);
		}
		break;
	      case LST_GM:
		if (bb && bb->n_active_if != 0)
		    bb->grp_orig(lsap->ls_id(), 0);
		break;
	      default:
		break;
	    }
	}
	delete iter;
	// This frees the LSAs, unless they are on some list
	tree->clear();
    }

    // Reset database checksum
    db_xsum = 0;
}

/* Silently delete all link-local Opaque-LSAs. Analogue to
 * SpfArea::delete_lsdb(), this is only called when the
 * interface becomes inoperational.
 */

void SpfIfc::delete_lsdb()

{
    AVLtree *tree;
    AVLsearch *iter;
    LSA *lsap;

    tree = ospf->FindLSdb(this, if_area, LST_LINK_OPQ);
    iter = new AVLsearch(tree);
    while ((lsap = (LSA *)iter->next())) {
	lsap->stop_aging();
	ospf->UnParseLSA(lsap);
	// Notify applications of Opaque-LSA deletion?
	if (lsap->lsa_rcvage != MaxAge) {
	    lsap->lsa_rcvage = MaxAge;
	    ospf->upload_opq(lsap);
    }
    }
    delete iter;
    // This frees the LSAs, unless they are on some list
    tree->clear();

    // Reset database checksum
    db_xsum = 0;
}

/* Parse an LSA. Call the LSA-specific parse routine. If that
 * routine indicates an exception, then must store the entire
 * body of the LSA in order to rebuild it later.
 */

void OSPF::ParseLSA(LSA *lsap, LShdr *hdr)

{
    int blen;

    if (lsap->parsed)
	return;

    blen = ntoh16(hdr->ls_length) - sizeof(LShdr);

    if (lsap->lsa_age() != MaxAge) {
        bool o_dna = donotage();
	bool o_a_dna = (lsap->lsa_ap ? lsap->lsa_ap->donotage() : false);
	lsap->exception = false;
	lsap->parsed = true;
	lsap->parse(hdr);
	lsap->process_donotage(true);
	total_lsas++;
	if (o_dna != donotage())
	    dna_change = true;
	if (lsap->lsa_ap && o_a_dna != lsap->lsa_ap->donotage())
	    lsap->lsa_ap->dna_change = true;
    }
    else
	lsap->exception = true;

    delete [] lsap->lsa_body;
    lsap->lsa_body = 0;

    if (lsap->exception) {
	lsap->lsa_body = new byte[blen];
	memcpy(lsap->lsa_body, (hdr + 1), blen);
    }
}

/* Process the DC-bit, to tell whether all routers
 * support DoNotAge. Argument indicates whether we
 * are parsing or unparsing.
 * Overriden for ASBR-summary-LSAs, which do special
 * indication-LSA processing.
 */

void LSA::process_donotage(bool parse)

{
    int increment;

    increment = parse ? 1 : -1;
    // Update global DoNotAge provcessing capabilities
    if ((lsa_opts & SPO_DC) != 0)
	return;
    // Area scoped?
    if (lsa_ap) {
	lsa_ap->wo_donotage += increment;
	if (!lsa_ap->is_stub())
	    ospf->wo_donotage += increment;
    }
    else {
	AreaIterator a_iter(ospf);
	SpfArea *ap;
        ospf->wo_donotage += increment;
	while ((ap = a_iter.get_next())) {
	    if (!ap->is_stub())
	        ap->wo_donotage += increment;
	}
    }
}

/* Unparse the LSA
 */

void OSPF::UnParseLSA(LSA *lsap)

{
    if (lsap->parsed) {
        bool o_dna = donotage();
	bool o_a_dna = (lsap->lsa_ap ? lsap->lsa_ap->donotage() : false);
	lsap->parsed = false;
	lsap->process_donotage(false);
	// Type-specific unparse
	lsap->unparse();
	total_lsas--;
	if (o_dna != donotage())
	    dna_change = true;
	if (lsap->lsa_ap && o_a_dna != lsap->lsa_ap->donotage())
	    lsap->lsa_ap->dna_change = true;
    }
}

/* Delete an LSA from the link-state database. Remove from
 * the AVL tree last, as that may cause the LSA to be returned to
 * the heap (and therefore become inaccessible). UnParseLSA() is called,
 */

void OSPF::DeleteLSA(LSA *lsap)

{
    AVLtree *btree;

    if (spflog(LOG_LSAFREE, 3))
	log(lsap);

    update_lsdb_xsum(lsap, false);
    lsap->stop_aging();
    UnParseLSA(lsap);
    btree = FindLSdb(lsap->lsa_ifp, lsap->lsa_ap, lsap->lsa_type);
    btree->remove((AVLitem *) lsap);
    lsap->delete_actions();
    lsap->chkref();
}

/* Update the checksum of the whole database. One checksum
 * is kept for AS-external-LSAs, one for each area's
 * link-state database, and one for each interface (link-scoped
 * LSAs).
 */

void OSPF::update_lsdb_xsum(LSA *lsap, bool add)

{
    uns32 *db_xsum;
    int scope;

    scope = flooding_scope(lsap->ls_type());

    if (scope == LocalScope)
	db_xsum = &lsap->lsa_ifp->db_xsum;
    else if (scope == AreaScope)
	db_xsum = &lsap->lsa_ap->db_xsum;
    else
	db_xsum = &ase_xsum;

    if (add)
	(*db_xsum) += lsap->lsa_xsum;
    else
	(*db_xsum) -= lsap->lsa_xsum;
}

/* Get the next LSA from the link-state database. Organized
 * first by OSPF area (AS-external-LSAs are considered part
 * of Area 0), then LS type, Link State ID, and Advertising
 * Router.
 *
 * Currently the next LSA function ignores link-scoped LSAs.
 * You can get these by using the API routine OSPF::get_next_opq().
 */

LSA *OSPF::NextLSA(aid_t a_id, byte ls_type, lsid_t id, rtid_t advrtr)

{
    LSA *lsap;
    SpfArea *ap;
    AVLtree *tree;

    // Iterate over all areas
    lsap = 0;
    ap = FindArea(a_id);
    do {
	// Iterate over LS types
	for (; ls_type <= MAX_LST; id = advrtr = 0, ++ls_type) {
	    if (flooding_scope(ls_type) == LocalScope)
	        continue;
	    if (a_id != 0 && flooding_scope(ls_type) == GlobalScope)
	        continue;
	    tree = FindLSdb(0, ap, ls_type);
	    if (tree) {
	        AVLsearch iter(tree);
		iter.seek(id, advrtr);
	        if ((lsap = (LSA *) iter.next()))
		    return(lsap);
	    }
	}
	ls_type = 0;
    } while ((ap = NextArea(a_id)));

    return(lsap);
}

/* This version of Next LSA can be used to get Link-local LSAs.
 * For link-local LSAs associated with virtual links, the
 * taid is the virtual link's Transit Area (non-zero)
 * and if_addr is the Router ID of the other endpoint of the
 * virtual link.
 */

LSA *OSPF::NextLSA(InAddr if_addr, int phyint, aid_t taid, 
		   byte ls_type, lsid_t id, rtid_t advrtr)

{
    LSA *lsap;
    SpfIfc *ip;
    AVLtree *tree;

    // Iterate over all areas
    lsap = 0;
    ip = (taid ? find_vl(taid, if_addr) : find_ifc(if_addr, phyint));
    do {
	// Iterate over LS types
	for (; ls_type <= MAX_LST; id = advrtr = 0, ++ls_type) {
	    if (flooding_scope(ls_type) != LocalScope)
	        continue;
	    tree = FindLSdb(ip, 0, ls_type);
	    if (tree) {
	        AVLsearch iter(tree);
		iter.seek(id, advrtr);
	        if ((lsap = (LSA *) iter.next()))
		    return(lsap);
	    }
	}
	ls_type = 0;
	if (ip && ip->is_virtual()) {
	    taid = ip->transit_area()->a_id;
	    if_addr = *ip->vl_endpt();
	}
	else if (ip) {
	    if_addr = ip->if_addr;
	    phyint = ip->if_phyint;
	}
	// Get next interface
	if (taid) 
	   ip = next_vl(taid, if_addr);
	else if (!(ip = next_ifc(if_addr,phyint)))
	   ip = next_vl(0, 0);
    } while (ip);

    return(lsap);
}
