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
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <netinet/ip_mroute.h>

#include "ospf_module.h"

#include "ospfinc.h"
#include "monitor.h"
#include "system.h"

#include "tcppkt.h"
#include "os-instance.h"
#include "ospfd_xorp.h"


/* Send an OSPF packet. If it is to be sent out
 * a serial lines, or to a specific IP next hop, adjust
 * the destination IP address to make the UNIX kernel happy.
 */

void XorpOspfd::sendpkt(InPkt *pkt, int phyint, InAddr gw)

{
    debug_msg("----\nsendpkt phyint=%d gw=%x\n", phyint, gw);
    BSDPhyInt *phyp;
    msghdr msg;
    iovec iov;
    size_t len;
    sockaddr_in to;

    phyp = phys[phyint];

    if (phyp->flags & IFF_POINTOPOINT)
	pkt->i_dest = hton32(phyp->dstaddr);
    else if (gw != 0)
	pkt->i_dest = hton32(gw);
    debug_msg("pkt->i_dest = %x\n", hton32(pkt->i_dest));
    pkt->i_chksum = ~incksum((uns16 *)pkt, sizeof(pkt));

    if (IN_CLASSD(ntoh32(pkt->i_dest))) {
	debug_msg("it's multicast!\n");
	in_addr mreq;
	mreq.s_addr = hton32(phyp->addr);
	debug_msg("mreq.s_addr = %x\n", phyp->addr);
	if (setsockopt(_netfd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&mreq, sizeof(mreq)) < 0) {
	    XLOG_ERROR("IP_MULTICAST_IF phyint %d: %s", 
		       phyint, strerror(errno));
	    return;
	}
    }

    len = ntoh16(pkt->i_len);
    debug_msg("len=%d, pkt->i_len=%d\n", len, pkt->i_len);

    //XXX  FreeBSD needs IP len in *host* byte order
    pkt->i_len = len;

    bzero(&to, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_len = sizeof(struct sockaddr_in);
    to.sin_addr.s_addr = pkt->i_dest;

    bzero(&msg, sizeof(msg));
    msg.msg_name = (caddr_t) &to;
    msg.msg_namelen = sizeof(to);
    iov.iov_len = len;
    iov.iov_base = (char *)pkt;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = MSG_DONTROUTE;
#if 1
    msg.msg_control = 0;
    msg.msg_controllen = 0;
#else
    byte cmsgbuf[128];
    cmsghdr *cmsg;
    in_pktinfo *pktinfo;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = CMSG_SPACE(sizeof(in_pktinfo));
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));
    cmsg->cmsg_level = SOL_IP;
    cmsg->cmsg_type = IP_PKTINFO;
    pktinfo = (in_pktinfo *) CMSG_DATA(cmsg);
    pktinfo->ipi_ifindex = phyint;
    pktinfo->ipi_spec_dst.s_addr = 0;
#endif
    if (sendmsg(_netfd, &msg, /*MSG_DONTROUTE*/0) == -1)
	XLOG_ERROR("sendmsg failed: %s", strerror(errno));
    debug_msg("done sending\n");
}

/* Send an OSPF packet, interface not specified.
 * This is used for virtual links.
 */

void XorpOspfd::sendpkt(InPkt *pkt)

{
    debug_msg("----\nsendpkt (virtual links)\n");
    size_t len;
    sockaddr_in to;

    len = ntoh16(pkt->i_len);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = pkt->i_dest;
    if (sendto(_netfd, pkt, len, 0, (sockaddr *) &to, sizeof(to)) == -1)
	XLOG_ERROR("sendto failed: %s", strerror(errno));
}

/* Return whether or not a physical interface is
 * operational. On UNIX, the best we can do is check the
 * interface flags.
 */

bool XorpOspfd::phy_operational(int phyint)

{
    debug_msg("----\nphy_operational %d\n", phyint);
    BSDPhyInt *phyp;

    if (phyint == -1)
        return(false);
    if (!(phyp = phys[phyint]))
        return(false);
    return((phyp->flags & IFF_UP) != 0);
}

/* Open a network interface for the sending and receiving of OSPF
 * packets. We're just using a single raw socket, so this
 * is a no-op.
 */

void XorpOspfd::phy_open(int)

{
    debug_msg("----\nphy_open\n");
}

/* Closing a particular network interface is also a no-op.
 */

void XorpOspfd::phy_close(int)

{
    debug_msg("----\nphy_close\n");
}

