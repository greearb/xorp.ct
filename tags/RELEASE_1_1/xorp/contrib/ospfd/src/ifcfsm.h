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

/* Defines used by the OSPF interface state machine.
 * See Sections 9.1 through 9.3 of the OSPF specification.
 */

extern FsmTran IfcFsm[];	// Interface FSM itself

/* The possible OSPF interface states.
 * These are defined in Section 9.1 of the OSPF specification.
 * 0 should not be used as a state, as it is reserved as an
 * indication from action routines that they have not determined
 * the new interface state, and by the FSM transitions to indicate that
 * there has been no state change.
 * Defined as separate bits so that they can be grouped together
 * in FSM transitions.
 */

enum {
    IFS_DOWN = 0x01,	// Interface is down
    IFS_LOOP = 0x02,	// Interface is looped
    IFS_WAIT = 0x04,	// Waiting to learn Backup DR
    IFS_PP  = 0x08,	// Terminal state for P-P interfaces
    IFS_OTHER = 0x10,	// Mult-access: neither DR nor Backup
    IFS_BACKUP = 0x20,	// Router is Backup on this interface
    IFS_DR  = 0x40,	// Router is DR on this interface

    N_IF_STATES	= 7,	/* # OSPF interface states */

    /* Terminal multi-access states */
    IFS_MULTI = (IFS_OTHER | IFS_BACKUP | IFS_DR),
    IFS_ANY = 0x7F,	/* Matches all states */
};

/* OSPF interface events.
 * Defined in Section 9.2 of the OSPF specification.
 * IFE_RECONFIG and IFE_ADM_DOWN have been added for dynamic
 * reconfiguration.
 */

enum {
    IFE_UP  = 1,	// Interface has come up
    IFE_WTIM,		// Wait timer has fired
    IFE_BSEEN,		// Backup DR has been seen
    IFE_NCHG,		// Associated neighbor has changed state
    IFE_LOOP,		// Interface has been looped
    IFE_UNLOOP,		// Interface has been unlooped
    IFE_DOWN,		// Interface has gone down
    // # OSPF interface events
    N_IF_EVENTS	= IFE_DOWN,
};

/* OSPF interface FSM actions.
 * Defined in Section 9.3 of the OSPF specification, and implemented
 * in file ifcfsm.C.
 */

enum {
    IFA_START = 1,	// Start up the interface
    IFA_ELECT,		// Elect the Designated Router
    IFA_RESET,		// Reset all interface variables
};
