/*
 *   OSPFD routing daemon
 *   Copyright (C) 2001 by John T. Moy
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
#include "nbrfsm.h"
#include "phyint.h"

/* This file contains the routines implementing the
 * hitless restart of an OSPF, as defined
 * in draft-ietf-ospf-hitless-restart-00.txt.
 * The first half of the routines are used by the
 * router to prepare for hitless restart, and the second
 * half for after the restart has occurred.
 */

/* Preparation for hitless restart goes through a number
 * of phases. This routine is called on the one
 * second timer OSPF::htltim to check whether the current phase
 * is done. If so, the next phase is started by calling
 * next_hitless_phase() and falling though the case statement.
 * Each phase is limited to a 10 second duration.
 *
 * The phases are:
 *	1 - Originate grace-LSAs
 *	2 - check to see they are reliably delivered
 *	3 - Save hitless info and restart
 */

void OSPF::prepare_hitless_restart()

{
    const char *phase_name = "";

    switch(hitless_prep_phase) {
      case 1:	// Send grace-LSAs
	send_grace_lsas();
	next_hitless_phase();
	// Fall through
      case 2:	// Verify that neighbors have received grace-LSAs
	phase_name = "Sending grace-LSAs";
	if (!verify_grace_acks())
	    break;
	next_hitless_phase();
	// Fall through
      case 3:	// Doesn't return
	store_hitless_parameters();
	spflog(LOG_PREPDONE, 5);
	// Inhibit logging from now
	base_priority = 10;
	sys->halt(0, "Hitless restart preparation complete");
	break;
      default:	// Shouldn't ever happen
	return;
    }

    // Has current phase timed out?
    if (++phase_duration > 10) {
	if (spflog(LOG_PHASEFAIL, 5))
	    log(phase_name);
        next_hitless_phase();
	// Recursion: start next phase immediately
	prepare_hitless_restart();
	return;
    }

    // Check for phase completion in another second
    htltim.start(Timer::SECOND);
}

/* Advance to the next phase of hitless restart
 * preparation.
 */

void OSPF::next_hitless_phase()

{
    hitless_prep_phase++;
    phase_duration = 0;
}

/* When timer fires, check to see whether current
 * phase of hitless restart preparation has completed.
 */

void HitlessPrepTimer::action()

{
    ospf->prepare_hitless_restart();
}

/* Send grace-LSAs out all interfaces.
 */

void OSPF::send_grace_lsas()

{
    IfcIterator ifcIter(ospf);
    SpfIfc *ip;
    byte *body;
    int blen;
    lsid_t ls_id;

    // Build body of grace-LSA
    ls_id = OPQ_T_HLRST << 24;

    while ((ip = ifcIter.get_next())) {
        tlvbuf.reset();
	tlvbuf.put_int(HLRST_PERIOD, grace_period);
	tlvbuf.put_byte(HLRST_REASON, restart_reason);
	if (ip->is_multi_access())
	    tlvbuf.put_int(HLRST_IFADDR, ip->if_addr);
	body = tlvbuf.start();
	blen = tlvbuf.length();
        opq_orig(ip, ip->area(), LST_LINK_OPQ, ls_id, body, blen, true, 0);
    }
}

/* Verify that all grace-LSAs have been acknowledged.
 */

bool OSPF::verify_grace_acks()

{
    IfcIterator ifcIter(ospf);
    SpfIfc *ip;
    lsid_t ls_id;
    LSA *lsap;
    int i;

    ls_id = OPQ_T_HLRST << 24;

    for (i = 0; (ip = ifcIter.get_next()); i++) {
        lsap = myLSA(ip, 0, LST_LINK_OPQ, ls_id);
	if (lsap && lsap->lsa_rxmt != 0)
	    return(false);
    }

    if (spflog(LOG_GRACEACKS, 5)) {
        log(i);
	log(" interface(s)");
    }
    return(true);
}

/* Make the call to the system interface to store
 * the hitless restart information in non-volatile
 * storage.
 */

void OSPF::store_hitless_parameters()

{
    IfcIterator ifcIter(ospf);
    SpfIfc *ip;
    int i;
    int n_md5;
    MD5Seq *sns;

    // Count interfaces using MD5
    for (i = 0, n_md5 = 0; (ip = ifcIter.get_next()); i++)
        if (ip->if_autype == AUT_CRYPT)
	    n_md5++;

    sns = (n_md5 != 0) ? new MD5Seq[n_md5] : 0;
    ifcIter.reset();
    for (i = 0, n_md5 = 0; (ip = ifcIter.get_next()); i++)
	if (ip->if_autype == AUT_CRYPT) {
	    sns[n_md5].phyint = ip->if_phyint;
	    sns[n_md5].if_addr = ip->if_addr;
	    sns[n_md5++].seqno = sys_etime.sec;
	}

    sys->store_hitless_parms(grace_period, n_md5, sns);
    delete sns;
}

