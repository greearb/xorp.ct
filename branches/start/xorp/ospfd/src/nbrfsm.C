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

/* The OSPF neighbor FSM. Defined in Section 10.3 of the OSPF
 * specification, it is table driven with resulting
 * action routines.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"
#include "system.h"

/* The FSM transitions, searched linearly until a match
 * is found.
 */

FsmTran NbrFsm[] = {
    { NBS_ACTIVE, NBE_HELLO,	NBA_RST_IATIM,	0},
    { NBS_BIDIR, NBE_2WAY,	0,		0},
    { NBS_INIT,	NBE_1WAY,	0,		0},
    { NBS_DOWN,	NBE_HELLO,	NBA_ST_IATIM,	NBS_INIT},
    { NBS_DOWN,	NBE_START,	NBA_START,	NBS_ATTEMPT},
    { NBS_ATTEMPT, NBE_HELLO,	NBA_RST_IATIM,	NBS_INIT},
    { NBS_INIT,	NBE_2WAY,	NBA_EVAL1,	0},
    { NBS_INIT,	NBE_DDRCVD,	NBA_EVAL2,	0},
    { NBS_2WAY,	NBE_DDRCVD,	NBA_EVAL2,	0},
    { NBS_EXST,	NBE_NEGDONE,	NBA_SNAPSHOT,	NBS_EXCH},
    { NBS_EXCH,	NBE_EXCHDONE,	NBA_EXCHDONE,	0},
    { NBS_LOAD,	NBE_LDONE,	0,		NBS_FULL},
    { NBS_2WAY,	NBE_EVAL,	NBA_EVAL1,	0},
    { NBS_ADJFORM, NBE_EVAL,	NBA_REEVAL,	0},
    { NBS_PRELIM, NBE_EVAL,	NBA_HELLOCHK,	0},
    { NBS_ADJFORM, NBE_ADJTMO,	NBA_RESTART_DD,	NBS_2WAY},
    { NBS_FLOOD, NBE_DDSEQNO,	NBA_RESTART_DD,	NBS_2WAY},
    { NBS_FLOOD, NBE_BADLSREQ,	NBA_RESTART_DD,	NBS_2WAY},
    { NBS_ANY,	NBE_DESTROY,	NBA_DELETE,	NBS_DOWN},
    { NBS_ANY,	NBE_LLDOWN,	NBA_DELETE,	NBS_DOWN},
    { NBS_ANY,	NBE_INACTIVE,	NBA_DELETE,	NBS_DOWN},
    { NBS_BIDIR, NBE_1WAY,	NBA_CLR_LISTS,	NBS_INIT},
    { 0, 	0,		-1,		0},
};

// Strings for neighbor states
const char *nbrstates(int state)

{
    switch(state) {
      case NBS_DOWN:
	return("Down");
      case NBS_ATTEMPT:
	return("Attempting");
      case NBS_INIT:
	return("1-Way");
      case NBS_2WAY:
	return("2-Way");
      case NBS_EXST:
	return("ExchangeStart");
      case NBS_EXCH:
	return("Exchange");
      case NBS_LOAD:
	return("Loading");
      case NBS_FULL:
	return("Full");
      default:
	break;
    }

    return("Unknown");
}

// Strings for neighbor events
const char *nbrevents(int event)

{
    switch(event) {
      case NBE_HELLO:
	return("HelloReceived");
      case NBE_START:
	return("StartDirective");
      case NBE_2WAY:
	return("2WayHello");
      case NBE_NEGDONE:
	return("NegotiationDone");
      case NBE_EXCHDONE:
	return("ExchangeDone");
      case NBE_BADLSREQ:
	return("BadLSReq");
      case NBE_LDONE:
	return("LoadingDone");
      case NBE_EVAL:
	return("EvaluateAdjacency");
      case NBE_DDRCVD:
	return("DDReceived");
      case NBE_DDSEQNO:
	return("BadDDSequenceNo");
      case NBE_1WAY:
	return("1WayHello");
      case NBE_DESTROY:
	return("DestroyDirective");
      case NBE_INACTIVE:
	return("HelloTimeout");
      case NBE_LLDOWN:
	return("LLDown");
      case NBE_ADJTMO:
	return("AdjacencyTimeout");
      default:
	break;
    }

    return("Unknown");
}

/* The actual Neighbor finite state machine. Call the FSM
 * utility routine to find a matchin transition, then dispatch
 * based upon the action specified.
 * Logs a message whenever the neighbor state regresses, or when
 * a terminal neighbor state has been reached.
 */

void SpfNbr::nbr_fsm(int event)

