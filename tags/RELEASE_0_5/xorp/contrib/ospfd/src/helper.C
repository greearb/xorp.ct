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
#include "opqlsa.h"

/* This file contains the routines implementing
 * helper mode for hitless restart.
 */

/* If we are in helper mode, we should be advertising
 * the neighbor as fully adjacent, even if it isn't.
 */

bool SpfNbr::adv_as_full()

{
    if (n_state == NBS_FULL)
        return(true);
    if (n_helptim.is_running())
        return(true);
    return(false);
}

/* If we are helping a neighbor perform a hitless restart,
 * its timer should be running.
 */

bool SpfNbr::we_are_helping()

{
    return(n_helptim.is_running());
}

/* If the helper timer expires, we exit helper mode with
 * a reason of "timeout".
 */

void HelperTimer::action()

{
    np->exit_helper_mode("Timeout");
}

/* We have received a grace-LSA from a neighbor. If after
 * parsing the grace-LSA a) the link-state database has not
 * changed (aside from refreshes) since the beginning
 * of the requested refresh period and b) the refresh period has not
 * expired, then we enter helper mode for the neighbor by
 * setting the helper timer.
 */

void OSPF::grace_LSA_rx(opqLSA *lsap, LShdr *hdr)

{
    SpfNbr *np;
    int grace_period;
    const char *refusal = 0;

    // Ignore our own
    if (lsap->adv_rtr() == myid)
        return;
    if (!(np = lsap->grace_lsa_parse((byte *)(hdr+1),
				     ntoh16(hdr->ls_length)-sizeof(LShdr),
				     grace_period)))
        return;

    // Have associated grace-LSA with a neighbor
    if (spflog(LOG_GRACERX, 5))
        log(np);
    // If we are going to cancel some help sessions, do it now
    if (topology_change)
        htl_topology_change();

    // Now determine whether we should accept it
    // Skip these checks if already helping
    if (!np->we_are_helping()) {
    // Neighbor must be in Full state
    if (np->n_state != NBS_FULL)
        refusal = "Not full";
    // No topology changes since grace period start
	else if (np->changes_pending())
        refusal = "Topology change";
    }
    // Grace period already expired?
    if (refusal == 0 && grace_period <= 0)
        refusal = "Timeout";

    /* If we are refusing the grace request, either exit
     * helper mode if we are already helping, or just
     * log the error.
     */
    if (refusal != 0 && !np->we_are_helping()) {
	if (spflog(LOG_GRACE_REJECT, 5)) {
	    log(np);
	    log(":");
	    log(refusal);
	}
    }
    else if (refusal != 0)
        np->exit_helper_mode(refusal);
    else {
        // (Re)enter helper mode
	if (spflog(LOG_HELPER_START, 5))
	    log(np);
        if (np->we_are_helping()) {
	    np->n_helptim.stop();
	    np->n_ifp->if_helping--;
	    np->n_ifp->area()->a_helping--;
	    n_helping--;
	}
	np->n_helptim.start(grace_period*Timer::SECOND, false);
	np->n_ifp->if_helping++;
	np->n_ifp->area()->a_helping++;
	n_helping++;
    }
}

/* When a grace-LSA is flushed, we exit helper mode.
 * This is considered to be the successful completion
 * of a hitless restart.
 */

void OSPF::grace_LSA_flushed(class opqLSA *lsap)

{
    SpfNbr *np;
    int grace_period;

    // Ignore our own
    if (lsap->adv_rtr() == myid)
        return;
    if (!(np = lsap->grace_lsa_parse(lsap->lsa_body,
				     lsap->lsa_length - sizeof(LShdr),
				     grace_period)))
        return;
    // Exit helper mode
    if (np->we_are_helping())
        np->exit_helper_mode("Success");
}

/* Parse the body of a grace-LSA, determing a) the length of the
 * requested grace period (from now) and b) the neighbor requesting
 * grace.
 */

SpfNbr *opqLSA::grace_lsa_parse(byte *body, int len, int &g_period)

