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

/* The OSPF interface FSM. Defined in Section 9.3 of the OSPF
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

FsmTran	IfcFsm[] = {
    { IFS_DOWN,	IFE_UP,		IFA_START,	0},
    { IFS_WAIT,	IFE_BSEEN,	IFA_ELECT,	0},
    { IFS_WAIT,	IFE_WTIM,	IFA_ELECT,	0},
    { IFS_WAIT,	IFE_NCHG,	0,		0},
    { IFS_MULTI, IFE_NCHG,	IFA_ELECT,	0},
    { IFS_ANY,	IFE_NCHG,	0,		0},
    { IFS_ANY,	IFE_DOWN,	IFA_RESET,	IFS_DOWN},
    { IFS_ANY,	IFE_LOOP,	IFA_RESET,	IFS_LOOP},
    { IFS_LOOP,	IFE_UNLOOP,	0,		IFS_DOWN},
    { 0, 	0,		-1,		0},
};


// Strings for interface state
const char *ifstates(int state)

{
    switch(state) {
      case IFS_DOWN:
	return("Down");
      case IFS_LOOP:
	return("Lpbk");
      case IFS_WAIT:
	return("Waiting");
      case IFS_PP:
	return("P-P");
      case IFS_OTHER:
	return("DRother");
      case IFS_BACKUP:
	return("Backup");
      case IFS_DR:
	return("DR");
      default:
	break;
    }

    return("Unknown");
}

// Strings for interface events
const char *ifevents(int event)

{
    switch(event) {
      case IFE_UP:
	return("PhyUp");
      case IFE_WTIM:
	return("WaitTim");
      case IFE_BSEEN:
	return("BackupSeen");
      case IFE_NCHG:
	return("NeighborChange");
      case IFE_LOOP:
	return("LoopInd");
      case IFE_UNLOOP:
	return("UnloopInd");
      case IFE_DOWN:
	return("PhyDown");
      default:
	break;
    }

    return("Unknown");
}

/* The actual Interface finite state machine. Call the FSM
 * utility routine to find a matchin transition, then dispatch
 * based upon the action specified.
 * Logs a message whenever the interface state regresses, or when
 * a terminal interface state has been reached.
 */

void SpfIfc::run_fsm(int event)

{
    int action;
    int llevel;
    int	if_ostate;	// Previous interface state

    // Don't process state transitions on loopback interfaces
    if (type() == IFT_LOOPBK)
        return;

    if_ostate = if_state;
    action = ospf->run_fsm(&IfcFsm[0], if_state, event);

    switch (action) {
      case 0:		// No associated action
	break;
      case IFA_START:	// Begin interface processing
	ifa_start();
	break;
      case IFA_ELECT:	// Elect (Backup) Designated Router
	ifa_elect();
	break;
      case IFA_RESET:	// Tear down interface
	ifa_reset();
	break;
      case -1:		// FSM error
      default:
	if (ospf->spflog(ERR_IFC_FSM, 5)) {
	    ospf->log(ifevents(event));
	    ospf->log(" state");
	    ospf->log(ifstates(if_state));
	    ospf->log(this);
	}
	return;
    }

    // If no state change, we're done
    if (if_ostate == if_state)
	return;
    // Set address to use when flooding
    if_faddr = (if_state == IFS_OTHER) ? AllDRouters : AllSPFRouters;
    // Newly up or down
    if (if_ostate == IFS_DOWN)
	if_area->IfcChange(1);
    else if (if_state == IFS_DOWN) {
        // Delete link-local-LSAs
        delete_lsdb();
	if_area->IfcChange(-1);
    }
    // Want AllDRouters now?
    if (if_state > IFS_OTHER && if_ostate <= IFS_OTHER)
	ospf->app_join(if_phyint, AllDRouters);
    else if (if_state <= IFS_OTHER && if_ostate > IFS_OTHER)
	ospf->app_leave(if_phyint, AllDRouters);
    // (Re)originate appropriate LSAs
    // Always do router-LSA, and do network-LSA if we've
    // transitioned to/from Designated Router
    if_area->rl_orig();
    if (if_state == IFS_DR || if_ostate == IFS_DR) {
	nl_orig(false);
	reorig_all_grplsas();
    }

    // Log significant events
    if (if_state < if_ostate)
	llevel = 5;
    else if (if_state >= IFS_PP)
	llevel = 4;
    else
	llevel = 1;
    if (ospf->spflog(IFC_STATECH, llevel)) {
	ospf->log(ifstates(if_state));
	ospf->log("<-");
	ospf->log(ifstates(if_ostate));
	ospf->log(" event ");
	ospf->log(ifevents(event));
	ospf->log(this);
    }
}