{
    int action;
    SpfArea *ap;
    SpfArea *tap;	// Transit area for virtual links
    int	llevel;
    int	n_ostate;	// Previous neighbor state

    n_ostate = n_state;
    ap = n_ifp->area();
    action = ospf->run_fsm(&NbrFsm[0], n_state, event);

    switch (action) {
      case 0:		// No associated action
	break;
      case NBA_START:
	send_hello();
	n_htim.start(n_ifp->if_hint*Timer::SECOND);
	n_acttim.start(n_ifp->if_dint*Timer::SECOND);
	break;
      case NBA_RST_IATIM:
	n_acttim.restart(n_ifp->if_dint*Timer::SECOND);
	break;
      case NBA_ST_IATIM:
	n_acttim.start(n_ifp->if_dint*Timer::SECOND);
	break;
      case NBA_EVAL1:
	nba_eval1();
	break;
      case NBA_EVAL2:
	nba_eval2();
	break;
      case NBA_SNAPSHOT:
	nba_snapshot();
	break;
      case NBA_EXCHDONE:
	nba_exchdone();
	break;
      case NBA_REEVAL:
	nba_reeval();
	break;
      case NBA_RESTART_DD:
	nba_clr_lists();
	AddPendAdj();
	break;
      case NBA_DELETE:
	nba_delete();
	break;
      case NBA_CLR_LISTS:
	nba_clr_lists();
	break;
      case NBA_HELLOCHK:
	/* Here we are not yet ready to form adjacencies,
	 * but reevaluation of whether an adjacency should
	 * form lets us set the Hello Interval
	 * appropriately on non-broadcast interfaces.
	 */
	if (n_ifp->type() != IFT_NBMA && n_ifp->type() != IFT_P2MP)
	    break;
	if (!n_ifp->adjacency_wanted(this) && n_state == NBS_ATTEMPT) {
	    n_state = NBS_DOWN;
	    nba_delete();
	    break;
	}
	if (n_ifp->adjacency_wanted(this) && n_state == NBS_DOWN)
	    n_state = NBS_ATTEMPT;
	n_ifp->adjust_hello_interval(this);
	break;
      case -1:		// FSM error
      default:
	if (ospf->spflog(ERR_NBR_FSM, 5)) {
	    ospf->log(nbrevents(event));
	    ospf->log("state ");
	    ospf->log(nbrstates(n_state));
	    ospf->log(this);
	}
	return;
    }

    if (n_ostate == n_state)
	return;
    // State change
    // Log significant events
    if (n_state < n_ostate)
	llevel = 5;
    else if (n_state == NBS_FULL ||
	     (n_state == NBS_2WAY && (!n_adj_pend)))
	llevel = 4;
    else
	llevel = 1;
    if (ospf->spflog(NBR_STATECH, llevel)) {
	ospf->log(nbrstates(n_state));
	ospf->log("<-");
	ospf->log(nbrstates(n_ostate));
	ospf->log(" event ");
	ospf->log(nbrevents(event));
	ospf->log(this);
    }

    // Maintain count of adjacencies that we are
    // currently attempting
    ap->adj_change(this, n_ostate);
    tap = n_ifp->transit_area();
    // now Full
    if (n_state == NBS_FULL) {
        if (!we_are_helping() && n_ifp->if_nfull++ == 0)
	    n_ifp->reorig_all_grplsas();
	ospf->n_dbx_nbrs--;
	n_progtim.stop();
	exit_dbxchg();
	if (tap && tap->n_VLs++ == 0)
	    tap->rl_orig();
	ap->rl_orig();
    }
    // beginning exchange
    else if (n_ostate <= NBS_EXST && n_state > NBS_EXST)
	ospf->n_dbx_nbrs++;
    // Never go from Full state immed back into dbxchng
    else if (n_ostate == NBS_FULL) {
	if (!we_are_helping()) {
	if (n_ifp->if_nfull-- == 1)
	    n_ifp->reorig_all_grplsas();
	if (tap && tap->n_VLs-- == 1)
	    tap->rl_orig();
	ap->rl_orig();
    }
    }
    else if (n_state <= NBS_2WAY && n_ostate >= NBS_EXST) {
	exit_dbxchg();
	if (n_ostate > NBS_EXST)
	    ospf->n_dbx_nbrs--;
    }

    // (Re)originate network-LSA if we're DR
    if (n_ifp->state() == IFS_DR)
	n_ifp->nl_orig(false);

    // If necessary, run Interface state machine with event
    // NeighborChange
    if ((n_state >= NBS_2WAY && n_ostate < NBS_2WAY) ||
	(n_state < NBS_2WAY && n_ostate >= NBS_2WAY))
	n_ifp->run_fsm(IFE_NCHG);
}


