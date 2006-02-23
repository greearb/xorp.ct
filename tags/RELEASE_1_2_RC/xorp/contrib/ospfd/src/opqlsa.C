/*
 *   OSPFD routing daemon
 *   Copyright (C) 2000 by John T. Moy
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

/* Internal support for Opaque-LSAs.
 */

#include "ospfinc.h"
#include "phyint.h"
#include "ifcfsm.h"
#include "opqlsa.h"

/* Constructor for a opaque-LSA.
 */

opqLSA::opqLSA(SpfIfc *i, SpfArea *a, LShdr *hdr, int blen)
  : LSA(i, a, hdr, blen)

{
    adv_opq = false;
    local_body = 0;
    local_blen = 0;
}

/* Parse a opaque-LSA. Just set the exception
 * flag, so that the caller will instead store
 * the body of the LSA.
 */

void opqLSA::parse(LShdr *hdr)

{
    exception = true;
    // Store for Opaque-LSA application upload, in case
    // LSA is deleted before it can be uploaded
    phyint = (lsa_ifp ? lsa_ifp->if_phyint : -1);
    if_addr = (lsa_ifp ? lsa_ifp->if_addr : 0);
    a_id = (lsa_ap ? lsa_ap->id() : 0);

    // Certain Opaque-LSAs are used by OSPF extensions
    if (lsa_type == LST_LINK_OPQ && ls_id() == (OPQ_T_HLRST<<24))
        ospf->grace_LSA_rx(this, hdr);
}

/* Unparse an opaque-LSA.
 */

void opqLSA::unparse()

{
    // Certain Opaque-LSAs are used by OSPF extensions
    if (lsa_type == LST_LINK_OPQ && ls_id() == (OPQ_T_HLRST<<24))
        ospf->grace_LSA_flushed(this);
}

/* Build an opaque-LSA. Since the parse function
 * always sets the exception flag, this function is never really
 * called.
 */

void opqLSA::build(LShdr *)

{
}

/* Reoriginate an opaque-LSA.
 */

void opqLSA::reoriginate(int forced)

{
    ospf->opq_orig(lsa_ifp, lsa_ap, ls_type(), ls_id(), local_body,
		   local_blen, adv_opq, forced);
}

/* Originate or reoriginate an Opaque-LSA.
 * Find the current database copy, if any. Then store
 * a local copy of the body and install/flood
 * the LSA, after building the network copy, by calling
 * OSPF::lsa_reorig().
 */

void OSPF::opq_orig(SpfIfc *ip, SpfArea *ap, byte lstype, lsid_t ls_id, 
		    byte *body, int blen, bool adv, int forced)

{
    opqLSA *lsap;
    int maxlen;
    LShdr *hdr;
    seq_t seqno;

    lsap = (opqLSA *) myLSA(ip, ap, lstype, ls_id);
    if (!adv) {
        if (lsap) {
	    lsap->adv_opq = false;
	    delete [] lsap->local_body;
	    lsap->local_body = 0;
	    lsap->local_blen = 0;
	    lsa_flush(lsap);
	}
	return;
    }
    
    // Estimate size of LSA
    maxlen = sizeof(LShdr) + blen;
    // Get LS Sequence Number
    seqno = ospf_get_seqno(lstype, lsap, forced);
    if (seqno != InvalidLSSeq) {
	LSA *nlsap;
	// Fill in LSA header
        hdr = ospf->orig_buffer(maxlen);
	hdr->ls_opts = SPO_DC;
	if (ap && !ap->a_stub)
	    hdr->ls_opts |= SPO_EXT;
	hdr->ls_type = lstype;
	hdr->ls_id = hton32(ls_id);
	hdr->ls_org = hton32(my_id());
	hdr->ls_seqno = hton32(seqno);
	// Body
	memcpy(hdr + 1, body, blen);
	// Fill in length, then reoriginate
        hdr->ls_length = hton16(maxlen);
	if ((nlsap = ospf->lsa_reorig(ip, ap, lsap, hdr, forced)))
	    lsap = (opqLSA *) nlsap;
	ospf->free_orig_buffer(hdr);
    }

    // Store local copy of body, in case it is overwritten
    // by received self-originated LSAs
    if (lsap) {
        lsap->adv_opq = true;
	if (lsap->local_body != body) {
	    delete [] lsap->local_body;
	    lsap->local_body = new byte[blen];
	    memcpy(lsap->local_body, body, blen);
	    lsap->local_blen = blen;
	}
    }
}