/* Join a particular multicast group on a particular interface.
 * Executed when we become DR/Backup, or when an interface first
 * comes up.
 */

void XorpOspfd::join(InAddr group, int phyint)

{
    debug_msg("----\njoin\n");
#ifdef HAVE_ST_IP_MREQN
    ip_mreqn mreq;
#else
    ip_mreq mreq;
#endif
    BSDPhyInt *phyp;

    phyp = phys[phyint];
    if ((phyp->flags & IFF_MULTICAST) == 0)
	return;
    mreq.imr_multiaddr.s_addr = hton32(group);
#ifdef HAVE_ST_IP_MREQN
    mreq.imr_ifindex = phyint;
    mreq.imr_address.s_addr = 0;
#else
    mreq.imr_interface.s_addr = hton32(phyp->addr);
#endif

    if (setsockopt(_netfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		   (char *)&mreq, sizeof(mreq)) < 0)
	XLOG_ERROR("Join error, phyint %d: %s", phyint, strerror(errno));
}

/* Leave a particular multicast group, again on a given
 * interface.
 */

void XorpOspfd::leave(InAddr group, int phyint)

{
    debug_msg("----\nleave\n");
#ifdef HAVE_ST_IP_MREQN
    ip_mreqn mreq;
#else
    ip_mreq mreq;
#endif
    BSDPhyInt *phyp;

    phyp = phys[phyint];
    if ((phyp->flags & IFF_MULTICAST) == 0)
	return;
    mreq.imr_multiaddr.s_addr = hton32(group);
#ifdef HAVE_ST_IP_MREQN
    mreq.imr_ifindex = phyint;
    mreq.imr_address.s_addr = 0;
#else
    mreq.imr_interface.s_addr = hton32(phyp->addr);
#endif

    if (setsockopt(_netfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		   (char *)&mreq, sizeof(mreq)) < 0)
	XLOG_ERROR("Leave error, phyint %d: %s", phyint, strerror(errno));
}

/* Enable or disable IP forwarding.
 */

void XorpOspfd::ip_forward(bool enabled)

{
    if (enabled)
	debug_msg("----\nip_forward (enable)\n");
    else
	debug_msg("----\nip_forward (disable)\n");
    int mib[4];
    int value;

    mib[0] = CTL_NET;
    mib[1] = PF_INET;
    mib[2] = 0; /* IP */
    mib[3] = IPCTL_FORWARDING;
    value = (enabled ? 1 : 0);
    sysctl(&mib[0], 4, 0, 0, &value, sizeof(value));
}
    
/* Enable or disable multicast forwarding globally.
 */

void XorpOspfd::set_multicast_routing(bool enabled)

{
    debug_msg("----\nset_multicast_routing\n");
    int on;

    if (enabled) {
        if ((_igmpfd = socket(AF_INET, SOCK_RAW, PROT_IGMP)) == -1) {
	    XLOG_ERROR("IGMP socket open failed: %s", strerror(errno));
	    return;
	}
#if 0
	// Request notification of receiving interface
	int pktinfo = 1;
	setsockopt(_igmpfd, IPPROTO_IP, IP_PKTINFO, &pktinfo, sizeof(pktinfo));
#endif
	on = 1;
	if (setsockopt(_igmpfd, IPPROTO_IP, MRT_INIT, &on, sizeof(on)) == -1){
	    XLOG_ERROR("MRT_INIT failed: %s", strerror(errno));
	    return;
	}
	if (_eventloop.add_selector(_igmpfd, SEL_RD,
		     callback(this, &XorpOspfd::raw_receive))) {
	    XLOG_ERROR("IGMP failed to add selector.");
	}
    }
    else {
	_eventloop.remove_selector(_igmpfd);

	on = 0;
	if (setsockopt(_igmpfd, IPPROTO_IP, MRT_DONE, &on, sizeof(on)) == -1){
	    XLOG_ERROR("MRT_DONE failed: %s", strerror(errno));
	    return;
	}

	close(_igmpfd);
	_igmpfd = -1;
    }

}

/* Enable or disable multicast forwarding on a given
 * interface.
 */

void XorpOspfd::set_multicast_routing(int phyint, bool enabled)

