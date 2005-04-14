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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <tcl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "../src/ospfinc.h"
#include "../src/monitor.h"
#include "../src/system.h"
#include "tcppkt.h"
#include "machtype.h"
#include "sim.h"
#include "mtrace.h"
#include "ospfd_sim.h"
#include <time.h>
#include "icmp.h"

rtid_t my_id;
SimSys *simsys;
char buffer[MAX_IP_PKTSIZE];
char *LOOPADDR = "127.0.0.1";
char *BROADADDR = "127.255.255.255";

// External declarations

/* The main simulated OSPF loop. Loops getting messages (packets, timer
 * ticks, configuration messages, etc.) and never returns
 * until the simulated OSPF process is told to exit.
 * Syntax:
 *	ospfd_sim "router_id" "controller_port"
 */

int main(int argc, char *argv[])

{
    int n_fd;
    fd_set fdset;
    fd_set wrset;
    uns16 controller_port;
    char temp[30];
    int stat;
    int ctl_fd;

    // Get command line arguments
    if (argc != 3) {
        printf("syntax: ospfd_mon $router_id $controller_port\n");
	exit(1);
    }
    my_id = ntoh32(inet_addr(argv[1]));
    controller_port = atoi(argv[2]);

    // Start random number generator
    srand(getpid());

    // Connect to simulation controller
    // Keep on trying if we're getting timeouts
    do {
        sockaddr_in ctl_addr;
	if (!(ctl_fd = socket(AF_INET, SOCK_STREAM, 0))) {
	    perror("create control socket");
	    exit(1);
	}

	memset((char *) &ctl_addr, 0, sizeof(ctl_addr));
	ctl_addr.sin_family = AF_INET;
	ctl_addr.sin_addr.s_addr = inet_addr(LOOPADDR);
	ctl_addr.sin_port = hton16(controller_port);
	stat = connect(ctl_fd, (struct sockaddr *) &ctl_addr,sizeof(ctl_addr));
	if (stat < 0) {
	    close(ctl_fd);
	    usleep ((int) ((1000000.0*rand())/(RAND_MAX+1.0)));
	}
    } while (stat < 0);

    // Create simulation environment
    sys = simsys = new SimSys(ctl_fd);
    // Log command arguments
    sprintf(temp, "invoked: ospfd_sim %s %s", argv[1], argv[2]);
    simsys->sys_spflog(ERR_SYS, temp);

    // Main loop, never exits
    while (1) {
        bool ifc_data=false;
	FD_ZERO(&fdset);
	FD_ZERO(&wrset);
	// Flush any logging messages
	if (ospf)
	    ospf->logflush();
	// If hitless restart preparation done, start ospfd again
	if (simsys->hitless_preparation_complete) {
	    simsys->hitless_preparation_complete = false;
	    delete ospf;
	    ospf = 0;
	}
	// Add connection to controller
	n_fd = simsys->ctl_fd;
	FD_SET(simsys->ctl_fd, &fdset);
	if (simsys->ctl_pkt.xmt_pending())
	    FD_SET(simsys->ctl_fd, &wrset);
	// Add our private port
	n_fd = MAX(n_fd, simsys->uni_fd);
	FD_SET(simsys->uni_fd, &fdset);
	// Add monitoring connection
	simsys->mon_fd_set(n_fd, &fdset, &wrset);
	// Wait for I/O
	if (select(n_fd+1, &fdset, &wrset, 0, 0) < 0)
	    simsys->halt(1, "select failed");
	// Process received data
	if (FD_ISSET(simsys->ctl_fd, &wrset))
	    simsys->ctl_pkt.sendpkt();
	// Received OSPF packets?
	if (FD_ISSET(simsys->uni_fd, &fdset)) {
	    ifc_data = true;
	    simsys->process_uni_fd();
	}
	// Instructions from controller?
	if (!ifc_data && FD_ISSET(simsys->ctl_fd, &fdset))
	    simsys->recv_ctl_command();
	// Process monitor queries and responses
	simsys->process_mon_io(&fdset, &wrset);
    }
    exit(0);
}

/* Process packets received on our private socket.
 * These are packets that have been sent directly
 * to us. If the destination address belongs to us,
 * forward it to the demux. Otherwise, do a routing lookup
 * on the packet and forward it further.
 */

