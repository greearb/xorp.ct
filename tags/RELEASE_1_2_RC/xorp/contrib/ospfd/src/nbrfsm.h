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

/* Defines used by the OSPF neighbor state machine.
 * See Sections 10.1 through 10.3 of the OSPF specification.
 */

extern FsmTran NbrFsm[];	// Neighbor FSM itself


/* The possible OSPF neighbor states.
 * These are defined in Section 10.1 of the OSPF specification.
 * 0 should not be used as a state, as it is reserved as an
 * indication from action routines that they have not determined
 * the new interface state, and by the FSM transitions to indicate that
 * there has been no state change.
 * Defined as separate bits so that they can be grouped together
 * in FSM transitions.
 */

enum {
    NBS_DOWN = 0x01,	// Neighbor down
    NBS_ATTEMPT = 0x02,	// Attempt to send hellos (NBMA)
    NBS_INIT = 0x04,	// 1-Way communication
    NBS_2WAY = 0x08,	// 2-Way communication
    NBS_EXST = 0x10,	// Negotiate Master/Slave
    NBS_EXCH = 0x20,	// Start sending DD packets
    NBS_LOAD = 0x40,	// DDs done, now only LS reqs
    NBS_FULL = 0x80,	// Full adjacency

    NBS_ACTIVE = 0xFC,	// Any state but down and attempt
    NBS_FLOOD = NBS_EXCH | NBS_LOAD | NBS_FULL,
    NBS_ADJFORM = NBS_EXST | NBS_FLOOD,
    NBS_BIDIR = NBS_2WAY | NBS_ADJFORM,
    NBS_PRELIM = NBS_DOWN | NBS_ATTEMPT | NBS_INIT,
    NBS_ANY = 0xFF,	// All states
};

/* OSPF neighbor events.
 * Defined in Section 10.2 of the OSPF specification.
 */

enum {
    NBE_HELLO = 1,	// Hello Received
    NBE_START,		// Start sending hellos
    NBE_2WAY,		// Bidirectional indication
    NBE_NEGDONE,	// Negotiation of master/slave, seq #
    NBE_EXCHDONE,	// All DD packets exchanged
    NBE_BADLSREQ,	// Bad LS request received
    NBE_LDONE,		// Loading Done
    NBE_EVAL,		// Evaluate whether should be adjacent
    NBE_DDRCVD,		// Received valide DD packet
    NBE_DDSEQNO,	// Bad sequence number in DD process
    NBE_1WAY,		// Only 1-way communication
    NBE_DESTROY,	// Destroy the neighbor
    NBE_INACTIVE,	// No hellos seen recently
    NBE_LLDOWN,		// Down indication from link level
    NBE_ADJTMO,		// Adjacency forming timeout
};

/* OSPF neighbor FSM actions.
 * Defined in Section 10.3 of the OSPF specification, and implemented
 * in file nbrfsm.C.
 */

enum {
    NBA_START	= 1,	// Start up the neighbor
    NBA_RST_IATIM,	// Reset inactivity timer
    NBA_ST_IATIM,	// Start inactivity timer
    NBA_EVAL1,		// Start DD process? (Internal request)
    NBA_EVAL2,		// Start DD process? (External request)
    NBA_SNAPSHOT,	// Take snapshot of LSDB, and start sending
    NBA_EXCHDONE,	// Actions associated with exchange done
    NBA_REEVAL,		// Reevaluate whether nbr should be adj.
    NBA_RESTART_DD,	// Restart DD process
    NBA_DELETE,		// Delete the neighbor
    NBA_CLR_LISTS,	// Clear database lists
    NBA_HELLOCHK,	// Start, stop NBMA hellos
};
