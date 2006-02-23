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

/* This is a simple implementation of an mtrace client.
 * We make a number of simplifying assumptions, including:
 *
 *	1. We only fill in the the packet fields necessary
 *	to fill in the path; we don't do any performance
 *	monitoring.
 *	2. We don't support the myriad of options present
 *	in mtrace. Probes are sent as multicasts on
 *	the destination segment (we can do this because
 *	we're running in a simulator). Responses are
 *	always sent to a unicast address belonging
 *	to the simulated router running the mtrace.
 *	3. The "destination" determines the segment on
 *	which we multicast the first probe, but after that
 *	it is ignored.
 *	4. The "no space" error code means everything is OK,
 *	but we just aren't finished.
 *	5. We trace 1 hop in the beginning, then two at a time,
 *	repeating one of the old hops. This allows MOSPF to
 *	multicast the trace packets out all of the incoming interfaces,
 *	yet still tracing "hop-by-hop" so as to obtain the maximum
 *	anount of debugging information.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
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
#include "igmp.h"

extern SimSys *simsys;

/* Calculate the number of bits in a mask.
 */

static int masklen(InMask mask)

{
    int i;
    for (i = 0; i < 32; i++)
	if ((mask & (1 << i)) != 0)
	    break;

    return(32-i);
}

/* Constructor for an mtrace session.
 * If cphyint is not equal to -1, we send probes directly onto
 * the destination network segment.
 */

MTraceSession::MTraceSession(uns16 id, int phyint, MTraceHdr *hp)
  : AVLitem(id, 0)

{
    hdr = *hp;
    chop = 0;	// Current probe destination, when switched to hop-by-hop
    cphyint = phyint;
    hops_from_dest = 0;
    cttl = 1;	// Current max # hops
    iteration = 0;	// Iteration in current probe sequence
    query_id = 0;
}

/* Probe has timed out. Send a timeout message to the controller,
 * and then increment the iteration. As long as we haven't reached
 * the limit, retransmit the probe.
 */

void MTraceSession::action()

{
    simsys->ctl_pkt.queue_xpkt(0, SIM_TRACEROUTE_TMO, index1(), 0);
    if (++iteration <= MAXITER)
        send_query();
    else {
        simsys->mtraces.remove(this);
	delete this;
    }
}

/* Send an MTrace query.
 */

void MTraceSession::send_query()

{
    size_t len;
    SimPktHdr *data;
    InPkt *iphdr;
    MTraceHdr *mthdr;

    if (iteration == 0) {
        TrTtlMsg ttlm;
	ttlm.ttl = -(hops_from_dest++);
	simsys->ctl_pkt.queue_xpkt(&ttlm, SIM_TRACEROUTE_TTL,
				   index1(), sizeof(ttlm));
    }

    query_id = ++(simsys->mtrace_qid);
    len = sizeof(InPkt) + sizeof(MTraceHdr);
    data = (SimPktHdr *) new byte[len+sizeof(SimPktHdr)];
    data->ts = simsys->xmt_stamp;
    data->phyint = hton32(cphyint);

    iphdr = (InPkt *)(data+1);
    iphdr->i_vhlen = IHLVER;
    iphdr->i_tos = 0;
    iphdr->i_ffo = 0;
    iphdr->i_prot = PROT_IGMP;
    iphdr->i_len = hton16(len);
    iphdr->i_id = 0;
    iphdr->i_ttl = DEFAULT_TTL;
    iphdr->i_src = hton32(ospf->ip_source(hdr.dest));
    iphdr->i_dest = (cphyint != -1) ? hton32(IGMP_ALL_ROUTERS) : hton32(chop);
    iphdr->i_chksum = 0;	// Don't bother

    mthdr = (MTraceHdr *)(iphdr+1);
    mthdr->igmp_type = IGMP_MTRACE_QUERY;
    mthdr->n_hops = cttl;
    mthdr->ig_xsum = 0;		// Don't bother
    mthdr->group = hton32(hdr.group);
    mthdr->src = hton32(hdr.src);
    mthdr->dest = hton32(hdr.dest);
    mthdr->respond_to = iphdr->i_src;
    mthdr->ttl_qid = hton32(query_id);

    if (cphyint != -1)
        simsys->send_multicast(data, len, true);
    else
        simsys->sendpkt(iphdr);
    delete [] ((byte *) data);
    start(Timer::SECOND, false);
}

/* IGMP packet has been received on a particular physical
 * interface. Demultiplex on type, performing mtrace processing
 * on mtrace queries and replies.
 */