/* For a neighbor in state 2-way, it is time to evaluate whether
 * we should attempt an adjacency. If so, we put the neighbor
 * on the tail of the "pending adjacency" list. The Neighbor FSM
 * will actually start the adjacencies a few at a time, for scaling
 * purposes.
 */

void SpfNbr::nba_eval1()

{
    n_state = NBS_2WAY;
    n_ifp->adjust_hello_interval(this);
    if (!n_ifp->adjacency_wanted(this))
	DelPendAdj();
    else if (!n_ifp->more_adjacencies_needed(id()))
	DelPendAdj();
    else if (ospf->n_lcl_inits < ospf->max_dds) {
	n_state = NBS_EXST;
	ospf->n_lcl_inits++;
	DelPendAdj();
	start_adjacency();
    }
    else
	AddPendAdj();		
}

/* Neighbor wants to form an adjacency with us. If adjacency is
 * desired, and there are enough resources, transition to ExStart
 * state. DD packets will be sent in response, as part of the
 * receive processing of DD packets.
 */

void SpfNbr::nba_eval2()

{
    if (!n_ifp->adjacency_wanted(this))
	n_state = NBS_2WAY;
    else if (ospf->n_rmt_inits < ospf->max_dds) {
	n_rmt_init = true;
	n_state = NBS_EXST;
	ospf->n_rmt_inits++;
	DelPendAdj();
	start_adjacency();
    }
    else {
	n_state = NBS_2WAY;
	AddPendAdj();
    }
}


/* Take a snapshot of the link state database, installing LSAs
 * in the "database summary list". Both invalid entries and
 * MaxAge entries are added to the list, but they will be pruned
 * by send_dd().
 */

void SpfNbr::nba_snapshot()

{

    // Area-scoped LSAs
    n_ifp->AddTypesToList(LST_RTR, &n_ddlst);
    n_ifp->AddTypesToList(LST_NET, &n_ddlst);
    n_ifp->AddTypesToList(LST_SUMM, &n_ddlst);
    n_ifp->AddTypesToList(LST_ASBR, &n_ddlst);
    if (supports(SPO_OPQ))
        n_ifp->AddTypesToList(LST_AREA_OPQ, &n_ddlst);
    if ((n_opts & SPO_MC) != 0)
	n_ifp->AddTypesToList(LST_GM, &n_ddlst);
    
    // AS-scoped
    if ((!n_ifp->is_virtual()) && (!n_ifp->area()->is_stub())) {
	n_ifp->AddTypesToList(LST_ASL, &n_ddlst);
	if (supports(SPO_OPQ))
	    n_ifp->AddTypesToList(LST_AS_OPQ, &n_ddlst);
    }

    // Link-scoped
    if (supports(SPO_OPQ))
        n_ifp->AddTypesToList(LST_LINK_OPQ, &n_ddlst);
}


/* The exchange of DD packets has completed. If there are no outstanding
 * requests, transition to Full state. Otherwise, transition to
 * Loading and wait for the requests to complete.
 */

void SpfNbr::nba_exchdone()

{
    if (n_rqlst.is_empty())
	n_state = NBS_FULL;
    else
	n_state = NBS_LOAD;
}


/* We must reexamine a neighbor that we are establishing (or have
 * established) an adjacency with. If we should no longer be
 * adjacenct to the neighbor, tear down the adjacency and go back
 * to state 2WAY.
 */

void SpfNbr::nba_reeval()

{
    n_ifp->adjust_hello_interval(this);
    if (!n_ifp->adjacency_wanted(this)) {
	nba_clr_lists();
	n_state = NBS_2WAY;
    }
}


/* Reinitialize the neighbor's data structures, in preparation
 * for either redoing the adjacency process or deleting the
 * neighbor.
 */

void SpfNbr::nba_clr_lists()

{
    DelPendAdj();
    n_adj_pend = false;

    dd_free();

    // Turn off LSA retransmissions
    clear_rxmt_list();
    
    // Clear all other LSA lists
    n_ddlst.clear();
    n_rqlst.clear();
    database_sent = false;

    // Stop all other timers related to adjacencies
    n_holdtim.stop();
    n_ddrxtim.stop();
    n_rqrxtim.stop();
    n_progtim.stop();
}


/* Determine whether an adjacency is wanted to a given neighbor.
 * Depends upon interface type.
 * On point-to-point links, point-to-multipoint links and virtual
 * links, adjacencies are always wanted.
 * On broadcast and NBMA links, adjacencies must have DR or Backup
 * DR as one (or both) endpoint(s).
 */

