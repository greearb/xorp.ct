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

/* Routines dealing with OSPF interfaces.
 * Does not include the Interface state machine, which is
 * contained in file ifcfsm.C.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"
#include "system.h"
#include "contrib/global.h"
#include "contrib/md5.h"

/* Add, modify, or delete an OSPF interface.
 */

void OSPF::cfgIfc(CfgIfc *m, int status)

{
    SpfIfc *ip;
    bool restart=false;
    bool nbr_change=false;
    bool new_lsa=false;
    bool new_net_lsa=false;
    SpfArea *new_ap;
    printf("----\nOSPF::cfgIfc\n");

    ip = find_ifc(m->address, m->phyint);

    // If delete, free to heap
    if (status == DELETE_ITEM) {
	if (ip)
	    delete ip;
	return;
    }

    // Add or modify an interface
    // Type of interface (NBMA, etc.)
    if (ip && ip->type() != m->IfType) {
	delete ip;
	ip = 0;
    }
    // Allocate new interface, if necessary
    if (!ip) {
	switch (m->IfType) {
	  case IFT_BROADCAST:
	    ip = new BroadcastIfc(m->address, m->phyint);
	    break;
	  case IFT_PP:
	    ip = new PPIfc(m->address, m->phyint);
	    break;
	  case IFT_NBMA:
	    ip = new NBMAIfc(m->address, m->phyint);
	    break;
	  case IFT_P2MP:
	    ip = new P2mPIfc(m->address, m->phyint);
	    break;
	  case IFT_VL:
	  default:
	    return;
	}
	ip->if_mask = m->mask;
	ip->if_net = ip->if_addr & ip->if_mask;
	// Queue into global interface list
	ip->next = ospf->ifcs;
	ospf->ifcs = ip;
	if (spflog(CFG_ADD_IFC, 5))
	    log(ip);
    }
    // Allocate new area, if necessary
    if (!(new_ap = FindArea(m->area_id)))
	new_ap = new SpfArea(m->area_id);
    // Change timers, if need be
    if (m->hello_int != ip->if_hint || ip->if_pint != m->poll_int) {
	ip->if_hint = m->hello_int;
	ip->if_pint = m->poll_int;
	ip->restart_hellos();
    }
    // Set new parameters
    ip->if_mtu = m->mtu;		// Interface packet size
    ip->if_xdelay = m->xmt_dly;	// Transit delay (seconds)
    ip->if_rxmt = m->rxmt_int;	// Retransmission interval (seconds)
    ip->if_dint = m->dead_int;	// Router dead interval (seconds)
    ip->if_autype = m->auth_type; // Authentication type
    memcpy(ip->if_passwd, m->auth_key, 8);// Auth key

    // If using MD5, reduce mtu to compensate for appended digest
    if (ip->if_autype == AUT_CRYPT)
	ip->if_mtu -= 16;

    // Interface address mask
    if (ip->if_mask != m->mask) {
	ip->if_mask = m->mask;
	ip->if_net = ip->if_addr & ip->if_mask;
	new_lsa = true;
	new_net_lsa = true;
    }
    // MIB-II IfIndex
    if (ip->if_IfIndex != m->IfIndex) {
	ip->if_IfIndex = m->IfIndex;
	new_lsa = true;
    }
    // Designated router priority
    if (ip->if_drpri != m->dr_pri) {
	ip->if_drpri = m->dr_pri;
	ip->ifa_allnbrs_event(NBE_EVAL);
	nbr_change = true;
    }
    // Interface cost
    // First verify cost
    if (m->if_cost == 0)
        m->if_cost = 1;
    if (ip->if_cost != m->if_cost) {
	ip->if_cost = m->if_cost;
	new_lsa = true;
    }
    // Multicast forwarding enabled?
    if (ip->if_mcfwd != m->mc_fwd) {
	ip->if_mcfwd = m->mc_fwd;
	mospf_clear_cache();
	new_net_lsa = true;
    }
    // On Demand interface?
    if (ip->if_demand != (m->demand != 0)) {
	ip->if_demand = (m->demand != 0);
	restart = true;
    }
    // Passive interface?
    if (ip->if_passive != m->passive) {
	ip->if_passive = m->passive;
	restart = true;
    }
    // IGMP enabled?
    if (ip->igmp_enabled != (m->igmp != 0)) {
        ip->igmp_enabled = (m->igmp != 0);
	restart = true;
    }
    // On Area change, must restart interface
    if (new_ap != ip->if_area) {
	if (ip->if_area) {
	    ip->run_fsm(IFE_DOWN);
	    ip->if_area->RemoveIfc(ip);
	}
	// Add to new area
	ip->if_area = new_ap;
	ip->anext = new_ap->a_ifcs;
	new_ap->a_ifcs = ip;
	if (sys->phy_operational(ip->if_phyint))
	    ip->run_fsm(IFE_UP);
    }
    /* Otherwise,
     * On certain changes, must restart interface:
     * Change in demand status
     */
    else if (restart)
	ip->restart();
    /* Otherwise
     * Change in DR priority causes DR election to be
     * redone. Change in cost or IFIndex causes router-LSA
     * to be regenerated.
     */
    else {
	if (nbr_change)
	    ip->run_fsm(IFE_NCHG);
	if (new_lsa)
	    new_ap->rl_orig();
	if (new_net_lsa)
	    ip->nl_orig(false);
    }
    ip->updated = true;
    new_ap->updated = true;
}

