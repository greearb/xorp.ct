// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "vrrp/vrrp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "vrrp/vrrp_exception.hh"
#include "vrrp/vrrp_interface.hh"
#include "vrrp/vrrp.hh"

namespace {

#define TEST_ASSERT(x)					    \
    if (!(x))						    \
	xorp_throw(VrrpException, "Test assertion failed")

const string  INITIALIZE = "initialize";
const string  MASTER     = "master";
const string  BACKUP	 = "backup";

EventLoop _eventloop;

// XXX split this into multiple files

struct Packet {
    Packet(const Mac& s, const Mac& d, uint32_t e, const PAYLOAD& p)
		: src(s), dst(d), ether(e), payload(p)
    {
    }

    Mac	     src;
    Mac	     dst;
    uint32_t ether;
    PAYLOAD  payload;
};

typedef list<Packet*>   PACKETS;
typedef set<IPv4>	IPS;

class VrrpVifTest : public VrrpInterface {
public:
    ~VrrpVifTest();

    bool	own(const IPv4& addr);
    bool	ready() const;
    const IPv4& addr() const;
    void        send(const Mac& src, const Mac& dst, uint32_t ether,
                     const PAYLOAD& payload);
    void        join_mcast();
    void        leave_mcast();
    void        add_mac(const Mac& mac);
    void        delete_mac(const Mac& mac);
    void        start_arps();
    void        stop_arps();
    void	add_ip(const IPv4& ip);
    PACKETS&	packets();

private:
    IPS	    _ips;
    IPv4    _addr;
    PACKETS _packets;
};

class VrrpInstance {
public:
    VrrpInstance(uint32_t vrid);

    void   from_initialize_to_master();
    void   from_initialize_to_backup();
    void   from_backup_to_initialize();
    void   from_backup_to_master();
    void   from_master_to_initialize();
    void   add_ip(const IPv4& ip);
    void   check_gratitious_arp(const IPv4& ip);
    bool   check_gratitious_arp(Packet& p, const IPv4& ip);
    void   check_advertisement();
    bool   check_advertisement(Packet& p);
    void   check_master_sent_packets();
    void   add_owned_ip(const IPv4& ip);
    string state();

private:
    uint32_t	_vrid;
    VrrpVifTest	_vif;
    Vrrp	_vrrp;
    IPS		_ips;
    Mac		_source_mac;
    uint32_t	_interval;
    uint32_t	_priority;
};

/////////////////////////
//
// VrrpVifTest
//
/////////////////////////

VrrpVifTest::~VrrpVifTest()
{
    for (PACKETS::iterator i = _packets.begin(); i != _packets.end(); ++i)
	delete *i;
}

bool
VrrpVifTest::own(const IPv4& addr)
{
    return _ips.find(addr) != _ips.end();
}

bool
VrrpVifTest::ready() const
{
    return true;
}

const IPv4&
VrrpVifTest::addr() const
{
    return _addr;
}

void
VrrpVifTest::send(const Mac& src, const Mac& dst, uint32_t ether,
		  const PAYLOAD& payload)
{
    _packets.push_back(new Packet(src, dst, ether, payload));
}

void
VrrpVifTest::join_mcast()
{
}

void
VrrpVifTest::leave_mcast()
{
}

void
VrrpVifTest::add_mac(const Mac& mac)
{
    UNUSED(mac);
}

void
VrrpVifTest::delete_mac(const Mac& mac)
{
    UNUSED(mac);
}

void
VrrpVifTest::start_arps()
{
}

void
VrrpVifTest::stop_arps()
{
}

void
VrrpVifTest::add_ip(const IPv4& ip)
{
    _ips.insert(ip);
}

PACKETS&
VrrpVifTest::packets()
{
    return _packets;
}

/////////////////////////
//
// VrrpInstance
//
/////////////////////////

VrrpInstance::VrrpInstance(uint32_t vrid)
		    : _vrid(vrid),
		      _vrrp(_vif, _eventloop, _vrid),
		      _interval(1),
		      _priority(100)
{
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "00:00:5E:00:01:%X", (uint8_t) vrid);
    _source_mac = Mac(tmp);
}

void
VrrpInstance::add_ip(const IPv4& ip)
{
    _ips.insert(ip);
    _vrrp.add_ip(ip);
}