void SimSys::igmp_demux(int phyint, InPkt *pkt)

{
    int len;
    IgmpPkt *igmppkt;

    if (!ospf)
        return;

    len = ntoh16(pkt->i_len);
    igmppkt = (IgmpPkt *)(pkt + 1);
    switch(igmppkt->ig_type) {
      case IGMP_MTRACE_QUERY:
	mtrace_query_received(phyint, pkt);
	break;
      case IGMP_MTRACE_RESPONSE:
	mtrace_response_received(pkt);
	break;
      default:
	ospf->rxigmp(phyint, pkt, len);
	break;
    }
}

/* Have received an mtrace query. Lookup the corresponding
 * cache entry, and then decide whether to fill in an entry
 * and forward it on, respond with an error to the response
 * address, or drop it silently.
 */

void SimSys::mtrace_query_received(int phyint, InPkt *pkt)

{
    MTraceHdr *mthdr;
    MCache *ce;
    MtraceBody *fill;
    InAddr src;
    InAddr group;
    InAddr nbr_addr;
    bool multicast=IN_CLASSD(ntoh32(pkt->i_dest));
    int n_filled;
    MtraceBody *first_block;
    int plen=ntoh16(pkt->i_len);
    InPkt *reply;
    byte fwd_code;
    byte thresh;
    SimPktHdr *data;
    int i;
    char temp[80];

    mthdr = (MTraceHdr *)(pkt + 1);
    sprintf(temp, "received IGMP type %x", mthdr->igmp_type);
    sys_spflog(ERR_SYS, temp);
    fwd_code = MTRACE_NO_ERROR;
    nbr_addr = mthdr->dest;
    first_block = (MtraceBody *)(mthdr + 1);
    n_filled = (plen - sizeof(InPkt) - sizeof(MTraceHdr))/sizeof(MtraceBody);
    src = ntoh32(mthdr->src);
    group = ntoh32(mthdr->group);
    if (n_filled != 0) {
        MtraceBody *last_block;
	last_block = first_block + (n_filled - 1);
	nbr_addr = ntoh32(last_block->incoming_addr);
    }

    // Look up cache entry
    ce = ospf->mclookup(src, group);
    if (!ce)
        fwd_code = MTRACE_NO_ROUTE;
    else if (ce->n_upstream == 0)
        fwd_code = MTRACE_NO_MULTICAST;
    else if (multicast && !ce->valid_outgoing(phyint, nbr_addr, thresh))
        fwd_code = MTRACE_WRONG_IF;

    if (multicast && fwd_code != MTRACE_NO_ERROR)
	return;

    if (thresh == 255)
        fwd_code = MTRACE_PRUNE_RCVD;
    data = (SimPktHdr *) new byte[plen+sizeof(SimPktHdr)+ sizeof(MtraceBody)];
    data->ts = simsys->xmt_stamp;
    reply = (InPkt *) (data+1);
    reply->i_vhlen = IHLVER;
    reply->i_tos = 0;
    reply->i_ffo = 0;
    reply->i_prot = PROT_IGMP;
    reply->i_len = hton16(plen+sizeof(MtraceBody));
    reply->i_id = 0;
    reply->i_ttl = DEFAULT_TTL;
    reply->i_src = hton32(ospf->ip_source(src));
    reply->i_dest = hton32(IGMP_ALL_ROUTERS);
    reply->i_chksum = 0;	// Don't bother
    memcpy(reply+1, mthdr, plen - sizeof(InPkt));
    fill = (MtraceBody *)(((byte *)reply) + plen);
    fill->arrival_time = 0;
    fill->incoming_addr = 0;
    fill->outgoing_addr = 0;
    if (multicast) {
	fill->outgoing_addr = hton32(ospf->if_addr(phyint));
	if (!fill->outgoing_addr)
	    fill->outgoing_addr = hton32(ospf->ip_source(nbr_addr));
    }
    fill->prev_router = hton32(IGMP_ALL_ROUTERS);
    fill->incoming_pkts = 0;
    fill->outgoing_pkts = 0;
    fill->fwd_count = 0;
    fill->prot = MTRACE_MOSPF;
    fill->fwd_ttl = thresh;
    fill->source_mask = masklen(ce->mask);
    fill->fwd_code = fwd_code;

    /* If there are no errors, and the packet is not
     * yet full, and we haven't reached the source,
     * forward to the previous hop.
     * If you instead send to the respond-to address, must change
     * the destination of the packet, and the IGMP type.
     */

    bool trace_done = false;
    if (ce->n_upstream != 0) {
        PhyintMap *phyp;
        phyint = ce->up_phys[0];
	if (phyint == -1)
	    trace_done = true;
	else if ((phyp = (PhyintMap *)port_map.find(phyint, 0)) &&
		 (phyp->addr != 0) && (phyp->mask != 0) &&
		 (src & phyp->mask) == (phyp->addr & phyp->mask))
	    trace_done = true;
    }

    if ((fwd_code != MTRACE_NO_ERROR) ||
	(mthdr->n_hops == n_filled+1) ||
	(trace_done)) {
        MTraceHdr *reply_mthdr;
        reply->i_dest = mthdr->respond_to;
	reply_mthdr = (MTraceHdr *)(reply+1);
	reply_mthdr->igmp_type = IGMP_MTRACE_RESPONSE;
        phyint = ce->up_phys[0];
	if (phyint != -1)
	    fill->incoming_addr = hton32(ospf->if_addr(phyint));
	if (!fill->incoming_addr)
	    fill->incoming_addr = hton32(ospf->ip_source(src));;
	if (!trace_done &&
	    (fwd_code == MTRACE_NO_ERROR) &&
	    (mthdr->n_hops == n_filled+1))
	    fill->fwd_code = MTRACE_NO_SPACE;
	sendpkt(reply);
        delete [] ((byte *) data);
	return;
    }

    for (i=0; i < ce->n_upstream; i++) {
        phyint = ce->up_phys[i];
	if (phyint == -1)
	    continue;
	fill->incoming_addr = hton32(ospf->if_addr(phyint));
	if (!fill->incoming_addr)
	    fill->incoming_addr = hton32(ospf->ip_source(src));;
	data->phyint = hton32(phyint);
        simsys->send_multicast(data, plen + sizeof(MtraceBody));
    }
    delete [] ((byte *) data);
}