/* Copy Interface info to CfgIfc structure
 */

static void ifc_to_cfg(const SpfIfc& i, struct CfgIfc& msg)
{
    msg.address = i.if_addr;
    msg.phyint = i.if_phyint;
    msg.mask = i.mask();
    msg.mtu = i.mtu();
    msg.IfIndex = i.if_index();
    msg.area_id = i.area()->id();
    msg.IfType = i.type();
    msg.dr_pri = i.drpri();
    msg.xmt_dly = i.xmt_delay();
    msg.rxmt_int = i.rxmt_interval();
    msg.if_cost = i.cost();
    msg.hello_int = i.hello_int();
    msg.dead_int = i.dead_int();
    msg.poll_int = i.poll_int();
    msg.auth_type = i.autype();
    memcpy(msg.auth_key, i.passwd(), sizeof(msg.auth_key));
    msg.mc_fwd = i.mcfwd();
    msg.demand = i.demand();
    msg.passive = i.passive();
    msg.igmp = i.igmp();
}

/* Query Interface
 */

bool OSPF::qryIfc(struct CfgIfc& msg, InAddr address, int phyint) const

{
    const SpfIfc *ip = find_ifc(address, phyint);
    if (ip == 0) return false;
    ifc_to_cfg(*ip, msg);
    return true;
}

/* Get all interfaces
 */
void OSPF::getIfcs(list<CfgIfc>& l) const

{
    IfcIterator iter(this);
    SpfIfc* ip;
    while((ip = iter.get_next())) {
	CfgIfc msg;
	ifc_to_cfg(*ip, msg);
	l.push_back(msg);
    }
}

/* Constructor for an OSPF interface. Set the identifiers
 * for an interface (address and physical interface),
 * set the area to NUL, and then initialize all of the
 * dynamic (non-config) values.
 */

SpfIfc::SpfIfc(InAddr a, int phy)
: if_wtim(this), if_htim(this), if_actim(this)

{
    if_addr = a;
    if_phyint = phy;
    if_area = 0;
    if_keys = 0;
    if_demand = false;
    if_passive = 0;
    igmp_enabled = false;

    db_xsum = 0;
    anext = 0;
    if_dr = 0;
    if_bdr = 0;
    if_dr_p = 0;
    if_state = IFS_DOWN;
    if_nlst = 0;
    if_nnbrs = 0;
    if_nfull = 0;
    if_helping = 0;
    in_recv_update = false;
    area_flood = false;
    global_flood = false;
    if_demand_helapse = 0;
    // Virtual link parameters
    if_tap = 0;		// Transit area
    if_nbrid = 0;	// Configured neighbor ID
    if_rmtaddr = 0;	// IP address of other end
}