string
VrrpInstance::state()
{
    IPv4 ip;
    string state;

    _vrrp.get_info(state, ip);

    return state;
}

bool
VrrpInstance::check_gratitious_arp(Packet& p, const IPv4& ip)
{
    if (p.ether != ETHERTYPE_ARP)
	return false;

    TEST_ASSERT(p.src == _source_mac);
    TEST_ASSERT(p.dst == Mac::BROADCAST());

    const ArpHeader& ah = ArpHeader::assign(p.payload);
   
    TEST_ASSERT(ah.is_request());
    IPv4 tmp_ip = ah.get_request();
    TEST_ASSERT(tmp_ip == ip);

    // check source fields
    Mac tmp_mac;
    tmp_mac.copy_in(ah.ah_data);
    TEST_ASSERT(tmp_mac == _source_mac);

    tmp_ip.copy_in(&ah.ah_data[ah.ah_hw_len]);
    TEST_ASSERT(tmp_ip == ip);

    return true;
}

void
VrrpInstance::check_gratitious_arp(const IPv4& ip)
{
    PACKETS& packets = _vif.packets();

    for (PACKETS::iterator i = packets.begin(); i != packets.end(); ++i) {
	Packet* p = *i;
	if (check_gratitious_arp(*p, ip)) {
	    delete p;
	    packets.erase(i);
	    return;
	}
    }

    TEST_ASSERT(false);
}

bool
VrrpInstance::check_advertisement(Packet& p)
{
    if (p.ether != ETHERTYPE_IP)
	return false;

    // check MAC addresses
    TEST_ASSERT(p.src == _source_mac);
    TEST_ASSERT(p.dst == Mac("01:00:5E:00:00:12"));

    // check IPs
    IPv4 tmp_ip;
    tmp_ip.copy_in(&p.payload[12]);
    TEST_ASSERT(tmp_ip == _vif.addr());

    tmp_ip.copy_in(&p.payload[16]);
    TEST_ASSERT(tmp_ip == IPv4("224.0.0.18"));

    // check VRRP
    PAYLOAD vrrp(p.payload.begin() + 20, p.payload.end());
    const VrrpHeader& vh = VrrpHeader::assign(vrrp);

    TEST_ASSERT(vh.vh_v	       == 2);
    TEST_ASSERT(vh.vh_type     == 1);
    TEST_ASSERT(vh.vh_auth     == 0);
    TEST_ASSERT(vh.vh_vrid     == _vrid);
    TEST_ASSERT(vh.vh_priority == _priority);
    TEST_ASSERT(vh.vh_interval == _interval);
    TEST_ASSERT(vh.vh_ipcount  == _ips.size());

    for (unsigned i = 0; i < vh.vh_ipcount; ++i) {
	tmp_ip.copy_in((const uint8_t*) &vh.vh_addr[i]);
	TEST_ASSERT(_ips.find(tmp_ip) != _ips.end());
    }

    return true;
}

void
VrrpInstance::check_advertisement()
{
    PACKETS& packets = _vif.packets();

    for (PACKETS::iterator i = packets.begin(); i != packets.end(); ++i) {
	Packet* p = *i;
	if (check_advertisement(*p)) {
	    delete p;
	    packets.erase(i);
	    return;
	}
    }

    TEST_ASSERT(false);
}

void
VrrpInstance::check_master_sent_packets()
{
    for (IPS::iterator i = _ips.begin(); i != _ips.end(); ++i)
	check_gratitious_arp(*i);

    check_advertisement();

    TEST_ASSERT(_vif.packets().empty());
}

void
VrrpInstance::from_initialize_to_master()
{
    //
    // initialize -> master
    //
    // -  If the Priority = 255 (i.e., the router owns the IP address(es)
    //    associated with the virtual router)
    //
    //    o  Send an ADVERTISEMENT
    //    o  Broadcast a gratuitous ARP request containing the virtual
    //       router MAC address for each IP address associated with the
    //       virtual router.
    //    o  Set the Adver_Timer to Advertisement_Interval
    //    o  Transition to the {Master} state
    //

    // check initial state
    TEST_ASSERT(state()	         == INITIALIZE);
    TEST_ASSERT(_vrrp.priority() == 255);
    _priority = 255;

    // start
    _vrrp.start();

    // check sent packets
    check_master_sent_packets();

    // check final state
    TEST_ASSERT(state() == MASTER);
}