/* Convert an mtrace multicast routing protocol
 * type into a printable string.
 */

char *mtrace_prot(int code)

{
    switch(code) {
      case MTRACE_DVMRP:
	return("DVMRP");
      case MTRACE_MOSPF:
	return("MOSPF");
      case MTRACE_PIM:
	return("PIM");
      case MTRACE_CBT:
	return("CBT");
      case MTRACE_PIM_SPECIAL:
	return("PIM/Special");
      case MTRACE_PIM_STATIC:
	return("PIM/Static");
      case MTRACE_DVMRP_STATIC:
	return("DVMRP/Static");
      case MTRACE_PIM_MBGP:
	return("PIM/BGP4+");
      case MTRACE_CBT_SPECIAL:
	return("CBT/Special");
      case MTRACE_CBT_STATIC:
	return("CBT/Static");
      case MTRACE_PIM_ASSERT:
	return("PIM/Assert");
      default:
	return("????");
    }

    return("????");
}

/* Convert an mtrace forwarding error code into a
 * printable string.
 */

char *mtrace_fwdcode(int code)

{
    switch(code) {
      case MTRACE_NO_ERROR:
	return("");
      case MTRACE_WRONG_IF:
	return("Wrong interface");
      case MTRACE_PRUNE_SENT:
	return("Prune sent upstream");
      case MTRACE_PRUNE_RCVD:
	return("Output pruned");
      case MTRACE_SCOPED:
	return("Hit scope boundary");
      case MTRACE_NO_ROUTE:
	return("No route");
      case MTRACE_WRONG_LAST_HOP:
	return("Wrong last hop");
      case MTRACE_NOT_FORWARDING:
	return("Not forwarding");
      case MTRACE_REACHED_RP:
	return("Reached RP/Core");
      case MTRACE_RPF_IF:
	return("RPF Interface");
      case MTRACE_NO_MULTICAST:
	return("Multicast disabled");
      case MTRACE_INFO_HIDDEN:
	return("Info hidden");
      case MTRACE_NO_SPACE:
	return("No space in packet");
      case MTRACE_OLD_ROUTER:
	return("Next router no mtrace");
      case MTRACE_ADMIN_PROHIB:
	return("Admin. prohibited");
      default:
	return("Unknown error");
    }

    return("");
}

/* Mtrace response has been received. After finding the
 * correct session, print some lines into the mtrace window
 * and decide whether to go on.
 */

void SimSys::mtrace_response_received(InPkt *pkt)