/* Start event on interface. Processing depends on interface type.
 * On point-to-point and virtual links, simply transition to point-to-point
 * state and start the Hello Timer. Point-to-MultiPoint interfaces
 * are similar, although hellos are sent to each neighbor separately.
 * On broadcast interfaces set the state to Waiting, start the hello
 * timer, and also start the wait timer. NBMA interfaaces are the non-
 * broadcast version, with hellos sent to each neighbor separately.
 */

void PPIfc::ifa_start()

{
    start_hellos();
    // Open physical interface
    ospf->phy_attach(if_phyint);
    ospf->app_join(if_phyint, AllSPFRouters); 
    if_state = IFS_PP;
}

void VLIfc::ifa_start()

{
    start_hellos();
    if_state = IFS_PP;
}

void P2mPIfc::ifa_start()

{
    start_hellos();
    // Open physical interface
    ospf->phy_attach(if_phyint);
    ospf->app_join(if_phyint, AllSPFRouters); 
    if_state = IFS_PP;
}

void DRIfc::ifa_start()

{
    start_hellos();
    // Open physical interface
    ospf->phy_attach(if_phyint);
    ospf->app_join(if_phyint, AllSPFRouters); 

    if (if_drpri == 0)
	if_state = IFS_OTHER;
    else {
	if_wtim.start(if_dint*Timer::SECOND);
	if_state = IFS_WAIT;
    }
}

/* Elect the Designated Router and its Backup. Executes the algorithm
 * specified in Section 9.4 of the OSPF specification.
 */

void SpfIfc::ifa_elect()

