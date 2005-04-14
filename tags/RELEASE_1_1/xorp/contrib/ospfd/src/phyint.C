/*
 *   OSPFD routing daemon
 *   Copyright (C) 1999 by John T. Moy
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

/* Routines dealing with physical interfaces.
 * There may be one or more OSPF interfaces defined
 * over each physical interface; OSPF interfaces
 * are operated upon in spfifc.C.
 */

#include "ospfinc.h"
#include "phyint.h"
#include "ifcfsm.h"
#include "system.h"
#include "igmp.h"

/* Constructor for a physical interface.
 */

PhyInt::PhyInt(int phyint) 
  : AVLitem((uns32 ) phyint, 0), qrytim(this), strqtim(this), oqtim(this)

{
    operational = true;
    my_addr = 0;
    mospf_ifp = 0;
    igmp_querier = 0;
    igmp_enabled = false;
    // Initialize IGMP configurable constants to RFC 2236 defaults
    robustness_variable = 2;
    query_interval = 125;
    query_response_interval = 100;// Tenths of seconds
    group_membership_interval = robustness_variable * query_interval;
    // Next is more liberal than spec, to survive Querier changes
    group_membership_interval += (query_response_interval*3)/20;
    other_querier_present_interval = robustness_variable * query_interval;
    other_querier_present_interval += query_response_interval/20;
    startup_query_interval = query_interval/4;
    startup_query_count = robustness_variable;
    last_member_query_interval = 10;// Tenths of seconds
    last_member_query_count = robustness_variable;
}

/* OSPFD application attaches to a physical interface.
 */

void OSPF::phy_attach(int phyint)

{
    PhyInt *phyp;

    if (!(phyp = (PhyInt *)phyints.find((uns32) phyint, 0))) {
	phyp = new PhyInt((uns32) phyint);
	phyints.add(phyp);
	sys->phy_open(phyint);
    }
    
    phyp->ref();
    phyp->verify_igmp_capabilities();
}

/* OSPFD application leaves a group on a physical interface.
 */

void OSPF::phy_detach(int phyint, InAddr if_addr)

{
    PhyInt *phyp;

    if ((phyp = (PhyInt *)phyints.find((uns32) phyint, 0))) {
	phyp->deref();
	if (phyp->not_referenced()) {
	    sys->phy_close(phyint);
	    phyints.remove(phyp);
	    delete phyp;
	}
	else {
	    if (phyp->igmp_querier == if_addr)
	        phyp->igmp_querier = 0;
	    phyp->verify_igmp_capabilities();
	}
    }
}

/* OSPFD application joins a group on a physical interface.
 */

void OSPF::app_join(int phyint, InAddr group)

{
    AVLitem *member;

    if (!(member = ospfd_membership.find((uns32) phyint, group))) {
	member = new AVLitem((uns32) phyint, group);
	ospfd_membership.add(member);
	sys->join(group, phyint);
	if (spflog(LOG_JOIN, 4)) {
	    log(&group);
	    log(" Ifc ");
	    log(sys->phyname(phyint));
	}
    }
    
    member->ref();
}

/* OSPFD application leaves a group on a physical interface.
 */

void OSPF::app_leave(int phyint, InAddr group)

{
    AVLitem *member;

    if ((member = ospfd_membership.find((uns32) phyint, group))) {
	member->deref();
	if (member->not_referenced()) {
	    sys->leave(group, phyint);
	    ospfd_membership.remove(member);
	    member->chkref();
	    if (spflog(LOG_LEAVE, 4)) {
		log(&group);
		log(" Ifc ");
		log(sys->phyname(phyint));
	    }
	}
    }
}

/***************************************************************
 *   An implemention of Version 2 of the IGMP
 *   protocol.
 **************************************************************/

/* Go through all the OSPF interfaces associated with an interface
 * to determine whether we should run IGMP on the interface and,
 * if so, which address we should use in the Group Membership
 * Queries.
 * At least one of the interfaces must be numbered, with MOSPF
 * enabled, for us to send IGMP queries. As for what source address
 * to use, we always use the smallest interface address associated
 * with the interface (which might not be the interface running
 * MOSPF!).
 */

void PhyInt::verify_igmp_capabilities()

