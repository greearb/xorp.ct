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


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ospfinc.h"
#include "monitor.h"
#include "system.h"
#include "tcppkt.h"
#include "machtype.h"
#include "mtrace.h"
#include "ospfd_sim.h"
#include "sim.h"
#include "icmp.h"

/* Send an OSPF packet out a specific interface.
 * Simply queue the packet to be sent when the
 * next timer tick is received.
 */

void SimSys::sendpkt(InPkt *pkt, int phyint, InAddr gw)

{
    InAddr nh;
    size_t len;
    SimPktHdr *data;

    // Calculate next hop	
    if ((nh = hton32(gw)) == 0)
	nh = pkt->i_dest;
    // Figure out socket destination address
    len = ntoh16(pkt->i_len);
    data = (SimPktHdr *) new byte[len+sizeof(SimPktHdr)];
    data->phyint = hton32(phyint);
    data->ts = xmt_stamp;
    memcpy(data+1, pkt, len);
    if (IN_CLASSD(ntoh32(nh)) || nh == (InAddr) -1)
	send_multicast(data, len);
    else {
	uns16 port;
	InAddr owner;
	sockaddr_in to;
	if (!(port = get_port_addr(ntoh32(nh), owner))) {
	    if (nh == pkt->i_dest) {
	        sendicmp(ICMP_TYPE_UNREACH, ICMP_CODE_UNREACH_HOST,
			 0, 0, pkt, 0, 0, 0);
	    }
	    else {
	        char temp[80];
		in_addr addtmp;
		addtmp.s_addr = nh;
		sprintf(temp, "Can't resolve nh %s", inet_ntoa(addtmp));
		sys_spflog(ERR_SYS, temp);
	    }
	}
	else {
	    to.sin_family = AF_INET;
	    to.sin_addr.s_addr = inet_addr(LOOPADDR);
	    to.sin_port = hton16(port);

	    if (sendto(uni_fd, data, len+sizeof(SimPktHdr), 0,
		       (sockaddr *) &to, sizeof(to)) == -1)
	        perror("sendto");
	}
    }

    delete [] ((byte *)data);
}

/* Send an OSPF packet, interface not specified.
 * This is used for virtual links.
 * Must first do a routing table lookup of the destination
 * to figure out the next hop.
 */

void SimSys::sendpkt(InPkt *pkt)

{
    MPath *mpp;
    InAddr gw;

    // Resolve next hop
    if (!ipforwarding)
        return;
    if (IN_CLASSD(ntoh32(pkt->i_dest))) {
	mc_fwd(-1, pkt);
	return;
    }
    if ((mpp = ospf->ip_lookup(ntoh32(pkt->i_dest))) == 0) {
        sendicmp(ICMP_TYPE_UNREACH, ICMP_CODE_UNREACH_HOST,
		 0, 0, pkt, 0, 0, 0);
	sys_spflog(ERR_SYS, "next hop lookup failed");
	return;
    }
    gw = mpp->NHs[0].gw;
    if (gw != 0 && mpp->NHs[0].if_addr == 0)
        gw = (InAddr) -1;
    sendpkt(pkt, mpp->NHs[0].phyint, gw);
}

/* Return whether or not a physical interface is
 * operational. The phyint of 0, which is illegal,
 * is declared operational so that ASEs can be imported
 * with forwarding addresses of 0.0.0.0.
 */

bool SimSys::phy_operational(int phyint)

{
    PhyintMap *phyp;

    if (phyint == 0)
        return(true);
    phyp = (PhyintMap *) port_map.find(phyint, 0);
    return(phyp && phyp->working);
}

/* Open a network interface for the sending and receiving of OSPF
 * packets. 
 * In the simulator this is a no-op, since all packets are received
 * on a single UDP socket (created during initialization
 */

void SimSys::phy_open(int phyint)

{
}

/* Closing a particular network interface is again a
 * no-op.
 */