/* Destructor for an interface. Declare the interface down,
 * remove from global and area lists, the free associated
 * keys and neighbors.
 */

SpfIfc::~SpfIfc()

{
    SpfIfc **ipp;
    SpfIfc *ip;
    SpfNbr *np;
    NbrIterator nbr_iter(this);
    CryptK *key;
    KeyIterator key_iter(this);

    if (ospf->spflog(CFG_DEL_IFC, 5))
	ospf->log(this);
    // Remove cryptographic keys
    while ((key = key_iter.get_next()))
	delete key;
    // Declare interface down
    run_fsm(IFE_DOWN);
    // Remove neighbors
    while ((np = nbr_iter.get_next()))
	delete np;

    // Free from global list
    for (ipp = &ospf->ifcs; (ip = *ipp) != 0; ipp = &ip->next) {
	if (ip == this) {
	    *ipp = next;
	    break;
	}
    }
    // Free from area list
    if_area->RemoveIfc(this);
    // Close physical interface
    if (if_phyint != -1)
	ospf->phy_detach(if_phyint, if_addr);
}

/* Return the physical interface type.
 */

int SpfIfc::type() const

{
    return(if_type);
}

/* Return a printable string for each interface type
 */

const char *iftypes(int iftype)

{
    switch(iftype) {
      case IFT_BROADCAST:
	return("BCast");
      case IFT_PP:
	return("P-P");
      case IFT_NBMA:
	return("NBMA");
      case IFT_P2MP:
	return("P2MP");
      case IFT_VL:
	return("VL");
      default:
	break;
    }

    return("Unknown");
}

// Is the interface a virtual link?

bool SpfIfc::is_virtual()
{
    return(if_type == IFT_VL);
}

// Can the interface support more than one neighbor?

bool SpfIfc::is_multi_access()
{
    return(if_type != IFT_PP && if_type != IFT_VL);
}

// Return the transit area (0 if not a virtual link)

SpfArea *SpfIfc::transit_area()
{
    return(if_tap);
}

// Return the endpoint of a virtual link

rtid_t *SpfIfc::vl_endpt()
{
    return(&if_nbrid);
}

/* Base functions that may be overriden for specific interface
 * types.
 */

void SpfIfc::start_hellos()
{
    send_hello();
    if_htim.start(if_hint*Timer::SECOND);
}
void SpfIfc::restart_hellos()
{
    if_htim.restart();
}
void SpfIfc::stop_hellos()
{
    if_htim.stop();
}

bool SpfIfc::elects_dr()
{
    return(false);
}

/* Trivial virtual functions for virtual links.
 */

/* Trivial functions for interfaces which elect a Designated
 * Router.
 */

bool DRIfc::elects_dr()
{
    return(true);
}

/* Trivial virtual functions for Broadcast interfaces.
 */

/* Trivial virtual functions for NBMA interfaces.
 */

void NBMAIfc::if_send(Pkt *pdesc, InAddr addr)
{
    nonbroadcast_send(pdesc, addr);
}
void NBMAIfc::start_hellos()
{
    if (if_drpri)
	ifa_nbr_start(1);
}
void NBMAIfc::restart_hellos()
{
    nonbroadcast_restart_hellos();
}
void NBMAIfc::stop_hellos()
{
    nonbroadcast_stop_hellos();
}

/* Trivial virtual functions for Point-to-MultiPoint interfaces.
 */

void P2mPIfc::if_send(Pkt *pdesc, InAddr addr)
{
    nonbroadcast_send(pdesc, addr);
}
void P2mPIfc::start_hellos()
{
    ifa_nbr_start(0);
}
void P2mPIfc::restart_hellos()
{
    nonbroadcast_restart_hellos();
}
void P2mPIfc::stop_hellos()
{
    nonbroadcast_stop_hellos();
}

/* Functions to manipulate loopback interfaces.
 */