{
    bool was_querier;
    bool multicast_routing;
    int phyint = index1();
    bool igmp_was_enabled;

    multicast_routing = false;
    was_querier = IAmQuerier();
    my_addr = 0;
    mospf_ifp = 0;
    igmp_was_enabled = igmp_enabled;
    if (operational) {
	IfcIterator iter(ospf);
	SpfIfc *ip;
	InAddr igmp_addr;
	while ((ip = iter.get_next())) {
            if (ip->if_phyint != phyint)
	        continue;
	    // IGMP allowed?
	    if (!ip->igmp_enabled)
	        continue;
	    // mulicast routing enabled?
	    if (ospf->mospf_enabled() &&
		(ip->if_mcfwd == IF_MCFWD_MC))
		multicast_routing = true;
	    // Address to use for IGMP
	    igmp_addr = (ip->unnumbered() ? ospf->myaddr : ip->if_addr);
	    // Can do IGMP on the interface
	    if (my_addr == 0 || igmp_addr < my_addr)
	        my_addr = igmp_addr;
	    // Running MOSPF too?
	    if (ospf->mospf_enabled() &&
		(ip->if_mcfwd == IF_MCFWD_MC))
		mospf_ifp = ip;
	}
    }

    // Enable/disable multicast routing on interface
    sys->set_multicast_routing(phyint, multicast_routing);
    igmp_enabled = (my_addr && mospf_ifp);
    if (igmp_enabled != igmp_was_enabled) {
        if (igmp_enabled)
	    ospf->app_join(phyint, IGMPAllRouters);
	else
	    ospf->app_leave(phyint, IGMPAllRouters);
    }

    // Should we be querier?
    if (igmp_enabled) {
        if (!igmp_querier || my_addr < igmp_querier ||
	    (was_querier && (my_addr != igmp_querier))) {
	    igmp_querier = my_addr;
	    if (ospf->spflog(LOG_QUERIER, 4)) {
	        ospf->log(&igmp_querier);
	        ospf->log(" Ifc ");
	        ospf->log(sys->phyname(phyint));
	    }
	}
    }

    // Notice IGMP querier state changes
    if (IAmQuerier()) {
        if (!was_querier)
	    start_query_duties();
    }
    else if (was_querier)
	stop_query_duties();
}

/* Start the IGMP querier duties by sending out a sequence
 * of General Queries at a higher rate.
 */

void PhyInt::start_query_duties()

{
    oqtim.stop();
    send_query(0);
    if (startup_query_count > 1) {
        strqtim.startup_queries = startup_query_count - 1;
        strqtim.start(startup_query_interval*Timer::SECOND, false);
    }
}

/* Stop IGMP query duties by stopping any timers which
 * might generate General Queries.
 */

void PhyInt::stop_query_duties()

{
    strqtim.stop();
    qrytim.stop();
}

/* The Startup Query Timer.
 * "startup_queries" will be set right before the timer is started,
 * so that it will have the most recently configured value.
 */

StartupQueryTimer::StartupQueryTimer(PhyInt *p)

{
    phyp = p;
}

void StartupQueryTimer::action()

{
    phyp->send_query(0);
    if (--startup_queries <= 0) {
        stop();
	phyp->qrytim.start(phyp->query_interval*Timer::SECOND, false);
    }
}

/* The regular query timer. Goes off continuously, sending
 * out Host Membership Queries at a regular interval.
 */

IGMPQueryTimer::IGMPQueryTimer(PhyInt *p)

{
    phyp = p;
}

void IGMPQueryTimer::action()

{
    phyp->send_query(0);
}

/* The timer measuring whether we should replace the
 * current querier, when/if it disappears.
 */

IGMPOtherQuerierTimer::IGMPOtherQuerierTimer(PhyInt *p)

{
    phyp = p;
}

void IGMPOtherQuerierTimer::action()

{
    phyp->igmp_querier = 0;
    phyp->verify_igmp_capabilities();
}


/* Send a Host Membership Query.
 * It is a general query if the group is 0.
 * Otherwise, it is a specific query sent in response
 * to a receive leave group message.
 * We're not quite to spec, as we don'y include a
 * router alert option!
 */

void PhyInt::send_query(InAddr group)