{
    MTraceHdr *mthdr;
    char temp[80];
    MtraceBody *last_block;
    InAddr src;
    InAddr group;
    int n_filled;
    MtraceBody *first_block;
    MTraceSession *session;
    AVLsearch iter(&mtraces);
    uns32 query_id;
    int plen=ntoh16(pkt->i_len);

    mthdr = (MTraceHdr *)(pkt + 1);
    sprintf(temp, "received IGMP type %x", mthdr->igmp_type);
    simsys->sys_spflog(ERR_SYS, temp);

    first_block = (MtraceBody *)(mthdr + 1);
    n_filled = (plen - sizeof(InPkt) - sizeof(MTraceHdr))/sizeof(MtraceBody);
    src = ntoh32(mthdr->src);
    group = ntoh32(mthdr->group);
    last_block = first_block + (n_filled - 1);
    query_id = ntoh32(mthdr->ttl_qid);

    while ((session = (MTraceSession *)iter.next())) {
        if (session->query_id != query_id)
	    continue;
	if (session->hdr.src != src)
	    continue;
	if (session->hdr.group == group)
	    break;
    }

    if (session) {
	// Cancel retransmissions
        session->stop();
	// Print this hop
	session->print(src, last_block);
	// Are we done with entire trace?
        if (!session->done(src, last_block)) {
	    session->cttl = 2;
	    session->cphyint = -1;
	    session->chop = ntoh32(last_block->incoming_addr);
	    session->send_query();
	}
	else {
	    mtraces.remove(session);
	    delete session;
	}
    }
}

/* Print the current hop of an Mtrace.
 */

void MTraceSession::print(InAddr src, MtraceBody *last_block)

{
    char output[200];
    int olen;
    in_addr addr;
    byte fwd_code=last_block->fwd_code;
    bool unreachable;

    if (fwd_code == MTRACE_NO_SPACE)
        fwd_code = MTRACE_NO_ERROR;
    unreachable = (fwd_code == MTRACE_NO_ROUTE);

    addr.s_addr = last_block->outgoing_addr;
    sprintf(output, "%s %s ", inet_ntoa(addr), mtrace_prot(last_block->prot));
    if (fwd_code == MTRACE_NO_ERROR)
        sprintf(output+strlen(output), "thresh^ %d ", last_block->fwd_ttl);
    sprintf(output+strlen(output), "%s ", mtrace_fwdcode(fwd_code));
    if (!unreachable) {
        uns32 mask;
	mask = ~((1 << (32 - last_block->source_mask)) - 1);
	addr.s_addr = hton32(src & mask);
	sprintf(output+strlen(output), "\\[%s/%d\\]",
		inet_ntoa(addr),
		last_block->source_mask);
	}
    olen = strlen(output)+1;
    simsys->ctl_pkt.queue_xpkt(output, SIM_PRINT_SESSION, index1(), olen);
}

/* Has the mtrace session finished?
 */

bool MTraceSession::done(InAddr src, MtraceBody *last_block)

{
    uns32 mask;

    switch (last_block->fwd_code) {
      case MTRACE_NO_ERROR:
      case MTRACE_PRUNE_SENT:
      case MTRACE_PRUNE_RCVD:
      case MTRACE_SCOPED:
      case MTRACE_NO_SPACE:
	break;
      case MTRACE_WRONG_IF:
      case MTRACE_NO_ROUTE:
      case MTRACE_WRONG_LAST_HOP:
      case MTRACE_NOT_FORWARDING:
      case MTRACE_REACHED_RP:
      case MTRACE_RPF_IF:
      case MTRACE_NO_MULTICAST:
      case MTRACE_INFO_HIDDEN:
      case MTRACE_OLD_ROUTER:
      case MTRACE_ADMIN_PROHIB:
      default:
        return(true);
    }

    /* It is possible that we have hit the source. If so,
     * return true and print a final line into the
     * session window.
     */

    mask = ~((1 << (32 - last_block->source_mask)) - 1);
    if ((src & mask) == (hton32(last_block->incoming_addr) & mask)) {
	TrTtlMsg ttlm;
	char output[200];
	int olen;
	in_addr addr;
        ttlm.ttl = -(hops_from_dest);
	simsys->ctl_pkt.queue_xpkt(&ttlm, SIM_TRACEROUTE_TTL,
				   index1(), sizeof(ttlm));
	addr.s_addr = hton32(src);
	sprintf(output, "%s", inet_ntoa(addr));
	olen = strlen(output)+1;
	simsys->ctl_pkt.queue_xpkt(output, SIM_PRINT_SESSION, index1(), olen);
	return(true);
    }

    return(false);
}