{
    TLVbuf buf(body, len);
    int type;
    InAddr nbr_addr = 0;
    int32 val;

    // Parse body of grace-LSA
    g_period = 0;
    while (buf.next_tlv(type)) {
	switch(type) {
	  case HLRST_PERIOD:	// Length of grace period
	    if (!buf.get_int(g_period))
	        return(0);
	    break;
	  case HLRST_REASON:	// Reason for restart
	    break;
	  case HLRST_IFADDR:	// Interface address
	    if (!buf.get_int(val))
	        return(0);
	    nbr_addr = (InAddr) val;
	    break;
	  default:
	    break;
	}
    };

    g_period -= (int) lsa_age();
    return(lsa_ifp->find_nbr(nbr_addr, adv_rtr()));
}

/* When exiting helper mode, we need to reoriginate
 * router-LSAs, network-LSAs, and rerun the Designated Router
 * calculation for the associated interface.
 * If the neighbor we were helping is DR, make sure it
 * stays DR until we receive its next Hello Packet.
 */

void SpfNbr::exit_helper_mode(const char *reason, bool actions)

{
    if (ospf->spflog(LOG_HELPER_STOP, 5)) {
        ospf->log(this);
	ospf->log(":");
	ospf->log(reason);
    }
    n_helptim.stop();
    n_ifp->if_helping--;
    n_ifp->area()->a_helping--;
    ospf->n_helping--;
    /* If neighbor is not yet full again, do the
     * processing that should have been done when the
     * neighjbor initially went out of FULL state,
     */
    if (n_state != NBS_FULL) {
        SpfArea *tap;
        tap = n_ifp->transit_area();
	if (n_ifp->if_nfull-- == 1)
	    n_ifp->reorig_all_grplsas();
	if (tap && tap->n_VLs-- == 1 && actions)
	    tap->rl_orig();
    }
    // Neighbor stays DR until next Hello
    else if (n_ifp->if_dr_p == this)
        n_dr = n_ifp->if_dr;
    // Caller may take actions itself
    if (actions) {
        // Recalculate Designated Router
        n_ifp->run_fsm(IFE_NCHG);
	// Re-originate router-LSA
	n_ifp->area()->rl_orig();
	// And network-LSA
	n_ifp->nl_orig(false);
    }
}

/* Determine whether a changed LSA should terminate
 * helping sessions in one or more areas. If so,
 * set SpfArea::cancel_help_sessions, and the sessions
 * will be cancelled at the next timer tick. We don't
 * cancel the sessions in this routine to avoid recursive
 * updates of the link-state database.
 */

void OSPF::cancel_help_sessions(LSA *lsap)

{
    SpfArea *ap;

    if (n_helping == 0)
        return;
    // Only LS types 1-5 are significant for hitless restart
    if (lsap->lsa_type > LST_ASL)
        return;
    // Area-scoped change
    else if (lsap->lsa_type < LST_ASL) {
	ap = lsap->lsa_ap;
	if ((ap->cancel_help_sessions = (ap->a_helping != 0)))
	    topology_change = true;;
    }
    // Global-scoped change
    else {
        AreaIterator iter(this);
	while ((ap = iter.get_next())) {
	    if (ap->is_stub())
	        continue;
	    if ((ap->cancel_help_sessions = (ap->a_helping != 0)))
	        topology_change = true;;
	}
    }
}

/* A topology change has occurred. Cancel all helping modes,
 * and reoriginate router-LSAs, network-LSAs and rerun
 * Designated Router calculations, as necessary.
 *
 * Also note the time, for use in verifying future
 * grace requests.
 */

void OSPF::htl_topology_change()

{
    AreaIterator a_iter(this);
    SpfArea *ap;

    topology_change = false;
    // Cancel any helping sessions
    while ((ap = a_iter.get_next())) {
        if (!ap->cancel_help_sessions)
	    continue;
	IfcIterator iiter(ap);
	SpfIfc *ip;
	while ((ip = iiter.get_next())) {
	    if (ip->if_helping == 0)
	        continue;
	    NbrIterator niter(ip);
	    SpfNbr *np;
	    while ((np = niter.get_next()))
	        if (np->we_are_helping())
		    np->exit_helper_mode("Topology change", false);
	    // Recalculate Designated Router
	    ip->run_fsm(IFE_NCHG);
	    // Reoriginate network-LSA
	    ip->nl_orig(false);
	}
	ap->cancel_help_sessions = false;
    }
	// Re-originate all router-LSAs
	rl_orig();
}