{
    InPkt *iphdr;
    IgmpPkt *ig_pkt;
    InAddr dest;
    byte max_response;
    int plen;

    if (!IAmQuerier())
        return;
    // General queries
    if (group == 0) {
        dest = IGMP_ALL_SYSTEMS;
	max_response = query_response_interval;
    }
    // Group-specific queries
    else {
        dest = group;
	max_response = last_member_query_interval;
    }
    plen = sizeof(InPkt) + sizeof(IgmpPkt);
    iphdr = (InPkt *) new byte[plen];
    iphdr->i_vhlen = IHLVER;
    iphdr->i_tos = 0;
    iphdr->i_len = hton16(plen);
    iphdr->i_id = 0;
    iphdr->i_ffo = 0;
    iphdr->i_ttl = 1;
    iphdr->i_prot = PROT_IGMP;
    iphdr->i_chksum = 0;
    iphdr->i_src = hton32(my_addr);
    iphdr->i_dest = hton32(dest);
    iphdr->i_chksum = ~incksum((uns16 *) iphdr, sizeof(*iphdr));
    ig_pkt = (IgmpPkt *) (iphdr+1);
    ig_pkt->ig_type = IGMP_MEMBERSHIP_QUERY;
    ig_pkt->ig_rsptim = max_response;
    ig_pkt->ig_chksum = 0;
    ig_pkt->ig_group = hton32(group);
    ig_pkt->ig_chksum = ~incksum((uns16 *) ig_pkt, sizeof(*ig_pkt));
    sys->sendpkt(iphdr, (int)index1());

    delete [] ((byte *) iphdr);
}

/* Receive an IGMP packet. If it hasn't been
 * associated with an interface by the kernel, we attempt
 * to do so by looking at the source address.
 * Then the packet is dispatched based on its IGMP
 * type.
 */

void OSPF::rxigmp(int phyint, InPkt *pkt, int plen)

{
    IgmpPkt *igmpkt;
    int rcv_err;
    int iphlen;
    InAddr src;
    PhyInt *phyp;
    InAddr group;

    rcv_err = 0;
    iphlen = (pkt->i_vhlen & 0xf) << 2;
    igmpkt = (IgmpPkt *) (((byte *) pkt) + iphlen);
    src = ntoh32(pkt->i_src);

    if (plen < ntoh16(pkt->i_len))
	rcv_err = IGMP_RCV_SHORT;
    else if (ntoh16(pkt->i_len) < (int) (iphlen + sizeof(IgmpPkt)))
	rcv_err = IGMP_RCV_SHORT;
    else if (incksum((uns16 *) igmpkt, sizeof(IgmpPkt)) != 0)
        rcv_err = IGMP_RCV_XSUM;
    else if (phyint == -1) {
        SpfIfc *rip;
        if ((rip = find_nbr_ifc(src)))
	    phyint = rip->if_phyint;
	else
	    rcv_err = IGMP_RCV_NOIFC;
    }
        
    if (rcv_err) {
	if (spflog(rcv_err, 3))
	    log(pkt);
	return;
    }

    if (!(phyp = (PhyInt *)phyints.find((uns32) phyint, 0))) {
	if (spflog(IGMP_RCV_NOIFC, 3))
	    log(pkt);
	return;
    }

    group = ntoh32(igmpkt->ig_group);
    if (spflog(IGMP_RCV, 1)) {
	log(igmpkt->ig_type);
	log(pkt);
    }

    // Dispatch on IGMP packet type
    switch (igmpkt->ig_type) {
      case IGMP_MEMBERSHIP_QUERY:
	phyp->received_igmp_query(src, group, igmpkt->ig_rsptim);
	break;
      case IGMP_V1MEMBERSHIP_REPORT:
	phyp->received_igmp_report(group, true);
	break;
      case IGMP_MEMBERSHIP_REPORT:
	phyp->received_igmp_report(group, false);
	break;
      case IGMP_LEAVE_GROUP:
	phyp->received_igmp_leave(group);
	break;
      default:		// Unexpected packet type
	break;
    }
}

/* Constructor for a group membership entry.
 */

GroupMembership::GroupMembership(InAddr group, int phyint)
: AVLitem(group, phyint), leave_tim(this), exp_tim(this), v1_tim(this)

{
    v1members = false;
}

/* Constructors for the group-membership timers.
 */

LeaveQueryTimer::LeaveQueryTimer(GroupMembership *g)
{
    gentry = g;
}