LoopIfc::LoopIfc(SpfArea *a, InAddr net, InMask mask) : SpfIfc(net, -1)

{
    if_type = IFT_LOOPBK;
    if_area = a;
    if_mask = mask;
    if_net = if_addr & if_mask;
    if_state = IFS_LOOP;
}

LoopIfc::~LoopIfc()

{
    if_state = IFS_DOWN;
}

void LoopIfc::ifa_start()
{
}

RtrLink *LoopIfc::rl_insert(RTRhdr *, RtrLink *)
{
    return(0);
}

void LoopIfc::add_adj_to_cand(class PriQ &)
{
}

/* If an interface has not been mentioned in a reconfig, just
 * remove it.
 */

void SpfIfc::clear_config()

{
    delete this;
}

/* Restart an interface. Generate the interface down event,
 * and if the interface is physically up, generate the
 * up event as well.
 */

void SpfIfc::restart()

{
    // Declare interface down
    run_fsm(IFE_DOWN);
    if (sys->phy_operational(if_phyint))
	run_fsm(IFE_UP);
}

/* Find an interface data structure given its IP address and,
 * optionally, its physical interface.
 * Only used for real interfaces, and not virtual links.
 */

SpfIfc  *OSPF::find_ifc(uns32 xaddr, int phy) const

{
    IfcIterator iter(this);
    SpfIfc *ip;

    while ((ip = iter.get_next())) {
	if (ip->is_virtual())
	    continue;
	if (phy != -1 && phy != ip->if_phyint)
	    continue;
	if (ip->if_addr == xaddr)
	    break;
    }

    return(ip);
}

/* OSPF packet has been received on a physical interface.
 * Associate the packet with the correct OSPF interface,
 * as described in Section 8.2 of the OSPF specification.
 */

SpfIfc *OSPF::find_ifc(Pkt *pdesc)

{
    SpfPkt *spfpkt;
    IfcIterator iter(this);
    SpfIfc *ip;
    InAddr ipsrc;
    InAddr ipdst;
    VLIfc *vif;
    RTRrte *endpt;
    SpfArea *tap;

    ipsrc = ntoh32(pdesc->iphdr->i_src);
    ipdst = ntoh32(pdesc->iphdr->i_dest);
    spfpkt = pdesc->spfpkt;
    tap = 0;

    while ((ip = iter.get_next())) {
	if (ip->is_virtual())
	    continue;
	// Save interface if dest matches interface
	// address, for later virtual link processing
	if (ipdst == ip->if_addr)
	    tap = ip->if_area;
	if (pdesc->phyint != -1 && ip->if_phyint != pdesc->phyint)
	    continue;
	else if (ip->if_area->a_id != ntoh32(spfpkt->p_aid))
	    continue;
	else if ((ip->is_multi_access() || pdesc->phyint == -1) &&
		 (ipsrc & ip->mask()) != ip->net())
	    continue;
	else if (ipdst == AllDRouters || ipdst == AllSPFRouters)
	    return(ip);
	else if (ipdst == ip->if_addr)
	    return(ip);
    }

    // Real interface not found
    if (tap == 0) {
        AreaIterator aiter(this);
	SpfArea *a;
	while ((a = aiter.get_next())) {
	    HostAddr *loopbk;
	    if ((loopbk = (HostAddr *) a->hosts.find(ipdst,0xffffffff)) &&
		loopbk->r_cost == 0) {
	        tap = a;
		break;
	    }
	}
    }
    if (ntoh32(spfpkt->p_aid) != BACKBONE || tap == 0)
	return(0);

    // Check instead for virtual link
    // Start with endpoint, and see whether a virtual links
    // is configured for the particular transit area

    endpt = tap->find_abr(ntoh32(spfpkt->srcid));
    if (endpt && (vif = endpt->VL))
	return(vif);

    return(0);
}

/* Find the interface to which a configured neighbor
 * attaches.
 */

SpfIfc *OSPF::find_nbr_ifc(InAddr nbr_addr) const