/* API routines used to force origination of Opaque-LSAs
 * into the routing domain. One function each for
 * link-local, area-scoped, and AS-scoped LSAs.
 */

bool OSPF::adv_link_opq(int phyint, InAddr ifaddr, lsid_t lsid,
			byte *body, int blen, bool adv)

{
    SpfIfc *ip;

    if (!(ip = find_ifc(ifaddr, phyint)))
        return(false);

    opq_orig(ip, ip->area(), LST_LINK_OPQ, lsid, body, blen, adv, 0);
    return(true);
}

bool OSPF::adv_area_opq(aid_t a_id, lsid_t lsid, byte *body, int blen,bool adv)

{
    SpfArea *ap;

    if (!(ap = FindArea(a_id)))
        return(false);

    opq_orig(0, ap, LST_AREA_OPQ, lsid, body, blen, adv, 0);
    return(true);
}

void OSPF::adv_as_opq(lsid_t lsid, byte *body, int blen, bool adv)

{
    opq_orig(0, 0, LST_AS_OPQ, lsid, body, blen, adv, 0);
}

/* Constructor for a holding queue of Opaque-LSAs, destined
 * for a requesting application.
 */

OpqHoldQ::OpqHoldQ(int conn_id) : AVLitem(conn_id, 0)

{
}

/* OSPF API routine whereby an application can register to
 * receive Opaque-LSAs. It gets all of them, and must filter
 * out the ones it doesn't want.
 */

void OSPF::register_for_opqs(int conn_id, bool disconnect)

{
    OpqHoldQ *holdq;
    IfcIterator iiter(this);
    SpfIfc *ip;
    AreaIterator aiter(this);
    SpfArea *ap;

    holdq = (OpqHoldQ *) opq_uploads.find(conn_id, 0);
    if (disconnect) {
        if (holdq) {
	    holdq->holdq.clear();
	    opq_uploads.remove(holdq);
	    delete holdq;
	}
        return;
    }
    // Already registered? If so, just ignore new registration
    else if (holdq)
        return;
    // Create new holdq
    holdq = new OpqHoldQ(conn_id);
    opq_uploads.add(holdq);
    // Install all current Opaque-LSAs onto holdq
    // Global scoped
    if (ifcs)
        ifcs->AddTypesToList(LST_AS_OPQ, &holdq->holdq);
    // Area scoped
    while ((ap = aiter.get_next())) {
        if (ap->a_ifcs)
	    ap->a_ifcs->AddTypesToList(LST_AREA_OPQ, &holdq->holdq);
    }
    // Link scoped
    while ((ip = iiter.get_next()))
        ip->AddTypesToList(LST_LINK_OPQ, &holdq->holdq);
}

/* Return the next Opaque-LSA on the hold queue to the equesting
 * application.
 */

bool OSPF::get_next_opq(int conn_id, int &phyint, InAddr &if_addr,
			aid_t &a_id, struct LShdr * &hdr)

{
    OpqHoldQ *holdq;
    opqLSA *lsap;

    // Not registered?
    if (!(holdq = (OpqHoldQ *) opq_uploads.find(conn_id, 0)))
        return(false);
    LsaListIterator iter(&holdq->holdq);
    while ((lsap = (opqLSA *)iter.get_next())) {
        // OK for Opaque-LSAs. But other LSAs should
        // not be built if they are not valid!
        hdr = BuildLSA(lsap);
        if (!lsap->valid() && lsap->lsa_rcvage != MaxAge) {
	    iter.remove_current();
	    continue;
	}
	phyint = lsap->phyint;
	if_addr = lsap->if_addr;
	a_id = lsap->a_id;
	iter.remove_current();
	return(true);
    }
    return(false);
}

/* Add new Opaque-LSA to upload lists for all requesting
 * applications.
 */

void OSPF::upload_opq(LSA *lsap)

{
    AVLsearch iter(&opq_uploads);
    OpqHoldQ *holdq;

    while ((holdq = (OpqHoldQ *)iter.next()))
        holdq->holdq.addEntry(lsap);
}

/* When overwriting the database copy, copy the needed state
 * into the nely installed LSA. For Opaque-LSAs, need
 * to copy any local data that is used in originating
 * the Opaque-LSA.
 */

void opqLSA::update_in_place(LSA *arg)

{
    opqLSA *olsap;
    olsap = (opqLSA *) arg;
    adv_opq = olsap->adv_opq;
    local_body = olsap->local_body;
    local_blen = olsap->local_blen;
}