void SimSys::process_uni_fd()

{
    int plen;
    sockaddr_in addr;
    socklen fromlen=sizeof(addr);
    SimPktHdr *pkthdr;
    InPkt *pkt;
    SPFtime rcvd;
    SPFtime limit;

    plen = recvfrom(uni_fd, buffer, sizeof(buffer),
		    0, (sockaddr *)&addr, &fromlen);
    pkthdr = (SimPktHdr *) buffer;
    pkt = (InPkt *) (pkthdr+1);
    if (plen < (int) sizeof(SimPktHdr) ||
	plen < (int) (sizeof(SimPktHdr) + ntoh16(pkt->i_len))) {
        perror("recvfrom");
	return;
    }

    rcvd = pkthdr->ts;
    time_add(rcvd, LINK_DELAY, &pkthdr->ts);
    time_add(sys_etime, 1000/TICKS_PER_SECOND, &limit);
    if (time_less(pkthdr->ts, limit))
        rxpkt(pkthdr);
    else
        queue_rcv(pkthdr, plen);
}

/* Process a received packet. Determine whether it is for us.
 * if not, try to forward, otherwise perform the local demux
 * functions.
 */

void SimSys::rxpkt(SimPktHdr *pkthdr)

{
    InPkt *pkt;
    InAddr daddr;
    SimRte *rte;

    pkt = (InPkt *) (pkthdr+1);
    daddr = ntoh32(pkt->i_dest);
    xmt_stamp = pkthdr->ts;
    if (!IN_CLASSD(daddr)) {
	InAddr home;
        if ((!get_port_addr(daddr, home)) || (home != my_id)) {
	    if (!(rte = rttbl.best_match(daddr))) {
	        sendicmp(ICMP_TYPE_UNREACH, ICMP_CODE_UNREACH_HOST,
			 0, 0, pkt, 0, 0, 0);
	    }
	    // Decrement TTL. We don't use checksums in the
	    // IP header of simulated packets
	    else if (pkt->i_ttl == 0 || pkt->i_ttl-- == 1) {
	        sendicmp(ICMP_TYPE_TTL_EXCEED, 0, 0, 0, pkt, 0, 0, 0);
	    }
	    else {
	        InAddr gw;
		gw = rte->gw;
		if (gw != 0 && rte->if_addr == 0)
		    gw = (InAddr) -1;
	        sendpkt(pkt, rte->phyint, gw);
	    }
	}
	else
	    local_demux(pkthdr);
    }
    else {
        mc_fwd(ntoh32(pkthdr->phyint), pkt);
	local_demux(pkthdr);
    }
}

/* Have received an IP packet that is addressed to
 * us (may be a multicast).
 * Fir determine if we are a member of the multicast group,
 * and then dispatch on protocol number.
 */

void SimSys::local_demux(SimPktHdr *pkthdr)