{
    debug_msg("----\nset_multicast_routing (pre interface)\n");
    vifctl vif;
    BSDPhyInt *phyp;
    int optname;

    phyp = phys[phyint];
    if (_igmpfd == -1)
        return;
    if ((phyp->flags & IFF_MULTICAST) == 0)
	return;
    vif.vifc_vifi = phyint;
    vif.vifc_flags = 0;
    vif.vifc_threshold = 1;
    vif.vifc_rate_limit = 0;
    vif.vifc_lcl_addr.s_addr = hton32(phyp->addr);
    vif.vifc_rmt_addr.s_addr = 0;
    optname = (enabled ? MRT_ADD_VIF : MRT_DEL_VIF);
    if (setsockopt(_igmpfd, IPPROTO_IP, optname, &vif, sizeof(vif)) == -1)
        XLOG_ERROR("MRT_ADD/DEL_VIF failed: %s", strerror(errno));
}


/* Add/modify a kernel routing table entry.
 * Multipath is not supported, so just use the first path
 * found in the routing table entry.
 * If there is no next hop pointer, it means that there
 * is either a) and unresolved next hop or b) and entry
 * whose interface has been deleted, but the router-LSA
 * not yet reoriginated. In either case, just delete
 * the routing table entry.
 */

void XorpOspfd::rtadd(InAddr net, InMask mask, MPath *mpp, 
		     MPath *ompp, bool)

{
    debug_msg("----\nrtadd\n");
    if (directs.find(net, mask))
	return;
    if (!mpp) {
	rtdel(net, mask, ompp);
	return;
    }
    if (ompp)
	rtdel(net, mask, ompp);

    IPv4 dst(htonl(net));
    IPv4 maskaddr(htonl(mask));
    int prefix_len = maskaddr.masklen();
    IPv4 gateway;

    if (mpp) {
	InAddr gw;
	BSDPhyInt *phyp=0;
	gw = mpp->NHs[0].gw;
	if (mpp->NHs[0].phyint != -1)
	    phyp = phys[mpp->NHs[0].phyint];
	if (phyp && (phyp->flags & IFF_POINTOPOINT) != 0)
	    gw = phyp->dstaddr;
	gateway = IPv4(htonl(gw));
	IPv4Net subnet(dst, prefix_len);
	_rib_client.send_add_route4("rib", "ospf",
				    /* unicast */ true,
				    /* multicast */ false,
				    subnet, gateway,
				    /* metric XXXX */255,
				    callback(this,
					     &XorpOspfd::rib_add_delete_route_cb,
					     "add"));

    }
}

/* Delete a kernel routing table entry.
 */

void XorpOspfd::rtdel(InAddr net, InMask mask, MPath *)

{
    debug_msg("----\nrtdel\n");
    if (directs.find(net, mask))
	return;

    IPv4 dst(htonl(net));
    IPv4 maskaddr(htonl(mask));
    int prefix_len = maskaddr.masklen();
    IPv4Net subnet(dst, prefix_len); 
    _rib_client.send_delete_route4("rib", "ospf",
				   /* unicast */ true,
				   /* multicast */ false,
				   subnet,
				   callback(this,
					    &XorpOspfd::rib_add_delete_route_cb,
					    "delete"));
}

/* Add a multicast routing table entry to the kernel.
 */

void XorpOspfd::add_mcache(InAddr, InAddr, MCache *)

{
}

/* Delete a multicast routing table entry from the kernel.
 */

void XorpOspfd::del_mcache(InAddr, InAddr)

{
}

/* Return the printable name of a physical interface.
 */

char *XorpOspfd::phyname(int phyint)

{
    debug_msg("----\nphyname %d\n", phyint);
    BSDPhyInt *phyp;

    phyp = phys[phyint];
    return((char *) phyp->key);
}

/* Print an OSPF logging message into the
 * log file.
 */

void XorpOspfd::sys_spflog(int msgno, char *msgbuf)

{
    time_t t;
    tm *tp;

    t = time(0);
    tp = localtime(&t);
    fprintf(_logstr, "%02d:%02d:%02d OSPF.%03d: %s\n",
	    tp->tm_hour, tp->tm_min, tp->tm_sec, msgno, msgbuf);
    fflush(_logstr);
    debug_msg("LOG: %02d:%02d:%02d OSPF.%03d: %s\n",
	    tp->tm_hour, tp->tm_min, tp->tm_sec, msgno, msgbuf);
}

/* Exit the ospfd program, printing a diagnostic message in
 * the process.
 */

void XorpOspfd::halt(int code, const char *string)

{
    XLOG_ERROR("Exiting: %s, code %d", string, code);
    if (code !=  0)
	abort();
    else if (_changing_routerid)
        _change_complete = true;
    else
        exit(0);
}