GroupTimeoutTimer::GroupTimeoutTimer(GroupMembership *g)
{
    gentry = g;
}

V1MemberTimer::V1MemberTimer(GroupMembership *g)
{
    gentry = g;
}

/* Process a received IGMP Host Membership Query.
 * Two things can result. We can let the send become
 * the querier, as a result of having a lower IP address.
 * Also, on queries for specific groups, we can
 * reset the group membership timeout.
 */

void PhyInt::received_igmp_query(InAddr src, InAddr group, byte max_response)

{
    int phyint=(int)index1();
    GroupMembership *gentry=0;

    // Ignore own queries
    if (src == my_addr)
	return;
    // Is packet source the querier now?
    if (!igmp_querier || src <= igmp_querier) {
	stop_query_duties();
	oqtim.stop();
	if (igmp_querier != src && ospf->spflog(LOG_QUERIER, 4)) {
	    ospf->log(&src);
	    ospf->log(" Ifc ");
	    ospf->log(sys->phyname(phyint));
	    }
	igmp_querier = src;
	oqtim.start(other_querier_present_interval*Timer::SECOND, false);
    }
    if (!IAmQuerier() && group != 0 &&
	(gentry = (GroupMembership *) ospf->local_membership.find(group,phyint))) {
        int msec_tmo;
	msec_tmo = last_member_query_count*max_response*100;
	if (msec_tmo < gentry->exp_tim.milliseconds_to_firing()) {
            gentry->exp_tim.stop();
            gentry->exp_tim.start(msec_tmo);
	}
    }
}

/* Process a received Host Membership Report. If
 * necessary, create a new group membership. In any case
 * reset the membership timeouts.
 */

void PhyInt::received_igmp_report(InAddr group, bool v1)

{
    GroupMembership *gentry;
    int phyint = (int) index1();

    if (LOCAL_SCOPE_MULTICAST(group))
        return;
    if (!(gentry = (GroupMembership *) ospf->local_membership.find(group,phyint))){
        ospf->join_indication(group, phyint);
	gentry = (GroupMembership *) ospf->local_membership.find(group,phyint);
    }

    if (v1) {
        gentry->v1members = true;
	gentry->v1_tim.stop();
	gentry->v1_tim.start(group_membership_interval*Timer::SECOND, false);
    }

    gentry->leave_tim.stop();
    gentry->exp_tim.stop();
    gentry->exp_tim.start(group_membership_interval*Timer::SECOND, false);
}

/* Group membership has timed out. Destroy the membership entry
 * and schedule a new group-membership-LSA.
 */

void GroupTimeoutTimer::action()

{
    InAddr group=gentry->index1();
    int phyint=gentry->index2();

    gentry->leave_tim.stop();
    gentry->exp_tim.stop();
    gentry->v1_tim.stop();
    ospf->leave_indication(group, phyint);
}

/* There are no longer an members speaking IGMP Version 1.
 */

void V1MemberTimer::action()

{
    gentry->v1members = false;
}

/* Process a received leave message. If the membership
 * exist, and isn't already in "Checking Membership" state,
 * start the Last Member Query timer.
 */

void PhyInt::received_igmp_leave(InAddr group)

{
    GroupMembership *gentry;
    int phyint = (int) index1();

    if (!IAmQuerier())
        return;
    if (!(gentry = (GroupMembership *) ospf->local_membership.find(group,phyint)))
        return;
    if (gentry->v1members)
        return;
    // Already doing leave processing?
    if (gentry->leave_tim.is_running())
        return;
    // Start the leave timer, and send the first group-specific query
    send_query(group);
    if (last_member_query_count > 1) {
        gentry->leave_tim.queries_remaining = last_member_query_count - 1;
        gentry->leave_tim.start(last_member_query_interval*100, false);
    }
}

/* Leave query timer. If more queries need to be sent, then do
 * so. Otherwise set the expiration timer to something short.
 */

void LeaveQueryTimer::action()

{
    PhyInt *phyp;
    
    phyp = (PhyInt *)ospf->phyints.find(gentry->index2(), 0);
    if (phyp)
        phyp->send_query(gentry->index1());
    if (--queries_remaining <= 0)
	gentry->exp_tim.restart(phyp->last_member_query_interval*100);
}