/* The rest of the file contains routines that are used by
 * the router when it is really in hitless restart state --
 * that is, it has completed its preparation and then restarted
 * or reloaded. You can tell that it is this state
 * by using OSPF::in_hitless_restart().
 *
 * When in hitless restart, we don't install/delete routes
 * into/from the system kernel's routing table. However, we do
 * still maintain our own routing table (inrttbl) so that we
 * can get virtual links back up and running. We also a) don't
 * delete remnants (stored in OSPF::remnants) from the system
 * kernel and b) ignore delete notifications received from
 * the kernel until we have exited hitless restart.
 *
 * When in hitless restart we don't originate LSAs, nor do
 * we respond to received self-originated LSAs (OSPF::self_originated()).
 * When we exit hitless restart, we go through all the
 * seemingly self-originated LSAs in the database and update
 * all of those that are necessary.
 *
 * Also when in hitless restart, we want to resume DR status
 * when we have previously been DR. For that reason, we declare
 * ourselves DR if the Hello causing the BackupSeen event lists
 * us as DR.
 *
 * When to exit...
 */

/* We are in hitless restart as long as the grace period
 * timer is running.
 */

bool OSPF::in_hitless_restart()

{
    return(hlrsttim.is_running());
}

/* Timer has fired forcing us to exit hitless restart state.
 */

void HitlessRSTTimer::action()

{
    ospf->exit_hitless_restart("Timeout");
}

/* Decide whether we should exit hitless restart, by comparing
 * the router-LSAs that we have generated to the ones that
 * we have received.
 * If we haven't received a router-LSA for one of our areas
 * having operational interfaces, we wait for the RouterDeadInterval
 * or for the grace period, whichever is smaller.
 * If we are Designated Router on a segment, we also need to check
 * whether we would generate the same network-LSA that is in our
 * link-state database.
 * (We actually only compare the network-LSA's length, options field, and
 * a checksum of the body, since we want to cover the case where the
 * neighbors are simply reported in a different order).
 */

void OSPF::htl_exit_criteria()

{
    SpfArea *ap;
    AreaIterator a_iter(this);

    check_htl_termination = false;
    while ((ap = a_iter.get_next())) {
        IfcIterator i_iter(ap);
	SpfIfc *ip;
	int max_dead = 0;
	LSA *rtrlsa;
	// Check to see that router-LSA is same
	if ((rtrlsa = myLSA(0, ap, LST_RTR, myid)) && !rtrlsa->we_orig)
	    return;
	while ((ip = i_iter.get_next())) {
	    netLSA *rxnet;
	    LShdr *hdr;
	    bool no_exit = false;
	    // Check network-LSAs
	    rxnet = (netLSA *) myLSA(0, ap, LST_NET, ip->if_addr);
	    hdr = ip->nl_raw_orig();
	    if ((rxnet != 0) != (hdr != 0))
	        no_exit = true;
	    else if (rxnet) {
	        int len;
	        len = rxnet->lsa_length;
		if (len != ntoh16(hdr->ls_length))
		    no_exit = true;
		else if (hdr->ls_opts != rxnet->lsa_opts)
		    no_exit = true;
		else {
		    LShdr *rxhdr;
		    rxhdr = BuildLSA(rxnet);
		    len -= sizeof(LShdr);
		    if (incksum((uns16 *)(hdr +1), len) != 
			incksum((uns16 *)(rxhdr +1), len))
		        no_exit = true;
		}
	    }
	    if (hdr)
	        free_orig_buffer(hdr);
	    if (no_exit)
	        return;
	    if (ip->if_state != IFS_DOWN && max_dead < (int) ip->if_dint)
	        max_dead = ip->if_dint;
	}
	// If no received router-LSA, wait maximum RouterDeadInterval
	if (!rtrlsa &&
	    time_diff(sys_etime, start_time) < max_dead*Timer::SECOND)
	    return;
    }

    htl_exit_reason = "Success";
    start_htl_exit = true;
}

/* An LSA with changed contents has been received. Check the
 * old router-LSA that we originated for this area before
 * the restart. If there was an adjacency to the LSA just received,
 * but the LSA received is no longer reporting it, a neighbor
 * has given up on us. If so, exit hitless restart.
 */

void OSPF::htl_check_consistency(SpfArea *ap, LShdr *hdr)