int SpfIfc::adjacency_wanted(SpfNbr *)

{
    return(true);
}

int DRIfc::adjacency_wanted(SpfNbr *np)

{
    if (if_state == IFS_DR)
	return(true);
    else if (if_state == IFS_BACKUP)
	return(true);
    else if (np->is_dr())
	return(true);
    else if (np->is_bdr())
	return(true);
    else
	return(false);
}

int P2mPIfc::adjacency_wanted(SpfNbr * /* np */)

{
    return(true);
}

/* Adjust Hello Interval on NBMA and
 * Point-to-MultiPoint interfaces, depending
 * upon neighbor state.
 */

void SpfIfc::adjust_hello_interval(SpfNbr *np)

{
    uns32 period;
    uns32 dead = 0;

    // Only on non-broadcast networks
    if (if_type != IFT_NBMA && if_type != IFT_P2MP)
        return;

    // Determine Hello frequency
    if (!adjacency_wanted(np))
        period = 0;
    else if (np->state() == NBS_DOWN)
        period = if_pint*Timer::SECOND;
    else {
        period = if_hint*Timer::SECOND;
	dead = if_dint*Timer::SECOND;
    }

    // Reset Hello and Dead timers if intervals have changed
    if (period != np->n_htim.interval()) {
        np->n_htim.stop();
	if (period != 0)
	    np->n_htim.start(period);
    }
    if (dead != np->n_acttim.interval()) {
        np->n_acttim.stop();
	if (dead != 0)
	    np->n_acttim.start(dead);
    }
}

/* Prepare a neighbor for deletion.
 */

void SpfNbr::nba_delete()

{
    md5_seqno = 0;
    nba_clr_lists();
    n_dr = UnknownAddr;
    n_bdr = UnknownAddr;
    rq_suppression = false;

    // Stop remaining timers
    n_acttim.stop();
    n_htim.stop();

    if (!configured())
	ospf->delete_neighbors = true;
    else if (n_ifp->state() != IFS_DOWN)
        n_ifp->adjust_hello_interval(this);
}

/* Start trying to establish an adjacency. Increment the DD sequence
 * number, and send an empty DD packet (init, more and master
 * bits set) to the neighbor.
 */

void SpfNbr::start_adjacency()

{
    n_ddseq++;
    n_progtim.start(n_ifp->if_dint*Timer::SECOND);
    send_dd();
}


/* Routines to get next pending adjacency, or to add or delete
 * a neighbor from the pending adjacency list.
 */

// Get next pending adjacency, so it can be started

SpfNbr *GetNextAdj()

{
    SpfNbr *np;

    if (!(np = ospf->g_adj_head))
	return(0);
    if (!(ospf->g_adj_head = np->n_next_pend))
	ospf->g_adj_tail = 0;

    np->n_adj_pend = false;
    return(np);
}

// Add a neighbor to the pedning adjacency list

void SpfNbr::AddPendAdj()

{
    if (n_adj_pend)
	return;

    n_adj_pend = true;
    n_next_pend = 0;
    if (!ospf->g_adj_head) {
	ospf->g_adj_head = this;
	ospf->g_adj_tail = this;
    }
    else {
	ospf->g_adj_tail->n_next_pend = this;
	ospf->g_adj_tail = this;
    }
}

// Remove a neighbor from the pending adjacency list

void SpfNbr::DelPendAdj()

{
    SpfNbr **ptr;
    SpfNbr *prev;
    SpfNbr *nbr;

    if (!n_adj_pend)
        return;
    prev = 0;
    for (ptr = &ospf->g_adj_head; ; prev = nbr, ptr = &nbr->n_next_pend) {
	if (!(nbr = *ptr))
	    return;
	if (nbr == this)
	    break;
    }

    *ptr = n_next_pend;
    if (ospf->g_adj_tail == this)
	ospf->g_adj_tail = prev;
    n_adj_pend = false;
}

/* Exit database exchange, either succeeding or failing.
 * Do the bookeeping necessary to rate limit the
 * number of simultaneous Database Exchanges.
 */

void SpfNbr::exit_dbxchg()

{
    if (n_rmt_init)
	ospf->n_rmt_inits--;
    else
	ospf->n_lcl_inits--;
    // Next time we may be locally initialized
    n_rmt_init = false;
}

/* Progress timer for an ongoing Database Exchange has
 * fired. Stop the database exchange, and return the
 * neighbor to the end of the pending adjacency list.
 */

void ProgressTimer::action()

{
    np->nbr_fsm(NBE_ADJTMO);
}