{
    InAddr dest;
    PhyintMap *phyp;
    InPkt *pkt;
    int phyint;

    pkt = (InPkt *) (pkthdr+1);
    phyint = ntoh32(pkthdr->phyint);

    if (!(phyp = (PhyintMap *)port_map.find(phyint,0)))
        return;
    if (phyp->promiscuous && pkt->i_prot == PROT_IGMP) {
        igmp_demux(phyint, pkt);
	return;
    }

    dest = ntoh32(pkt->i_dest);
    if (IN_CLASSD(dest)) {
        if (!membership.find(dest, phyint))
	    return;
    }

    switch(pkt->i_prot) {
      case PROT_OSPF:
	// Discard packet if OSPF not ready
	if (ospf)
	ospf->rxpkt(phyint, pkt, ntoh16(pkt->i_len));
	break;
      case PROT_ICMP:
	IcmpPkt *icmphdr;
	SPFtime *timep;
	IcmpErrMsg emsg;
	uns16 s_id;
	TRSession *tr;
	icmphdr = (IcmpPkt *) (pkt + 1);
	switch (icmphdr->ic_type) {
	  case ICMP_TYPE_ECHO:
	    sendicmp(ICMP_TYPE_ECHOREPLY, 0, 0, 0, pkt, 0, 0, 0);
	    break;
	  case ICMP_TYPE_ECHOREPLY:
	    s_id = ntoh16(icmphdr->ic_id);
	    timep = (SPFtime *) (icmphdr+1);
	    if ((tr = (TRSession *)traceroutes.find(s_id,0))) {
		emsg.src = ntoh32(pkt->i_src);
		emsg.type = icmphdr->ic_type;
		emsg.code = icmphdr->ic_code;
		emsg.msd = time_diff(pkthdr->ts, *timep);
		emsg.icmp_seq = ntoh16(icmphdr->ic_seqno);
		ctl_pkt.queue_xpkt(&emsg, SIM_ICMP_ERROR, s_id, sizeof(emsg));
	        tr->response_received(ICMP_TYPE_ECHOREPLY);
	    }
	    else {
	        EchoReplyMsg msg;
	        msg.src = ntoh32(pkt->i_src);
		msg.msd = time_diff(pkthdr->ts, *timep);
		msg.icmp_seq = ntoh16(icmphdr->ic_seqno);
		msg.ttl = pkt->i_ttl;
		ctl_pkt.queue_xpkt(&msg, SIM_ECHO_REPLY, s_id, sizeof(msg));
	    }
	    break;
	    // ICMP error messages come here!
	  default:
	    InPkt *encap_hdr;
	    s_id = 0;
	    emsg.src = ntoh32(pkt->i_src);
	    emsg.type = icmphdr->ic_type;
	    emsg.code = icmphdr->ic_code;
	    encap_hdr = (InPkt *) (icmphdr +1);
	    if (encap_hdr->i_prot == PROT_ICMP) {
	        IcmpPkt *encap_icmp;
		encap_icmp = (IcmpPkt *) (encap_hdr + 1);
		s_id = ntoh16(encap_icmp->ic_id);
		timep = (SPFtime *) (encap_icmp+1);
		emsg.msd = time_diff(pkthdr->ts, *timep);
		emsg.icmp_seq = ntoh16(encap_icmp->ic_seqno);
	    }
	    ctl_pkt.queue_xpkt(&emsg, SIM_ICMP_ERROR, s_id, sizeof(emsg));
	    if ((tr = (TRSession *)traceroutes.find(s_id,0)))
	        tr->response_received(emsg.type);
	    break;
	}
	break;
      case PROT_IGMP:
	igmp_demux(phyint, pkt);
	break;
      default:
	break;
    }
}


/* Initialize the Simulation environment.
 * Connect to the controller
 * Start the random number generator.
 */

SimSys::SimSys(int fd) : OSInstance(0), ctl_pkt(fd)