{
    IfcIterator iter(this);
    SpfIfc *ip;

    while ((ip = iter.get_next())) {
	if (!ip->is_multi_access())
	    continue;
	if ((nbr_addr & ip->mask()) == ip->net())
	    return(ip);
    }

    return(0);
}

/* Add or delete an MD5 key. Interface must already be present.
 */

void OSPF::cfgAuKey(struct CfgAuKey *m, int status)

{
    SpfIfc *ip;
    CryptK *key;
    CryptK **prevk;

    if (!(ip = find_ifc(m->address, m->phyint)))
	return;

    // Search for current key
    for (prevk = &ip->if_keys; (key = *prevk); prevk = &key->link) {
	if (key->key_id == m->key_id)
	    break;
    }

    // If delete, free to heap
    if (status == DELETE_ITEM) {
	if (key) {
	    *prevk = key->link;
	    delete key;
	}
	return;
    }

    // Add or modify key
    // If new, allocate and link into list of keys
    if (!key) {
	key = new CryptK(m->key_id);
	key->ip = ip;
	key->link = ip->if_keys;
	ip->if_keys = key;
    }
    // Copy in new data
    memcpy(key->key, m->auth_key, 16);

    key->start_accept		 = m->start_accept;
    key->stop_accept		 = m->stop_accept;
    key->stop_accept_specified	 = m->stop_accept_specified;
    key->start_generate		 = m->start_generate;
    key->stop_generate		 = m->stop_generate;
    key->stop_generate_specified = m->stop_generate_specified;

    key->updated = true;
}

/* Transfer key data to CfgAuKey structure/
 */

static void key_to_cfg(const CryptK& key, CfgAuKey& msg)

{
    msg.address	= key.interface()->if_addr;
    msg.phyint	= key.interface()->if_phyint;
    msg.key_id	= key.id();
    memcpy(msg.auth_key, key.key_data(), sizeof(msg.auth_key));
    msg.start_accept		= key.accept_start();
    msg.stop_accept		= key.accept_stop();
    msg.stop_accept_specified	= key.stop_accept_set();
    msg.start_generate		= key.generate_start();
    msg.stop_generate		= key.generate_stop();
    msg.stop_generate_specified = key.stop_generate_set();
}

/* Get key.
 */

bool OSPF::qryAuKey(struct CfgAuKey& msg,
		    byte	     key_id,
		    InAddr	     address,
		    int		     phyint) const

{
    SpfIfc* ip = find_ifc(address, phyint);
    if (ip == 0) return false;

    CryptK *key;
    CryptK **prevk;

    // Search for current key
    for (prevk = &ip->if_keys; (key = *prevk); prevk = &key->link) {
	if (key->key_id == key_id) {
	    key_to_cfg(*key, msg);
	    return true;
	}
    }
    return false;
}

/* Get all keys associated with interface.
 */

void OSPF::getAuKeys(list<CfgAuKey>& l,
		     InAddr	     address,
		     int	     phyint) const

{
    SpfIfc* ip = find_ifc(address, phyint);
    if (ip == 0) return;

    CryptK *key;
    CryptK **prevk;

    for (prevk = &ip->if_keys; (key = *prevk); prevk = &key->link) {
	CfgAuKey msg;
	key_to_cfg(*key, msg);
	l.push_back(msg);
    }
}

/* Key has not been mentioned in the reconfig, so delete it.
 */

void CryptK::clear_config()

{
    CryptK *ptr;
    CryptK **prevk;

    for (prevk = &ip->if_keys; (ptr = *prevk); prevk = &ptr->link) {
	if (ptr == this)
	    break;
    }

    if (ptr)
	*prevk = link;

    delete this;
}

/* Add the authentication fields to a packet that is to
 * be transmitted. Returns whether the packet needs to
 * be checksummed.
 */

void SpfIfc::generate_message(Pkt *pdesc)