void SimSys::phy_close(int phyint)

{
}

/* Join a particular multicast group on a particular interface.
 * Executed when we become DR/Backup, or when an interface first
 * comes up.
 */

void SimSys::join(InAddr group, int phyint)

{
    AVLitem *entry;
    entry = new AVLitem(group, phyint);
    membership.add(entry);
}

/* Leave a particular multicast group, again on a given
 * interface.
 */

void SimSys::leave(InAddr group, int phyint)

{
    AVLitem *entry;
    if ((entry = membership.find(group, phyint)))
        membership.remove(entry);
}

/* Enable or disable IP forwarding.
 */

void SimSys::ip_forward(bool enabled)

{
    ipforwarding = enabled;
}
    
/* Enable/disable multicast forwarding on an interface. This mainly
 * involves putting the interface into promiscuous mode.
 */

void SimSys::set_multicast_routing(int phyint, bool enabled)

{
    PhyintMap *phyp;
    if ((phyp = (PhyintMap *)port_map.find(phyint,0)))
	phyp->promiscuous = enabled;
}

/* Enable/disable multicast forwarding globally.
 * A noop in the simulator.
 */

void SimSys::set_multicast_routing(bool)

{
}

/* Add/modify a kernel routing table entry.
 */

void SimSys::rtadd(InAddr net, InMask mask, MPath *mpp, MPath *, bool reject)

{
    SimRte *rte;
    rte = rttbl.add(net, mask);
    rte->reachable = true;
    rte->reject = reject;
    if (mpp) {
	rte->phyint = mpp->NHs[0].phyint;
	rte->if_addr = mpp->NHs[0].if_addr;
	rte->gw = mpp->NHs[0].gw;
    }
}

/* Delete a kernel routing table entry.
 * We don't actually delete them, but instead set the
 * reachability to false - this will cause the lookup to
 * fall back on a less-specific prefix.
 */

void SimSys::rtdel(InAddr net, InMask mask, MPath *)

{
    SimRte *rte;
    rte = rttbl.add(net, mask);
    rte->reachable = false;
}

/* Upload the current set of routing
 * table entries into the OSPF application.
 */

void SimSys::upload_remnants()

{
    SimRte *rte;
    AVLsearch iter(&rttbl.routes);

    while ((rte = (SimRte *)iter.next())) {
        if (rte->reachable)
	    ospf->remnant_notification(rte->net(), rte->mask());
    }
}

/* Add a multicast routing table entry to the kernel.
 */

void SimSys::add_mcache(InAddr, InAddr, MCache *)

{
}

/* Delete a multicast routing table entry from the kernel.
 */

void SimSys::del_mcache(InAddr, InAddr)

{
}

/* Return the printable name of a physical interface.
 */

char *SimSys::phyname(int phyint)

{
    PhyintMap *phyp;

    if (!(phyp = (PhyintMap *) port_map.find(phyint, 0)))
        return(0);
    return(phyp->name);
}

/* Print a logging message. Send it over the
 * control connection to the simulation controller.
 */

void SimSys::sys_spflog(int msgno, char *msgbuff)

{
    ctl_pkt.queue_xpkt(msgbuff, SIM_LOGMSG, msgno, strlen(msgbuff)+1);
}

/* Exit the ospfd program, printing a diagnostic message in
 * the process.
 */

void SimSys::halt(int code, char *string)

{   char buffer[80];

    if (code == 0 && hitless_preparation) {
        hitless_preparation = false;
        hitless_preparation_complete = true;
        return;
    }
    sprintf(buffer, "Exiting: %s, code %d", string, code);
    sys_spflog(ERR_SYS, buffer);
    abort();
}

/* Simulated router has successfully prepared for a hitless
 * restart.
 */

void SimSys::store_hitless_parms(int period, int n, MD5Seq *sns)

{
    time_add(sys_etime, period*Timer::SECOND, &grace_period);
    hitless_preparation = true;
}