{
//  rlimit rlim;
    sockaddr_in addr;
    socklen size;
    SimHello hello;

    grace_period = sys_etime;
    hitless_preparation = false;
    hitless_preparation_complete = false;
    ctl_fd = fd;
    my_addr = 0;
    // Initialize time
    ticks = 0;
    // Allow core files
//  rlim.rlim_max = RLIM_INFINITY;
//  (void) setrlimit(RLIMIT_CORE, &rlim);

    // Open our unicast socket, where people send packets
    // addressed to us (unicasts and multicasts)
    uni_fd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(uni_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	perror("bind fails");
    size = sizeof(addr);
    if (getsockname(uni_fd, (struct sockaddr *) &addr, &size) < 0) {
	perror("getsockname");
	exit (1);
    }
    uni_port = ntohs(addr.sin_port);

    // Identify ourselves to the server
    hello.rtrid = my_id;
    hello.myport = hton16(uni_port);
    ctl_pkt.queue_xpkt(&hello, SIM_HELLO, 0, sizeof(hello));

    // Open monitoring listen socket
    monitor_listen();
    // Set transmission timestamp
    xmt_stamp = sys_etime;
    // Initialize mtrace query IDs
    mtrace_qid = 0;
}

/* Destructor not expected to be called during the life
 * of the program.
 */

SimSys::~SimSys()

{
}

/* Process command received over the controller
 * connection.
 */

void SimSys::recv_ctl_command()

{
    uns16 type;
    uns16 subtype;
    int nbytes;
    char *msg;
    char *end;

    nbytes = ctl_pkt.receive((void **)&msg, type, subtype);
    if (nbytes < 0)
	simsys->halt(1, "Lost controller connection");
    else if (type != 0) {
        AddrMap *addrmap;
	PingSession *ping;
	TRSession *tr;
	PingStartMsg *pm;
	MTraceHdr *mtrm;
	MTraceSession *mtrace;
	HitlessRestartMsg *htlm;
	int phyint;
        end = msg + nbytes;
	xmt_stamp = sys_etime;
        switch (type) {
	    TickBody *tm;
	    GroupMsg *grpm;
	  case SIM_FIRST_TICK:
	    // Init time before any configuration is downloaded
	    tm = (TickBody *) msg;
	    ticks = tm->tick;
	    sys_etime.sec = ticks/TICKS_PER_SECOND;
	    sys_etime.msec = (ticks%TICKS_PER_SECOND) * 1000/TICKS_PER_SECOND;
	    xmt_stamp = sys_etime;
	    // Start OSPF, delayed so that time is initialized
	    // correctly
	    ospf = new OSPF(my_id, sys_etime);
	    break;
	  case SIM_TICK:
	    // Advance time
	    tm = (TickBody *) msg;
	    ticks = tm->tick;
	    sys_etime.sec = ticks/TICKS_PER_SECOND;
	    sys_etime.msec = (ticks%TICKS_PER_SECOND) * 1000/TICKS_PER_SECOND;
	    xmt_stamp = sys_etime;
	    // Process any pending timers
	    if (ospf)
	    ospf->tick();
	    // Process any queued receives
	    process_rcvqueue();
	    // Send tick reponse
	    send_tick_response();
	    break;
          case SIM_CONFIG:
          case SIM_CONFIG_DEL:
	    config(type, subtype, msg);
	    break;
 	  case SIM_SHUTDOWN:
	    if (ospf)
	    ospf->shutdown(10);
	    break;
	  case SIM_ADDRMAP:
	    addrmap = (AddrMap *) msg;
	    for (; (char *)(addrmap + 1) <= end; addrmap++) {
	        AddressMap *entry;
		InAddr addr;
		InAddr home;
		addr = addrmap->addr;
		home = addrmap->home;
	        if (!(entry = (AddressMap *) address_map.find(addr, home))) {
		    entry = new AddressMap(addr, home);
		    address_map.add(entry);
		}
		if (addrmap->port == 0) {
		    address_map.remove(entry);
		    delete entry;
		}
		else {
		    entry->port = (uns16) addrmap->port;
		    entry->home = addrmap->home;
		}
	    }
	    break;
	  case SIM_START_PING:
	    pm = (PingStartMsg *) msg;
	    ping = new PingSession(subtype, pm->src, pm->dest, pm->ttl);
	    pings.add(ping);
	    ping->start(Timer::SECOND, false);
	    break;
	  case SIM_STOP_PING:
	    if ((ping = (PingSession *)pings.find(subtype, 0))) {
	        ping->stop();
		pings.remove(ping);
		delete ping;
	    }
	    break;
	  case SIM_START_TR:
	    pm = (PingStartMsg *) msg;
	    tr = new TRSession(subtype, pm->dest, pm->ttl);
	    traceroutes.add(tr);
	    tr->response_received(255);
	    break;
	  case SIM_ADD_MEMBER:
	    grpm = (GroupMsg *) msg;
	    if (ospf)
	    ospf->join_indication(grpm->group, grpm->phyint);
	    join(grpm->group, grpm->phyint);
	    break;
	  case SIM_DEL_MEMBER:
	    grpm = (GroupMsg *) msg;
	    if (ospf)
	    ospf->leave_indication(grpm->group, grpm->phyint);
	    leave(grpm->group, grpm->phyint);
	    break;
	  case SIM_START_MTRACE:
	    mtrm = (MTraceHdr *) msg;
	    phyint = (int) mtrm->ttl_qid;
	    mtrm->ttl_qid = 0;
	    mtrace = new MTraceSession(subtype, phyint, mtrm);
	    mtraces.add(mtrace);
	    mtrace->send_query();
	    break;
 	  case SIM_RESTART:
	    close_monitor_connections();
	    delete ospf;
	    ospf = 0;
	    // Will then get First tick, and config
	    break;
 	  case SIM_RESTART_HITLESS:
	    htlm = (HitlessRestartMsg *) msg;
	    if (ospf)
	        ospf->hitless_restart(htlm->period, 1);
	    else
	        ospf = new OSPF(my_id, grace_period);
	    break;
	  default:
	    break;
      }
    }
}

/* Configuration request. Dispatch to the correct routine
 * to do specific processing.
 */

void SimSys::config(int type, int subtype, void *msg)

{
    int status;
    
    status = (type == SIM_CONFIG) ? ADD_ITEM : DELETE_ITEM;

    if (!ospf)
        return;

    switch(subtype) {
        PhyintMap *phyp;
	CfgIfc *ifcmsg;
      case CfgType_Gen:
	ospf->cfgOspf((CfgGen *) msg);
	break;
      case CfgType_Area:
	ospf->cfgArea((CfgArea *)msg, status);
	break;
      case CfgType_Range:
	ospf->cfgRnge((CfgRnge *)msg, status);
	break;
      case CfgType_Host:
	ospf->cfgHost((CfgHost *)msg, status);
	break;
      case CfgType_Ifc:
	ifcmsg = (CfgIfc *)msg;
	if (!(phyp = (PhyintMap *)port_map.find(ifcmsg->phyint, 0))) {
	    phyp = new PhyintMap(ifcmsg->phyint);
	    port_map.add(phyp);
	}
	phyp->addr = ifcmsg->address;
	phyp->mask = ifcmsg->mask;
	if (!my_addr)
	    my_addr = ifcmsg->address;
	ospf->cfgIfc(ifcmsg, status);
	break;
      case CfgType_VL:
	ospf->cfgVL((CfgVL *)msg, status);
	break;
      case CfgType_Nbr:
	ospf->cfgNbr((CfgNbr *)msg, status);
	break;
      case CfgType_Route:
	ospf->cfgExRt((CfgExRt *)msg, status);
	break;
      case CfgType_Key:
	ospf->cfgAuKey((CfgAuKey *)msg, status);
	break;
      default:
	break;
    }
}

/* Constructor for the physical interface.
 */

PhyintMap::PhyintMap(int phyint) : AVLitem(phyint,0)

{
    working = true;
    promiscuous = false;
    sprintf(name, "N%d", phyint);
}

/* Send a tick response, listing the current state
 * of the link-state database.
 */

void SimSys::send_tick_response()

{
    DBStats *statp;
    SpfArea *ap;
    SpfArea *low;
    int mlen;

    low = 0;
    statp = new DBStats;
    mlen = sizeof(DBStats);

    if (ospf) {
	AreaIterator iter(ospf);
    statp->n_exlsas = ospf->n_extLSAs();
    statp->ex_dbxsum = ospf->xsum_extLSAs();
    while ((ap = iter.get_next())) {
      if (ap->n_active_if == 0)
	  continue;
      if (!low || low->id() > ap->id())
	  low = ap;
    }
    }
    else {
	statp->n_exlsas = 0;
	statp->ex_dbxsum = 0;
    }

    if (low) {
	statp->area_id = low->id();
	statp->n_lsas = low->n_LSAs();
	statp->dbxsum = low->database_xsum();
    }
    else {
	statp->area_id = 0;
	statp->n_lsas = 0;
	statp->dbxsum = 0;
    }

    ctl_pkt.queue_xpkt(statp, SIM_TICK_RESPONSE, 0, mlen);
    delete statp;
}

/* Queue a received packet, until it is time to process
 * it.
 */

void SimSys::queue_rcv(struct SimPktHdr *pkt, int plen)

{
    SimPktQ *qptr;

    qptr = new SimPktQ;
    qptr->next = 0;
    qptr->ip_data = (SimPktHdr *) new byte[plen];
    memcpy(qptr->ip_data, pkt, plen);
    if (rcv_head == 0)
        rcv_head = rcv_tail = qptr;
    else {
        rcv_tail->next = qptr;
	rcv_tail = qptr;
    }
}


/* After receiving a tick, process the queue of packets
 * that were delayed until this time interval.
 */

void SimSys::process_rcvqueue()

{
    SimPktQ *qptr;
    SimPktQ *next;
    SimPktQ *prev;
    SPFtime limit;

    prev = 0;
    time_add(sys_etime, 1000/TICKS_PER_SECOND, &limit);
    for (qptr = rcv_head; qptr; qptr = next) {
	SimPktHdr *pkthdr;

        next = qptr->next;
	pkthdr = qptr->ip_data;

	// Time to process?
	if (time_less(pkthdr->ts, limit)) {
	    // Remove from receive queue
	    if (rcv_head == qptr)
	        rcv_head = next;
	    else
	        prev->next = next;
	    if (rcv_tail == qptr)
	        rcv_tail = prev;
	    delete qptr;
	    rxpkt(pkthdr);
	    delete [] ((byte *) pkthdr);
	}
	else
	    prev = qptr;
    }
}

/* Forward a multicast datagram.
 * Have MOSPF calculate the forwarding cache entry.
 * If the incoming interface is not -1, and not in the
 * cache entry, just discard. Otherwise, forward out all
 * of the specified output interfaces.
 */

void SimSys::mc_fwd(int in_phyint, InPkt *pkt)

{
    size_t len;
    SimPktHdr *data;
    MCache *ce;

    if (!ospf)
        return;

    len = ntoh16(pkt->i_len);
    data = (SimPktHdr *) new byte[len+sizeof(SimPktHdr)];
    data->ts = xmt_stamp;
    memcpy(data+1, pkt, len);

    // Calculate cache entry
    if ((ce = ospf->mclookup(ntoh32(pkt->i_src), ntoh32(pkt->i_dest)))) {
	// Verify incoming interface
        if (ce->valid_incoming(in_phyint)) {
	    int i;
	    /* If locally originated, send out upstream interface.
	     * will then be forwarded correctly when received
	     * by us and *other* routers.
	     */
	    if (in_phyint == -1 &&
		ce->n_upstream != 0 && ce->up_phys[0] != -1) {
	        data->phyint = hton32(ce->up_phys[0]);
		send_multicast(data, len, true);
	    }
	    else {
	        // Send to all outgoing interfaces, and neighbors
	        for (i = 0; i < ce->n_downstream; i++) {
		    if (pkt->i_ttl <= ce->down_str[i].ttl)
		        continue;
		    if (ce->down_str[i].nbr_addr == 0) {
		        data->phyint = hton32(ce->down_str[i].phyint);
			send_multicast(data, len, true);
		    }
		    else 
		        sendpkt(pkt, ce->down_str[i].phyint, 
				ce->down_str[i].nbr_addr);
		}
	    }
	}
    }

    delete [] ((byte *)data);
}

/* Send a multicast packet. Find all the routers attached
 * to the segment and broadcast to them individually.
 * Router does not send the multicast to itself. This routine
 * is also used to deliver packets over unnumbered point-to-point
 * links.
 */

void SimSys::send_multicast(SimPktHdr *data, size_t len, bool loopback)

{
    int phyint;
    AddressMap *mapp;
    AVLsearch iter(&address_map);
    InPkt *pkt;
    InAddr dest;

    pkt = (InPkt *) (data+1);
    dest = ntoh32(pkt->i_dest);

    phyint = ntoh32(data->phyint);

    iter.seek(phyint, 0);
    while ((mapp = (AddressMap *) iter.next())) {
	sockaddr_in to;
        if (mapp->index1() != (uns32) phyint)
	    break;
	// Loopback ping packets
        if (mapp->home == my_id) {
	    if (loopback)
	       rxpkt(data); 
	    continue;
	}
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = inet_addr(LOOPADDR);
	to.sin_port = hton16(mapp->port);

	if (sendto(uni_fd, data, len+sizeof(SimPktHdr), 0,
		   (sockaddr *) &to, sizeof(to)) == -1)
	  perror("sendto");
	//sys_spflog(ERR_SYS, "sendto failed");
    }
}

/* Return the port that is listening for unicast
 * packets to te simulated router.
 */

uns16 SimSys::get_port_addr(InAddr addr, InAddr &owner)

{
    AddressMap *mapp;
    AVLsearch iter(&address_map);

    iter.seek(addr, 0);
    if (!(mapp = (AddressMap *) iter.next()) ||
	(mapp->index1() != addr))
        return(0);
    owner = mapp->home;
    return (mapp->port);
}

/* Contructor for an entry in the address map, that maps
 * unicast addresses to group addresses to simulate
 * data-link address translation.
 */

AddressMap::AddressMap(InAddr addr, InAddr home) : AVLitem(addr, home)

{
}

/* Construct a ping session.
 */

PingSession::PingSession(uns16 id, InAddr s, InAddr d, byte t) : AVLitem(id, 0)

{
    src = s;
    dest = d;
    ttl = t;
    seqno = 0;
}

/* When the ping session timer fires, we send another ICMP
 * echo.
 */

void PingSession::action()

{
    simsys->sendicmp(ICMP_TYPE_ECHO, 0, src, dest, 0, index1(), seqno++, ttl);
}

/* Construct a traceroute session.
 */

TRSession::TRSession(uns16 id, InAddr d, byte t) : AVLitem(id, 0)

{
    dest = d;
    maxttl = t;
    seqno = 0;
    ttl = 0;
    iteration = MAXITER;
    terminating = false;
}

/* Probe has timed out. Send a timeout message to the controller,
 * and then just act as if a response was received.
 */

void TRSession::action()

{
    simsys->ctl_pkt.queue_xpkt(0, SIM_TRACEROUTE_TMO, index1(), 0);
    response_received(255);
}

/* Traceroute response received. If it is any form of destination
 * unreachable, we are going to end the probes after the
 * current iteration. Otherwise, send a new ping and start
 * the timer again.
 * "type" is the ICMP packet type of any ICMP error received.
 * type == 255 indicates a timeout.
 */

void TRSession::response_received(byte type)

{
    TrTtlMsg ttlm;

    stop();
    if (type == ICMP_TYPE_UNREACH || type == ICMP_TYPE_ECHOREPLY) {
        if (iteration == 0)
          terminating = true;
    } else
        terminating = false;
    // Start of new probe sequence?
    if (iteration++ == MAXITER) {
        if (terminating || ++ttl > maxttl) {
	    simsys->traceroutes.remove(this);
	    delete this;
	    return;
	}
	iteration = 0;
	ttlm.ttl = ttl;
	simsys->ctl_pkt.queue_xpkt(&ttlm, SIM_TRACEROUTE_TTL,
				   index1(), sizeof(ttlm));
    }

    // Send next probe packet
    simsys->sendicmp(ICMP_TYPE_ECHO, 0, 0, dest, 0, index1(), seqno++, ttl);
    start(2*maxttl*10, false);
}

/* When the ping session timer fires, we send another ICMP
 * echo.
 */

void SimSys::sendicmp(byte type, byte code, InAddr src, InAddr dest,
		      InPkt *rcvd, uns16 id, uns16 seqno, byte ttl)

{
    InPkt *iphdr;
    IcmpPkt *icmphdr;
    int len;
    IcmpPkt *r_icmp = 0;
    int iphlen = 0;

    if (rcvd) {
        dest = ntoh32(rcvd->i_src);
        iphlen = (rcvd->i_vhlen & 0xf) << 2;
	r_icmp = (IcmpPkt *) (((byte *) rcvd) + iphlen);
    }

    switch (type) {
      case ICMP_TYPE_ECHO:
	len = sizeof(InPkt) + sizeof(IcmpPkt) + sizeof(SPFtime);
	break;
      case ICMP_TYPE_ECHOREPLY:
	len = ntoh16(rcvd->i_len) - iphlen;
	len += sizeof(InPkt);
	break;
      default:
	// Don't respond to errors with errors
	if (rcvd->i_prot == PROT_ICMP &&
	    r_icmp->ic_type != ICMP_TYPE_ECHO &&
	    r_icmp->ic_type != ICMP_TYPE_ECHOREPLY)
	    return;
	len = MIN(ntoh16(rcvd->i_len), ICMP_ERR_RETURN_BYTES);
	len += sizeof(InPkt) + sizeof(IcmpPkt);
	break;
    }

    iphdr = (InPkt *) new byte[len];
    iphdr->i_vhlen = IHLVER;
    iphdr->i_tos = 0;
    iphdr->i_ffo = 0;
    iphdr->i_prot = PROT_ICMP;
    iphdr->i_len = hton16(len);
    iphdr->i_id = 0;
    iphdr->i_ttl = ((ttl != 0) ? ttl : DEFAULT_TTL);
    iphdr->i_src = src ? hton32(src) : hton32(ip_source(dest));
    iphdr->i_dest = hton32(dest);
    iphdr->i_chksum = 0;	// Don't bother

    icmphdr = (IcmpPkt *) (iphdr + 1);
    icmphdr->ic_type = type;
    icmphdr->ic_code = code;
    icmphdr->ic_chksum = 0;	// Don't bother

    // Fill in body
    switch (type) {
	int rlen;
      case ICMP_TYPE_ECHO:
	SPFtime *timep;
	icmphdr->ic_id = hton16(id);
	icmphdr->ic_seqno = hton16(seqno);
	timep = (SPFtime *) (icmphdr+1);
	*timep = sys_etime;
	break;
      case ICMP_TYPE_ECHOREPLY:
	icmphdr->ic_id = r_icmp->ic_id;
	icmphdr->ic_seqno = r_icmp->ic_seqno;
	rlen = ntoh16(rcvd->i_len) - iphlen - sizeof(IcmpPkt);
	if (rlen > 0)
	    memcpy(icmphdr+1, r_icmp+1, rlen);
	break;
      default:
	rlen = MIN(ntoh16(rcvd->i_len), ICMP_ERR_RETURN_BYTES);
	memcpy(icmphdr+1, rcvd, rlen);
	break;
    }

    if (iphdr->i_ttl-- == 1)
        sendicmp(ICMP_TYPE_TTL_EXCEED, 0, 0, 0, iphdr, 0, 0, 0);
    else
        simsys->sendpkt(iphdr);
    delete [] ((byte *) iphdr);
}

/* The follow two routines taken from ospfd's INtbl
 * and INrte classes.
 * Should make these utility routines.
 */

/* Add an entry to an IP routing table entry. Install the
 * prefix pointers so that the best match operations will
 * work correctly.
 */

SimRte *SimRttbl::add(uns32 net, uns32 mask)

{
    SimRte *rte;
    SimRte *parent;
    SimRte *child;
    AVLsearch iter(&routes);

    if ((rte = (SimRte *) routes.find(net, mask)))
	return(rte);
    // Add to routing table entry
    rte = new SimRte(net, mask);
    routes.add(rte);
    // Set prefix pointer
    parent = (SimRte *) routes.previous(net, mask);
    for (; parent; parent = parent->prefix) {
	if (rte->is_child(parent)) {
	    rte->prefix = parent;
	    break;
	}
    }
    // Set children's parent pointers
    iter.seek(rte);
    while ((child = (SimRte *)iter.next()) && child->is_child(rte)) {
	if (child->prefix && child->prefix->is_child(rte))
	    continue;
	child->prefix = rte;
    }

    return(rte);
}

/* Find the best matching routing table entry for a given
 * IP destination.
 */

SimRte *SimRttbl::best_match(uns32 addr)

{
    SimRte *rte;
    SimRte *prev;

    rte = (SimRte *) routes.root();
    prev = 0;
    while (rte) {
	if (addr < rte->net())
	    rte = (SimRte *) rte->go_left();
	else if (addr > rte->net() ||
		 0xffffffff > rte->mask()) {
	    prev = rte;
	    rte = (SimRte *) rte->go_right();
	}
	else
	    // Matching host route
	    break;
    }

    // If no exact match, take previous entry
    if (!rte)
	rte = prev;
    // Go up prefix chain, looking for valid routes
    for (; rte; rte = rte->prefix) {
	if ((addr & rte->mask()) != rte->net())
	    continue;
	if (rte->reachable)
	    break;
    }

    return((!rte || rte->reject) ? 0 : rte);
}

/* Find the source address that would be used to send
 * packets to the given destination.
 */

InAddr SimSys::ip_source(InAddr dest)

{
    SimRte *rte;

    if ((rte = rttbl.best_match(dest))) {
        if (rte->if_addr != 0)
	    return(rte->if_addr);
    }

    return(my_addr);
}