{
    SpfPkt *spfpkt;
    int length;

    spfpkt = pdesc->spfpkt;
    length = ntoh16(spfpkt->plen);

    spfpkt->xsum = 0;
    spfpkt->autype = hton16(if_autype);
    memset(spfpkt->un.aubytes, 0, 8);

    switch (if_autype) {
      case AUT_NONE:	// No authentication
	if (!pdesc->xsummed)
	    spfpkt->xsum = ~incksum((uns16 *) spfpkt, length);
        else
	    spfpkt->xsum = ~incksum((uns16 *) spfpkt, sizeof(SpfPkt),
				    pdesc->body_xsum);
	break;

      case AUT_PASSWD:	// Simple cleartext password
	if (!pdesc->xsummed)
	    spfpkt->xsum = ~incksum((uns16 *) spfpkt, length);
        else
	    spfpkt->xsum = ~incksum((uns16 *) spfpkt, sizeof(SpfPkt),
				    pdesc->body_xsum);
	memcpy(spfpkt->un.aubytes, if_passwd, 8);
	break;

      case AUT_CRYPT:	// Cryptographic authentication (e.g., MD5)
	md5_generate(pdesc);
	break;

      default:
	sys->halt(HALT_AUTYPE, "Bad authtype on transmit");
    }
}

/* Set the MD5 authentication fields in a packet that we
 * are going to transmit.
 * Bumps Pkt::dptr by the size of the digest, so the digest
 * will be included as part of the IP packet.
 */

void SpfIfc::md5_generate(Pkt *pdesc)

{
    SpfPkt *spfpkt;
    int	length;
    CryptK *key;
    KeyIterator key_iter(this);
    byte digest[16];
    MD5_CTX context;
    CryptK *best_key;
    byte *spfend;
    SPFtime now;

    spfpkt = pdesc->spfpkt;
    length = ntoh16(spfpkt->plen);
    spfend = ((byte *) spfpkt) + length;
    best_key = 0;

    // Locate correct key
    now = sys_etime;
    while ((key = key_iter.get_next())) {
	if (time_less(now, key->start_generate))
	    continue;
	if (key->stop_generate_specified &&
	    !time_less(now, key->stop_generate))
	    continue;
	if (!best_key ||
	    time_less(best_key->start_generate, key->start_generate))
	    best_key = key;
    }

    if (!best_key)
	return;
    // Set MD5 parameters
    spfpkt->un.crypt.crypt_mbz = 0;
    spfpkt->un.crypt.keyid = best_key->key_id;
    spfpkt->un.crypt.audlen = 16;
    spfpkt->un.crypt.seqno = hton32(now.sec);

    // Calculate digest with our secret key
    memcpy(spfend, best_key->key, 16);
    MD5Init(&context);
    MD5Update(&context, (byte *) spfpkt, length + 16);
    MD5Final(digest, &context);
    // Append digest to end of packet
    memcpy(spfend, digest, 16);

    // Make sure that we transmit digest
    pdesc->dptr += 16;
}

/* Authenticate the packet.
 * Coming into this routine, pdesc->end points to the
 * end of the encapsulating IP packet. After
 * authentication, end is adjusted to be the end of
 * the OSPF packet.
 */

int SpfIfc::verify(Pkt *pdesc, SpfNbr *np)

{
    SpfPkt *spfpkt;
    byte *spfend;

    spfpkt = pdesc->spfpkt;
    if (ntoh16(spfpkt->autype) != if_autype)
	return(false);
    spfend = ((byte *) spfpkt) + ntoh16(spfpkt->plen);

    switch (if_autype) {
      case AUT_NONE:	// No authentication
	if (incksum((uns16 *) spfpkt, ntoh16(spfpkt->plen)) != 0)
	    return(false);
	break;

      case AUT_PASSWD:	// Simple cleartext password
	if (memcmp(spfpkt->un.aubytes, if_passwd, 8) != 0)
	    return(false);
        memset(spfpkt->un.aubytes, 0, 8);
	if (incksum((uns16 *) spfpkt, ntoh16(spfpkt->plen)) != 0)
	    return(false);
	break;

      case AUT_CRYPT:	// Cryptographic authentication (e.g., MD5)
	if (!md5_verify(pdesc, np))
	    return(false);
	break;

      default:
	return(false);
    }

    // Packet authenticates
    // Strip IP trailer
    pdesc->end = spfend;
    return(true);
}