{
    RTRhdr  *rhdr;
    RtrLink *rtlp;
    byte *end;
    int n_links;
    int i;
    TOSmetric *mp;
    rtrLSA *rtrlsa;
    Link *lp;

    if (hdr->ls_type != LST_RTR && hdr->ls_type != LST_NET)
        return;
    if (!(rtrlsa = (rtrLSA *)myLSA(0, ap, LST_RTR, myid)))
        return;
    // Look for adjacency in network (old) copy
    for (lp = rtrlsa->t_links; ; lp = lp->l_next) {
	if (!lp)
	    return;
	if (lp->l_ltype == hdr->ls_type && lp->l_id == ntoh32(hdr->ls_id))
	    break;
    }

    // We had an adjacency
    // Now make sure it is still reported
    if (hdr->ls_type == LST_RTR) {
	// Make sure that router-LSA reports link to us
	rhdr = (RTRhdr *) (hdr+1);
	rtlp = (RtrLink *) (rhdr+1);
	n_links = ntoh16(rhdr->nlinks);
	end = ((byte *) hdr) + ntoh16(hdr->ls_length);
	for (i = 0; i < n_links; i++) {
	    if (((byte *) rtlp) > end)
	        break;
	    if (rtlp->link_type == LST_RTR && ntoh32(rtlp->link_id) == myid)
	        return;
	    // Step over non-zero TOS metrics
	    mp = (TOSmetric *)(rtlp+1);
	    mp += rtlp->n_tos;
	    rtlp = (RtrLink *)mp;
	}
    }
    else {
	// Make sure that network-LSA lists us
        NetLShdr *nethdr;
	int len;
	rtid_t *idp;
	nethdr = (NetLShdr *) (hdr + 1);
	len = ntoh16(hdr->ls_length);
	len -= sizeof(LShdr);
	len -= sizeof(NetLShdr);
	idp = (rtid_t *) (nethdr + 1);
	for (; len >= (int) sizeof(rtid_t); ) {
	    if (ntoh32(*idp) == myid)
	        return;
	    // Progress to next link
	    len -= sizeof(rtid_t);
	    idp++;
	}
    }

    // Old adjacncy has not been reported. Exit hitless restart
    htl_exit_reason = "Database Inconsistency";
    start_htl_exit = true;
}

/* Exit hitless restart.
 * Stop the grace period timer, so that the rest of the
 * code will know that we are no longer in hitless restart
 * (in_hitless_restart() will return false). Set "exiting_htl_restart"
 * to true to tell the routing calculations that they should
 * install routing entries into the kernel and originate
 * summary-LSAs/ASBR-summary-LSAs/AS-external-LSAs even if there
 * have been no changes since the last calculations.
 *
 * Then we synchronize our link-state database and the kernel
 * forwarding tables with the current local and network state.
 * Any router-LSAs that we had prevented from flooding are now
 * flooded. We reoriginate/flush any network-LSAs whose contents need
 * to change. Then we rerun the routing calculations. Since
 * "exiting_htl_restart" is true, this will download all routing
 * table entries into the kernel and reoriginate any necessary
 * summary/ASBR-summary/AS-external-LSAs.
 *
 * However, that does not deal with kernel forwarding table entries
 * that need to be deleted, or with LSAs that now need to be
 * flushed. For the former, we go through the remant list and delete
 * any entries that are no longer valid (see
 * OSPF::remnant_notification()). For the latter, we go through
 * the link-state database and attempt to reoriginate (which
 * will flush if necessary) and self-originated LSAs that we have
 * not really originated (LSA::we_orig is false).
 */

void OSPF::exit_hitless_restart(const char *reason)

{
    LSA *lsap;
    IfcIterator i_iter(this);
    SpfIfc *ip;
    AVLsearch remn_iter(&remnants);
    AVLitem *item;
    AreaIterator a_iter(this);
    SpfArea *ap;
    lsid_t ls_id;

    hlrsttim.stop();
    exiting_htl_restart = true;
    if (spflog(LOG_HTLEXIT, 5))
        log(reason);

    // Reoriginate any necessary router-LSAs
    rl_orig();
    // Re-originate any necessary network-LSAs
    while ((ip = i_iter.get_next()))
        ip->nl_orig(false);

    // Rerun routing calculations
    full_calculation();
    do_all_ases();

    // Delete kernel forwarding entries that are no longer valid
    while ((item = remn_iter.next())) {
        InAddr net;
	InMask mask;
	INrte *rte;
	net = item->index1();
	mask = item->index2();
	if (!(rte = inrttbl->find(net, mask)) || !rte->valid()) {
	    if (spflog(LOG_REMNANT, 5)) {
	        log(&net, &mask);
	    }
	    sys->rtdel(net, mask, 0);
	}
    }

    /* Flush/reoriginate all summary-LSAs and ASBR-summary-LSAs
     * that have us as originator but have not been
     * originated by the above (i.e., LSA::we_orig is false).
     */
    while ((ap = a_iter.get_next())) {
        htl_reorig(FindLSdb(0, ap, LST_SUMM));
        htl_reorig(FindLSdb(0, ap, LST_SUMM));
    }
    // Ditto for AS-external-LSAs
    htl_reorig(FindLSdb(0, 0, LST_ASL));

    // Flush grace-LSAs
    i_iter.reset();
    ls_id = OPQ_T_HLRST << 24;
    while ((ip = i_iter.get_next())) {
        if ((lsap = myLSA(ip, 0, LST_LINK_OPQ, ls_id)))
	    lsa_flush(lsap);
    }

    // Done with hitless restart exit
    exiting_htl_restart = false;
}

/* Attempt reorigination of all LSAs listing us
 * as originator in the Advertising Router field, but that
 * we may not have originated since the hitless restart.
 * These LSAs will probably be flushed as a result.
 */

void OSPF::htl_reorig(AVLtree *tree)

{
    AVLsearch iter(tree);
    LSA *lsap;

    while ((lsap = (LSA *)iter.next())) {
	if (lsap->adv_rtr() != myid)
	    continue;
	if (!lsap->we_orig)
	    lsap->reoriginate(false);
    }
}

    