void
VrrpInstance::from_master_to_initialize()
{
    //
    // -  If a Shutdown event is received, then:
    //
    //    o  Cancel the Adver_Timer
    //    o  Send an ADVERTISEMENT with Priority = 0
    //    o  Transition to the {Initialize} state
    //

    TEST_ASSERT(state() == MASTER);

    _vrrp.stop();

    _priority = 0;
    check_advertisement();

    TEST_ASSERT(_vif.packets().empty());

    TEST_ASSERT(state() == INITIALIZE);
}

void
VrrpInstance::add_owned_ip(const IPv4& ip)
{
    _vif.add_ip(ip);
    add_ip(ip);
}

void
VrrpInstance::from_initialize_to_backup()
{
    //
    // initialize -> backup
    //
    //    o  Set the Master_Down_Timer to Master_Down_Interval
    //    o  Transition to the {Backup} state
    //
    //

    // check initial state
    TEST_ASSERT(state()	         == INITIALIZE);
    TEST_ASSERT(_vrrp.priority() == _priority);

    // start
    _vrrp.start();

    // check sent packets
    TEST_ASSERT(_vif.packets().empty());

    // check final state
    TEST_ASSERT(state() == BACKUP);
}

void
VrrpInstance::from_backup_to_initialize()
{
    //
    // -  If a Shutdown event is received, then:
    //
    //    o  Cancel the Master_Down_Timer
    //    o  Transition to the {Initialize} state
    //

    // check initial state
    TEST_ASSERT(state() == BACKUP);

    // stop
    _vrrp.stop();

    // check sent packets
    TEST_ASSERT(_vif.packets().empty());

    // check final state
    TEST_ASSERT(state() == INITIALIZE);
}

void
VrrpInstance::from_backup_to_master()
{
    //
    // -  If the Master_Down_Timer fires, then:
    //
    //    o Send an ADVERTISEMENT
    //    o Broadcast a gratuitous ARP request containing the virtual
    //      router MAC address for each IP address associated with the
    //      virtual router
    //    o Set the Adver_Timer to Advertisement_Interval
    //    o Transition to the {Master} state
    //
    TEST_ASSERT(state() == BACKUP);

    // XXX wait for Master_Down_Interval
    cout << "Waiting for Master_Down_Interval" << endl;
    while (state() != MASTER)
	_eventloop.run();

    TEST_ASSERT(state() == MASTER);

    check_master_sent_packets();
}

/////////////////////////
//
// Tests
//
/////////////////////////

void
from_initialize_to_master()
{
    VrrpInstance vi(1);

    vi.add_owned_ip(IPv4("1.2.3.4"));
    vi.from_initialize_to_master();
}

void
from_master_to_initialize()
{
    VrrpInstance vi(1);

    vi.add_owned_ip(IPv4("1.2.3.4"));
    vi.from_initialize_to_master();
    vi.from_master_to_initialize();
}

void
from_initialize_to_backup()
{
    VrrpInstance vi(1);

    vi.add_ip(IPv4("1.2.3.4"));
    vi.from_initialize_to_backup();
}

void
from_backup_to_initialize()
{
    VrrpInstance vi(1);

    vi.add_ip(IPv4("1.2.3.4"));
    vi.from_initialize_to_backup();
    vi.from_backup_to_initialize();
}

void
from_backup_to_master()
{
    VrrpInstance vi(1);

    vi.add_ip(IPv4("1.2.3.4"));
    vi.from_initialize_to_backup();
    vi.from_backup_to_master();
}

void
test_state_transitions()
{
    // master
    from_initialize_to_master();
    from_master_to_initialize();
    // XXX test receiving advertisements

    // backup
    from_initialize_to_backup();
    from_backup_to_initialize();
    from_backup_to_master();
    // XXX test receiving advertisements
}

void
go()
{
    test_state_transitions();

    // XXX test sending advertisements

    // XXX test instances working in harmony

    // XXX test corner cases
}

} // anon namespace

int
main(int argc, char* argv[])
{
    UNUSED(argc);

    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	go();
    } catch(const VrrpException& e) {
	cout << "VrrpException: " << e.str() << endl;
	exit(1);
    }

    xlog_stop();
    xlog_exit();

    exit(0);
}