/* Verify the cryptographic authentication for a received
 * packet. First locate the key based on the Key ID. Then,
 * check for replays by verifying the sequence number.
 * Finally, run the cryptographic hash using our secret key,
 * and check that the generated digest matches the one appended
 * to the received packet.
 * Packet checksum is not used when we're doing cryptographic
 * authentication.
 */

int SpfIfc::md5_verify(Pkt *pdesc, SpfNbr *np)

{
    SpfPkt *spfpkt;
    byte *spfend;
    CryptK *key;
    KeyIterator key_iter(this);
    byte saved_digest[16];
    byte digest[16];
    MD5_CTX context;
    SPFtime now;

    spfpkt = pdesc->spfpkt;
    spfend = ((byte *) spfpkt) + ntoh16(spfpkt->plen);

    // Check that digest actually appended
    if ((spfend + 16) > pdesc->end)
	return(false);
    // Locate correct key
    now = sys_etime;
    while ((key = key_iter.get_next())) {
	if (time_less(now, key->start_accept))
	    continue;
	if (key->stop_accept_specified &&
	    !time_less(now, key->stop_accept))
	    continue;
	if (key->key_id == spfpkt->un.crypt.keyid)
	    break;
    }

    if (!key)
	return(false);
    // Verify sequence number of live nighbors
    if (np != 0 && np->state() > NBS_ATTEMPT)
	if (np->md5_seqno > ntoh32(spfpkt->un.crypt.seqno))
	    return(false);

    // Save received digest
    memcpy(saved_digest, spfend, 16);
    // Calculate digest with our secret key
    memcpy(spfend, key->key, 16);
    MD5Init(&context);
    MD5Update(&context, (byte *) spfpkt, ntoh16(spfpkt->plen) + 16);
    MD5Final(digest, &context);
    // Compare to saved copy
    if (memcmp(digest, saved_digest, 16) != 0)
	return(false);

    // Save sequence number
    if (np)
	np->md5_seqno = ntoh32(spfpkt->un.crypt.seqno);
    return(true);
}


/* Find the neighbor representing the source of the received
 * OSPF packet. Identified by IP source address. Overriden
 * by point-to-point interfaces and virtual links, where neighbor
 * is identified by Router ID in the received OSPF header.
 */

SpfNbr *SpfIfc::find_nbr(InAddr addr, rtid_t)

{
    NbrIterator iter(this);
    SpfNbr *np;

    while ((np = iter.get_next())) {
	if (np->addr() == addr)
	    break;
    }

    return(np);
}

/* On point-to-point and virtual links, the neighbor is
 * identified by its OSPF Router ID.
 */

SpfNbr *PPIfc::find_nbr(InAddr, rtid_t id)

{
    NbrIterator iter(this);
    SpfNbr *np;

    while ((np = iter.get_next())) {
	if (np->id() == id)
	    break;
    }

    return(np);
}

/* Upon receiving a Hello, may set the neighbor's Router ID
 * or IP address, or neither, depending on interface type.
 */

// Set router ID on multi-access interfaces
void SpfIfc::set_id_or_addr(SpfNbr *np, rtid_t _id, InAddr)
{
    np->n_id = _id;
}

// Set IP address on point-to-point interfaces
void PPIfc::set_id_or_addr(SpfNbr *np, rtid_t, InAddr _addr)
{
    np->n_addr = _addr;
}

/* Functions that multicast OSPF packets out the given interface
 * types.
 * Unicast packets are handled by SpfIfc::nbr_send().
 */

/* Generic multicast send function, used by broadcast and point-to-point
 * interfaces. Simply send the IP packet out the correct
 * physical interface, specifying the next hop or multicast
 * address.
 */

void SpfIfc::if_send(Pkt *pdesc, InAddr addr)