{
    int pass;
    InAddr prev_dr;
    InAddr prev_bdr;
    
    if_wtim.stop();
    prev_bdr = if_bdr;
    prev_dr = if_dr;

    for (pass = 1; pass <= 2; pass++) {
	int declared;
	byte c_pri;
	rtid_t c_id;
	SpfNbr *np;
	NbrIterator iter(this);
	SpfNbr *bdr_p;

	/* Initialize BDR election.
	 * We are BDR until other neighbors are
	 * examined.
	 */
	bdr_p = 0;
	if (if_dr != if_addr && !ospf->host_mode && if_drpri != 0) {
	    declared = (if_bdr == if_addr);
	    if_bdr = if_addr;
	    c_pri = if_drpri;
	    c_id = ospf->my_id();
	}
	else {
	    if_bdr = UnknownAddr;
	    c_pri = 0;
	    declared = false;
	    c_id = 0;
	}
	// Go through bidirectional neighbors
	while ((np = iter.get_next())) {
	    if (np->state() < NBS_2WAY)
		continue;
	    else if (np->n_pri == 0)
		continue;
	    else if (np->declared_dr())
		continue;
	    else if (declared) {
		if (!np->declared_bdr())
		    continue;
		if (np->n_pri < c_pri)
		    continue;
		if (np->n_pri == c_pri && np->n_id < c_id)
		    continue;
	    }
	    else if (!np->declared_bdr()) {
		if (np->n_pri < c_pri)
		    continue;
		if (np->n_pri == c_pri && np->n_id < c_id)
		    continue;
	    }
	    // For now, current neighbor is BDR
	    bdr_p = np;
	    if_bdr = np->n_addr;
	    declared = np->declared_bdr();
	    c_pri = np->n_pri;
	    c_id = np->n_id;
	}

	// If we are helping the current DR through hitless
	// restart, keep it as DR
	if (if_dr_p && if_dr_p->we_are_helping())
	    break;

	// Initialize DR election
	iter.reset();
	if_dr_p = 0;
	if (if_dr == if_addr && if_drpri != 0) {
	    c_pri = if_drpri;
	    c_id = ospf->my_id();
	}
	else {
	    if_dr = UnknownAddr;
	    c_pri = 0;
	}
	// Go through bidirectional neighbors
	while ((np = iter.get_next())) {
	    if (np->state() < NBS_2WAY)
		continue;
	    else if (np->n_pri == 0)
		continue;
	    else if (!np->declared_dr())
		continue;
	    else if (np->n_pri < c_pri)
		continue;
	    else if (np->n_pri == c_pri && np->n_id < c_id)
		continue;
	    // For now, current neighbor is DR
	    if_dr_p = np;
	    if_dr = np->n_addr;
	    c_pri = np->n_pri;
	    c_id = np->n_id;
	}

	// If no DR, set equal to BDR
	if (if_dr == UnknownAddr) {
	    if_dr = if_bdr;
	    if_dr_p = bdr_p;
	}
	// Repeat if DR or backup change involving ourself
	if (if_dr !=prev_dr && (if_dr==if_addr || prev_dr==if_addr))
	    continue;
	else if (if_bdr == prev_bdr)
	    break;
	else if (if_bdr == if_addr || prev_bdr == if_addr)
	    continue;
	else
	    break;
    }	

    // Set the appropriate state
    if (if_dr == if_addr)
	if_state = IFS_DR;
    else if (if_bdr == if_addr)
	if_state = IFS_BACKUP;
    else
	if_state = IFS_OTHER;

    // Act on changes
    if (if_dr != prev_dr || if_bdr != prev_bdr) {
        if (ospf->spflog(LOG_DRCH, 4)) {
	    ospf->log(this);
	    ospf->log(" DR ");
	    ospf->log(&if_dr);
	    ospf->log(" Back ");
	    ospf->log(&if_bdr);
	}
	ifa_allnbrs_event(NBE_EVAL);
	if (if_dr != prev_dr)
	    if_area->rl_orig();
    }
}

/* Reset all interface variables to their default condition.
 * Stop all interface timers, and destroy all associated
 * neighbors.
 */

void SpfIfc::ifa_reset()

{
    if (!is_virtual())
	ospf->app_leave(if_phyint, AllSPFRouters); 
    if_dr = UnknownAddr;
    if_bdr = UnknownAddr;
    ospf->ospf_freepkt(&if_update);
    ospf->ospf_freepkt(&if_dack);
    if_wtim.stop();
    stop_hellos();
    if_actim.stop();
    ifa_allnbrs_event(NBE_DESTROY);
}


/* Routines that are not called directly by the interface FSM, but are
 * instead called by the action routines.
 */

/* Invoke the neighbor FSM for each attached neighbor with a
 * given event.
 */

void SpfIfc::ifa_allnbrs_event(int event)

{
    NbrIterator iter(this);
    SpfNbr *np;

    while ((np = iter.get_next()))
	np->nbr_fsm(event);
}

/* Invoke the start event on all neighbors whose Router Priority
 * is greater than or equal to the passed value. This processing
 * is only performed on non-broadcast interfaces. These are detected
 * by seeing whether the interface's Hello Timer is running.
 */

void SpfIfc::ifa_nbr_start(int base_priority)

{
    NbrIterator iter(this);
    SpfNbr *np;

    while ((np = iter.get_next())) {
	if (np->priority() >= base_priority)
	    np->nbr_fsm(NBE_START);
    }
}

/* Wait timer has fired. Time to run the first DR election.
 */

void WaitTimer::action()

{
    ip->run_fsm(IFE_WTIM);
}