{
    InPkt *pkt;

    if (!pdesc->iphdr)
	return;
    finish_pkt(pdesc, addr);
    pkt = pdesc->iphdr;
    if (pkt->i_src != 0) {
	if (ospf->spflog(LOG_TXPKT, 1)) {
	    ospf->log(pdesc);
	    ospf->log(this);
	}
    sys->sendpkt(pkt, if_phyint);
    }
    else if (ospf->spflog(ERR_NOADDR, 5)) {
	    ospf->log(pdesc);
	    ospf->log(this);
	}

    ospf->ospf_freepkt(pdesc);
}

/* Send a multicast packet out a point-to-point link. Packet is
 * always sent to the AllSPFRouters address.
 */

void PPIfc::if_send(Pkt *pdesc, InAddr)

{
    if (!pdesc->iphdr)
	return;
    SpfIfc::if_send(pdesc, AllSPFRouters);
}

/* Send a multicast packet out a virtual link. Packet is sent directly
 * to the IP address of the other end of the link.
 */

void VLIfc::if_send(Pkt *pdesc, InAddr)

{
    InPkt *pkt;

    if (!pdesc->iphdr)
	return;
    
    finish_pkt(pdesc, if_rmtaddr);
    pkt = pdesc->iphdr;
    if (ospf->spflog(LOG_TXPKT, 1)) {
	ospf->log(pdesc);
	ospf->log(this);
    }
    sys->sendpkt(pkt);
    ospf->ospf_freepkt(pdesc);
}

/* Send the multicast packet out an NBMA or ptmp interface. Packet is sent
 * separately to each neighbor. This function is only called
 * by flooding (Hellos have a separate function), and so packets
 * are only sent to those neighbors in states Exchange or greater.
 */

void SpfIfc::nonbroadcast_send(Pkt *pdesc, InAddr addr)

{
    InPkt *pkt;
    SpfNbr *np;
    NbrIterator nbrIter(this);

    if (!pdesc->iphdr)
	return;
    finish_pkt(pdesc, addr);
    pkt = pdesc->iphdr;
    while ((np = nbrIter.get_next())) {
	if (np->state() < NBS_EXCH)
	    continue;
	if (ospf->spflog(LOG_TXPKT, 1)) {
	    ospf->log(pdesc);
	    ospf->log(np);
	}
	sys->sendpkt(pkt, if_phyint, np->addr());
    }
    ospf->ospf_freepkt(pdesc);
}

/* Hello interval has changed. Restart the hello timer on each
 * of a non-broadcast networks nighbors. Using restart, we don't
 * have to worry about whether we should be sending them a hello
 * or not.
 */

void SpfIfc::nonbroadcast_restart_hellos()

{
    NbrIterator iter(this);
    SpfNbr *np;

    while ((np = iter.get_next()))
	np->nbr_fsm(NBE_EVAL);
}

/* Interface has gone down. Stop sending Hellos to all
 * neighbors.
 */

void SpfIfc::nonbroadcast_stop_hellos()

{
    NbrIterator iter(this);
    SpfNbr *np;

    while ((np = iter.get_next()))
	np->n_htim.stop();
}

/* If OSPF::PPAdjLimit is non-zero, we limit the number
 * of point-to-point links which will become adjacent
 * to a particular neighbor. If the "enlist" parameter
 * is true, and there are insufficient adjacencies, we
 * add all the 2-Way point-to-point interfaces to the
 * pending adjacency list, since we don't know which
 * ones we will be able to advance.
 */

bool SpfIfc::more_adjacencies_needed(rtid_t)

{
    return(true);
}

bool VLIfc::more_adjacencies_needed(rtid_t)

{
    return(true);
}

bool PPIfc::more_adjacencies_needed(rtid_t nbr_id)

{
    PPAdjAggr *adjaggr;

    if (ospf->PPAdjLimit == 0)
        return(true);
    if (!(adjaggr = (PPAdjAggr *)if_area->AdjAggr.find(nbr_id, 0)))
        return(true);
    return ((ospf->my_id() >nbr_id) && (adjaggr->n_adjs< ospf->PPAdjLimit));
}
