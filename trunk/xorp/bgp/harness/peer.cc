// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/bgp/harness/peer.cc,v 1.55 2004/06/10 22:40:40 hodson Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include <string>
#include <stdlib.h>

#include "config.h"
#include "bgp/bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4net.hh"

#include "bgp/local_data.hh"
#include "bgp/packet.hh"

#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/test_peer_xif.hh"
#include "peer.hh"
#include "bgppp.hh"

Peer::~Peer()
{
    debug_msg("XXX Deleting peer %p\n", this);
}

Peer::Peer(EventLoop    *eventloop,
	   XrlStdRouter *xrlrouter,
	   const string& peername,
	   const uint32_t genid,
	   const string& target_hostname,
	   const string& target_port)
    : _eventloop(eventloop),
      _xrlrouter(xrlrouter),
      _peername(peername),
      _genid(genid),
      _target_hostname(target_hostname),
      _target_port(target_port),
      _up(true),
      _busy(0),
      _connected(false),
      _session(false),
      _passive(false),
      _established(false),
      _keepalive(false),
      _as(AsNum::AS_INVALID), // XXX
      _holdtime(0),
      _ipv6(false)
{
    debug_msg("XXX Creating peer %p\n", this);
}

Peer::Peer(const Peer& rhs)
    : _as(AsNum::AS_INVALID) // XXX
{
    debug_msg("Copy constructor\n");

    copy(rhs);
}

Peer::Peer
Peer::operator=(const Peer& rhs)
{
    debug_msg("= operator\n");
    if(&rhs == this)
	return *this;

    copy(rhs);
    
    return *this;
}

void
Peer::copy(const Peer& rhs)
{
    debug_msg("peername: %s\n", rhs._peername.c_str());

    _eventloop = rhs._eventloop;
    _xrlrouter = rhs._xrlrouter;

    _peername = rhs._peername;
    _genid = rhs._genid;
    _target_hostname = rhs._target_hostname;
    _target_port = rhs._target_port;

    _up = rhs._up;

    _busy = rhs._busy;
    _session = rhs._session;
    _passive = rhs._passive;
    _keepalive = rhs._keepalive;
    _established = rhs._established;

    _as = rhs._as;
    _holdtime = rhs._holdtime;
    _ipv6 = rhs._ipv6;
    _id = rhs._id;
}

Peer::PeerState
Peer::is_this_you(const string& peer_name) const
{
    debug_msg("is this you: %s %s\n", _peername.c_str(), peer_name.c_str());
    
    if(_up && peer_name == _peername) {
	debug_msg("Its me\n");
	return Peer::YES_ITS_ME;
    }


    /*
    ** If this peer has been down for ten minutes its a candidate to
    ** be deleted. There should be no more activity on this peer.
    */
    if(!_up) {
	TimeVal now;
	_eventloop->current_time(now);
	if(now - _shutdown_time > TimeVal(60 * 10, 0))
	    return Peer::PLEASE_DELETE_ME;
    }

    return Peer::NO_ITS_NOT_ME;
}

Peer::PeerState
Peer::is_this_you(const string& peer_name, const uint32_t genid) const
{
    debug_msg("is this you: %s %s %u\n", _peername.c_str(), peer_name.c_str(),
	      genid);
    
    if(_up && peer_name == _peername && genid == _genid)
	return Peer::YES_ITS_ME;

    if(!_up)
	return is_this_you("");

    return Peer::NO_ITS_NOT_ME;
}

void
Peer::shutdown()
{
    XLOG_ASSERT(_up);	// We should only ever be shutdown once.

    _up = false;
    _eventloop->current_time(_shutdown_time);
    
    /*
    ** The corresponding test peer may have a tcp connection with a
    ** bgp process. Attempt a disconnect.
    */
    _busy++;
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    if(!test_peer.send_reset(_peername.c_str(),
			     callback(this, &Peer::xrl_callback, "reset")))
	XLOG_FATAL("reset failed");
}

bool
Peer::up() const
{
    return _up;
}

void
Peer::status(string& status)
{
    status = _peername + " ";
    status += _established ? "established" : "";
    status += " ";
    status += "sent: " + c_format("%d", _trie_sent.update_count()) + " ";
    status += "received: " + c_format("%d", _trie_recv.update_count()) + " ";
}

bool
Peer::pending()
{
    debug_msg("pending? : %s\n", (_session && !_established && !_passive)
		  ? "true" : "false");

//     if(_xrlrouter->pending())
// 	return true;

    if(_busy > 0)
	return true;

//     printf("%s: session: %d established: %d passive %d\n", 
// 	   _peername.c_str(), _session, _established, _passive);
    if(_session && !_established && !_passive)
	return true;

    return false;
}

void 
Peer::listen(const string& /*line*/, const vector<string>& /*words*/)
	throw(InvalidString)
{
    /* Connect the test peer to the target BGP */
    debug_msg("About to listen on: %s\n", _peername.c_str());
    _busy += 3;
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    if(!test_peer.send_register(_peername.c_str(), _xrlrouter->name(), _genid,
			callback(this, &Peer::xrl_callback, "register")))
	XLOG_FATAL("send_register failed");
    if(!test_peer.send_packetisation(_peername.c_str(), "bgp",
			   callback(this, &Peer::xrl_callback,
				    "packetisation")))
	XLOG_FATAL("send_packetisation");
    if(!test_peer.send_listen(_peername.c_str(),
			   _target_hostname, atoi(_target_port.c_str()),
			      callback(this, &Peer::xrl_callback, "listen")))
	XLOG_FATAL("send_listen failed");
}

void 
Peer::connect(const string& /*line*/, const vector<string>& /*words*/)
	throw(InvalidString)
{
    /* Connect the test peer to the target BGP */
    debug_msg("About to connect to: %s\n", _peername.c_str());
    _busy += 3;
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    if(!test_peer.send_register(_peername.c_str(), _xrlrouter->name(), _genid,
			callback(this, &Peer::xrl_callback, "register")))
	XLOG_FATAL("send_register failed");
    if(!test_peer.send_packetisation(_peername.c_str(), "bgp",
		     callback(this, &Peer::xrl_callback, "packetisation")))
	XLOG_FATAL("send_packetisation failed");
    if(!test_peer.send_connect(_peername.c_str(),
			   _target_hostname, atoi(_target_port.c_str()),
			       callback(this, &Peer::xrl_callback_connected,
					"connected")))
	XLOG_FATAL("send_connect failed");
}

void 
Peer::disconnect(const string& /*line*/, const vector<string>& /*words*/)
	throw(InvalidString)
{
    /* Connect the test peer to the target BGP */
    debug_msg("About to disconnect from: %s\n", _peername.c_str());
    _busy++;
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    _session = false;
    _established = false;
    if(!test_peer.send_disconnect(_peername.c_str(),
			  callback(this, &Peer::xrl_callback, "disconnect")))
	XLOG_FATAL("send_disconnect failed");
}

/*
** Form of command:
** peer1 establish active <true/false> AS <asnum>
**
*/
void
Peer::establish(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    debug_msg("About to establish a connection %s\n", _peername.c_str());

    bool active = true;

    _session = true;
    /*
    ** Parse the command line.
    ** The arguments for this command come in pairs:
    ** name value
    */
    uint16_t as = 0;
    size_t size = words.size();
    if(0 != (size % 2))
	xorp_throw(InvalidString,
		   c_format("Incorrect number of arguments:\n[%s]",
			    line.c_str()));

    for(size_t i = 2; i < words.size(); i += 2) {
	debug_msg("name: %s value: %s\n",
		  words[i].c_str(),
		  words[i + 1].c_str());
	if("active" == words[i]) {
	    string aarg = words[i+1];
	    if("true" == aarg)
		active = true;
	    else if("false" == aarg)
		active = false;
	    else
		xorp_throw(InvalidString, 
			   c_format("Illegal argument to active: <%s>\n[%s]",
				    aarg.c_str(), line.c_str()));
	} else if("AS" == words[i]) {
	    as = atoi(words[i + 1].c_str());
	} else if("keepalive" == words[i]) {
	    string kaarg = words[i+1];
	    if("true" == kaarg)
		_keepalive = true;
	    else if("false" == kaarg)
		_keepalive = false;
	    else
		xorp_throw(InvalidString, 
			   c_format("Illegal argument to keepalive: <%s>\n[%s]",
				    kaarg.c_str(), line.c_str()));
	} else if("holdtime" == words[i]) {
	    _holdtime = atoi(words[i + 1].c_str());
	} else if("id" == words[i]) {
	    _id = IPv4(words[i + 1].c_str());
	} else if("ipv6" == words[i]) {
	    string ipv6arg = words[i+1];
	    if("true" == ipv6arg)
		_ipv6 = true;
	    else if("false" == ipv6arg)
		_ipv6 = false;
	    else
		xorp_throw(InvalidString, 
			   c_format("Illegal argument to ipv6: <%s>\n[%s]",
				    ipv6arg.c_str(), line.c_str()));
	} else
	    xorp_throw(InvalidString, 
		       c_format("Illegal argument to establish: <%s>\n[%s]",
				words[i].c_str(), line.c_str()));
    }

    if(0 == as)
	xorp_throw(InvalidString,
		   c_format("No AS number specified:\n[%s]", line.c_str()));

    _as = AsNum(static_cast<uint16_t>(as));

    if(!active) {
	_passive = true;
	listen(line, words);
	return;
    }

    connect(line, words);

    send_open();
}

void
Peer::send(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    size_t size = words.size();
    if(size < 3)
	xorp_throw(InvalidString,
		   c_format("Insufficient number of arguments:\n[%s]",
			    line.c_str()));

    const char PACKET[] = "packet";
    const char DUMP[] = "dump";
    if(PACKET == words[2]) {
	send_packet(line, words);
    } else if(DUMP == words[2]) {
	send_dump(line, words);
    } else {
	xorp_throw(InvalidString,
		   c_format(
		   "Second argument should be %s or %s not <%s>\n[%s]",
			    PACKET, DUMP, words[2].c_str(), line.c_str()));
    }
}

/*
** Form of command:
** peer1 send packet update ...
** peer1 send packet notify ...
** peer1 send packet open ...
** peer1 send packet keepalive
**
** See the packet method for the rest of the commands
*/
void
Peer::send_packet(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    const BGPPacket *pkt = Peer::packet(line, words, 3);

    size_t len;
    const uint8_t *buf = pkt->encode(len);
    delete pkt;

    /*
    ** If this is an update message, save it in the sent trie.
    */
    const char update[] = "update";
    if(update == words[3]) {
	TimeVal tv;
	_eventloop->current_time(tv);
	_trie_sent.process_update_packet(tv, buf, len);
    }

    _busy++;
    send_message(buf, len, callback(this, &Peer::xrl_callback, "send packet"));
}

struct mrt_header {
    uint32_t time;
    uint16_t type;
    uint16_t subtype;
    uint32_t length;
};

struct mrt_update {
    uint16_t source_as;
    uint16_t dest_as;
    uint16_t ifindex;
    uint16_t af;
    uint32_t source_ip;
    uint32_t dest_ip;
};

inline
const
uint8_t *
mrtd_traffic_file_read(FILE *fp, size_t& len)
{
    mrt_header header;

    if(fread(&header, sizeof(header), 1, fp) != 1) {
	if(feof(fp))
	    return 0;
	XLOG_WARNING("fread failed:%s", strerror(errno));
	return 0;
    }

    len = ntohl(header.length) - sizeof(mrt_update);

    mrt_update update;
    if(fread(&update, sizeof(update), 1, fp) != 1) {
	if(feof(fp))
	    return 0;
	XLOG_WARNING("fread failed:%s", strerror(errno));
	return 0;
    }

    uint8_t *buf = new uint8_t[len];

    if(fread(buf, len, 1, fp) != 1) {
	if(feof(fp))
	    return 0;
	XLOG_WARNING("fread failed:%s", strerror(errno));
	return 0;
    }

    return buf;
}

/*
** peer send dump mrtd update fname <count>
** 0    1    2    3    4      5	    6
*/
void
Peer::send_dump(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    if(6 != words.size() && 7 != words.size())
	xorp_throw(InvalidString,
		   c_format("Incorrect number of arguments:\n[%s]",
			    line.c_str()));

    const char MRTD[] = "mrtd";
    if(MRTD != words[3])
	xorp_throw(InvalidString,
		   c_format("Third argument should be %s not <%s>\n[%s]",
			    MRTD, words[3].c_str(), line.c_str()));

    const char UPDATE[] = "update";
    if(UPDATE != words[4])
	xorp_throw(InvalidString,
		   c_format("Fourth argument should be %s not <%s>\n[%s]",
			    UPDATE, words[4].c_str(), line.c_str()));

    string fname = words[5];
    FILE *fp = fopen(fname.c_str(), "r");
    if(0 == fp)
	xorp_throw(InvalidString,
		   c_format("fopen of %s failed: %s\n[%s]",
			    fname.c_str(), strerror(errno), line.c_str()));

    size_t packets_to_send = 0;
    if(7 == words.size()) 
	packets_to_send = atoi(words[6].c_str());
	// XXX - We don't check for this to be an integer, we assume
	// failure returns zero. In which case the whole file is sent.

    /*
    ** It could take quite a while to send a large file so we need to
    ** set up the transfer and then return.
    */
    send_dump_callback(XrlError::OKAY(), fp, 0, packets_to_send,
		       "mrtd_traffic_send");
}

void
Peer::send_dump_callback(const XrlError& error, FILE *fp,
			 const size_t packet_number,
			 const size_t packets_to_send,
			 const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
	fclose(fp);
	return;
    }

    if(packets_to_send != 0 && packet_number == packets_to_send) {
	fclose(fp);
	return;
    }

    size_t len;
    const uint8_t *buf;

    while(0 != (buf = mrtd_traffic_file_read(fp, len))) {
	const fixed_header *header = 
	    reinterpret_cast<const struct fixed_header *>(buf);
	if(MESSAGETYPEUPDATE == header->type) {
	    /*
	    ** Save the update message in the sent trie.
	    */
	    TimeVal tv;
	    _eventloop->current_time(tv);
	    _trie_sent.process_update_packet(tv, buf, len);

	    _smcb = callback(this, &Peer::send_dump_callback,
			     fp, 
			     packet_number + 1, packets_to_send,
			     "mrtd_traffic_send");
	    send_message(buf, len, _smcb);
	    return;
	} else {
	    delete [] buf;
	}
    }
    fclose(fp);
}

void
Peer::trie(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    if(words.size() < 4)
	xorp_throw(InvalidString,
		   c_format("Insufficient arguments:\n[%s]", line.c_str()));

    /*
    ** Each peer holds two tries. One holds updates sent the other
    ** holds updates received. Determine which trie we are about to
    ** operate on.
    */
    Trie *op;
    if("sent" == words[2]) {
	op = &_trie_sent;
    } else if("recv" == words[2]) {
	op = &_trie_recv;
    } else
	xorp_throw(InvalidString,
		   c_format("\"sent\" or \"recv\" accepted not <%s>\n[%s]",
			    words[2].c_str(), line.c_str()));

    /*
    ** The only operation currently supported is lookup
    */
    if("lookup" != words[3])
	xorp_throw(InvalidString,
		   c_format("\"lookup\" expected not <%s>\n[%s]",
			    words[3].c_str(), line.c_str()));

    if(words.size() < 5)
	xorp_throw(InvalidString,
		   c_format("No arguments for \"lookup\"\n[%s]",
			    line.c_str()));
	
    const UpdatePacket *bgpupdate = op->lookup(words[4]);
    if(0 == bgpupdate) {
	if((words.size() == 6) && ("not" == words[5]))
	    return;
	xorp_throw(InvalidString,
		   c_format("Lookup failed [%s]", line.c_str()));
    }

    if((words.size() == 6) && ("not" == words[5]))
	xorp_throw(InvalidString,
		   c_format("Lookup failed entry exists [%s]", line.c_str()));

    debug_msg("Found: %s\n", bgpupdate->str().c_str());
    
    size_t size = words.size();
    if(0 == (size % 2))
	xorp_throw(InvalidString,
		   c_format("Incorrect number of arguments:\n[%s]",
			    line.c_str()));


    for(size_t i = 5; i < size; i += 2) {
	debug_msg("attribute: %s value: %s\n",
		  words[i].c_str(),
		  words[i + 1].c_str());
	if("aspath" == words[i]) {
	    /*
	    ** Search for the AS Path in the update packet.
	    */
	    const list<PathAttribute*>& palist = bgpupdate->pa_list();

	    list<PathAttribute*>::const_iterator pai;
	    const AsPath *aspath = 0;
	    for(pai = palist.begin(); pai != palist.end(); pai++) {
		if(AS_PATH == (*pai)->type())
		    aspath = &((static_cast<ASPathAttribute *>(*pai))->
			       as_path());
	    }
	    if(0 == aspath)
		xorp_throw(InvalidString, 
			   c_format("NO AS Path associated with route\n[%s]",
				    line.c_str())); 
		
	    if(*aspath != AsPath(words[i + 1].c_str()))
		xorp_throw(InvalidString, 
			   c_format("Looking for Path: <%s> Found: <%s>\n[%s]",
				    words[i + 1].c_str(),
				    aspath->str().c_str(),
				    line.c_str())); 
	} else
	    xorp_throw(InvalidString, 
		       c_format("Illegal attribute: <%s>\n[%s]",
				words[i].c_str(), line.c_str()));
    }
}

/*
** peer expect packet ...
** 0    1      2
*/
void
Peer::expect(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    if(words.size() < 3)
	xorp_throw(InvalidString,
		   c_format("Insufficient arguments:\n[%s]", line.c_str()));

    if("packet" == words[2]) {
	_expect._list.push_back(packet(line, words, 3));
    } else
	xorp_throw(InvalidString,
		   c_format(
			    "\"packet\" accepted not <%s>\n[%s]",
			    words[2].c_str(), line.c_str()));
}

/*
** peer assert queue
** 0    1      2
**
** peer assert queue <len>
** 0    1      2     3
**
** peer assert connected
** 0    1      2
**
** peer assert established
** 0    1      2
**
** peer assert idle
** 0    1      2
*/
void
Peer::assertX(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    if(words.size() < 3)
	xorp_throw(InvalidString,
		   c_format("Insufficient arguments:\n[%s]", line.c_str()));

    if("queue" == words[2]) {
	if(!_expect._ok)
	    xorp_throw(InvalidString,
	       c_format("Expect queue violated\nExpect: %sReceived: %s",
			_expect._list.front()->str().c_str(),
			_expect._bad->str().c_str()));
	switch(words.size()) {
	case 3:
	    break;
	case 4:
	    if(static_cast<unsigned int>(atoi(words[3].c_str())) !=
	       _expect._list.size())
		xorp_throw(InvalidString, 
			   c_format("Expected list size to be %d actual %u",
				    atoi(words[3].c_str()),
				    (uint32_t)_expect._list.size()));
	    break;
	default:
	    xorp_throw(InvalidString, 
		   c_format("Illegal number of arguments to \"queue\"\n[%s]",
			    line.c_str()));
	    break;
	}
    } else if("connected" == words[2]) {
	if(!_connected)
	    xorp_throw(InvalidString,
		       c_format("No TCP session established"));
    } else if("established" == words[2]) {
	if(!_established)
	    xorp_throw(InvalidString,
		       c_format("No BGP session established"));
    } else if("idle" == words[2]) {
	if(_connected)
	    xorp_throw(InvalidString,
		       c_format("Session established"));
    } else
	xorp_throw(InvalidString,
		   c_format(
		   "\"queue\" \"established\" \"idle\"accepted not <%s>\n[%s]",
		   words[2].c_str(), line.c_str()));
}

void
mrtd_traffic_dump(const uint8_t *buf, const size_t len , const TimeVal tv,
		  const string fname)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    mrt_header header;
    header.time = htonl(tv.sec());
    header.type = htons(16);
    header.subtype = htons(1);
    header.length = htonl(len + sizeof(mrt_update));
    
    if(fwrite(&header, sizeof(header), 1, fp) != 1)
	XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(), strerror(errno));

    mrt_update update;
    memset(&update, 0, sizeof(update));
    update.af = htons(1);	/* IPv4 */

    if(fwrite(&update, sizeof(update), 1, fp) != 1)
	XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(), strerror(errno));

    if(fwrite(buf, len, 1, fp) != 1)
	XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(), strerror(errno));

    fclose(fp);
}

template <class A>
void
mrtd_routview_dump(const  UpdatePacket* p, const IPNet<A>& net,
		   const TimeVal tv, const string fname, const int sequence)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    /*
    ** Figure out the total length of the attributes.
    */
    uint16_t length = 0;
    
    list <PathAttribute*>::const_iterator pai;
    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;

	length += pa->wire_size();
    }

    /*
    ** Due to alignment problems I can't use a structure overlay.
    */
    uint8_t viewbuf[18 + A::addr_size()], *ptr;

    mrt_header header;
    header.time = /*htonl(tv.sec())*/0;
    header.type = htons(12);
    if(4 == A::ip_version())
	header.subtype = htons(AFI_IPV4);
    else if(6 == A::ip_version())
	header.subtype = htons(AFI_IPV6);
    else
	XLOG_FATAL("unknown ip version %d", A::ip_version());

    header.length = htonl(length + sizeof(viewbuf));
    
    if(fwrite(&header, sizeof(header), 1, fp) != 1)
	XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(), strerror(errno));

    memset(&viewbuf[0], 0, sizeof(viewbuf));
    ptr = &viewbuf[0];

    // View number
    *reinterpret_cast<uint16_t *>(ptr) = 0;
    ptr += 2;

    // Sequence number
    *reinterpret_cast<uint16_t *>(ptr) = htons(sequence);
    ptr += 2;

    // Prefix
    net.masked_addr().copy_out(ptr);
    ptr += A::addr_size();

    // Prefix length
    *reinterpret_cast<uint8_t *>(ptr) = net.prefix_len();
    ptr += 1;

    // Status
    *reinterpret_cast<uint8_t *>(ptr) = 0x1;
    ptr += 1;

    // Uptime
    *reinterpret_cast<uint32_t *>(ptr) = htonl(tv.sec());
    ptr += 4;

    // Peer address
    *reinterpret_cast<uint32_t *>(ptr) = 0;
    ptr += 4;

    // Peer AS
    *reinterpret_cast<uint16_t *>(ptr) = 0;
    ptr += 2;

    // Attribute length
    *reinterpret_cast<uint16_t *>(ptr) = htons(length);
    ptr += 2;

    XLOG_ASSERT(ptr == &viewbuf[sizeof(viewbuf)]);

    if(fwrite(&viewbuf[0], sizeof(viewbuf), 1, fp) != 1)
	XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(), strerror(errno));

    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;

	if(fwrite(pa->data(), pa->wire_size(), 1, fp) != 1)
	    XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(),
		       strerror(errno));
    }

    fclose(fp);
}

void
text_traffic_dump(const uint8_t *buf, const size_t len, const TimeVal, 
	  const string fname)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    fprintf(fp, bgppp(buf, len).c_str());
    fclose(fp);
}

template <class A>
void
mrtd_routeview_dump(const UpdatePacket* p, const IPNet<A>& net,
		const TimeVal& tv,
		const string fname, int *sequence)
{
    mrtd_routview_dump<A>(p, net, tv, fname, *sequence);
    (*sequence)++;
}

template <class A>
void
mrtd_debug_dump(const UpdatePacket* p, const IPNet<A>& /*net*/,
		const TimeVal& tv,
		const string fname)
{
    size_t len;
    const uint8_t *buf = p->encode(len);
    mrtd_traffic_dump(buf, len , tv, fname);
    delete [] buf;
}

template <class A>
void
text_debug_dump(const UpdatePacket* p, const IPNet<A>& net,
		const TimeVal& tv,
		const string fname)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    fprintf(fp, "%s\n%s\n%s\n", net.str().c_str(), tv.pretty_print().c_str(),
	    p->str().c_str());
    fclose(fp);
}

void
mrtd_replay_dump(const UpdatePacket* p,
		const TimeVal& tv,
		const string fname)
{
    size_t len;
    const uint8_t *buf = p->encode(len);
    mrtd_traffic_dump(buf, len , tv, fname);
    delete [] buf;
}

void
text_replay_dump(const UpdatePacket* p,
		const TimeVal& /*tv*/,
		const string fname)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    fprintf(fp, "%s\n", p->str().c_str());
    fclose(fp);
}

/*
** peer dump <recv/sent> <mtrd/text> <ipv4/ipv6> 
** 0    1    2           3           4           
**
** 	<traffic/routeview/replay/debug> <fname>
** 	5				  6
*/
void
Peer::dump(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    
    if(words.size() < 6)
	xorp_throw(InvalidString,
		   c_format("Insufficient arguments:\n[%s]", line.c_str()));

    /*
    ** Each peer holds two tries. One holds updates sent the other
    ** holds updates received. Determine which trie we are about to
    ** operate on.
    */
    Trie *op;
    Dumper *dumper;
    if("sent" == words[2]) {
	op = &_trie_sent;
	dumper = &_traffic_sent;
    } else if("recv" == words[2]) {
	op = &_trie_recv;
	dumper = &_traffic_recv;
    } else
	xorp_throw(InvalidString,
		   c_format("\"sent\" or \"recv\" accepted not <%s>\n[%s]",
			    words[2].c_str(), line.c_str()));

    bool mrtd;
    if("mrtd" == words[3]) {
	mrtd = true;
    } else if("text" == words[3]) {
	mrtd = false;
    } else
	xorp_throw(InvalidString,
		   c_format("\"mrtd\" or \"text\" accepted not <%s>\n[%s]",
			    words[3].c_str(), line.c_str()));

    bool ipv4 = true;
    if("ipv4" == words[4]) {
	ipv4 = true;
    } else if("ipv6" == words[4]) {
	ipv4 = false;
    } else
	xorp_throw(InvalidString,
		   c_format("\"ipv4\" or \"ipv6\" accepted not <%s>\n[%s]",
			    words[4].c_str(), line.c_str()));
    
    string filename;
    if(words.size() == 7)
	filename = words[6];

    if("traffic" == words[5]) {
	if("" == filename) {
	    dumper->release();
	    return;
	}
 	if(mrtd)
 	    *dumper = callback(mrtd_traffic_dump, filename);
 	else
	    *dumper = callback(text_traffic_dump, filename);
    } else if("routeview" == words[5]) {
	if("" == filename) {
	    xorp_throw(InvalidString,
		       c_format("no filename provided\n[%s]", line.c_str()));
	}
	int sequence = 0;
	if(ipv4) {
	    Trie::TreeWalker_ipv4 tw_ipv4;
	    if(mrtd)
		tw_ipv4 = callback(mrtd_routeview_dump<IPv4>, filename,
				   &sequence);
	    else
		tw_ipv4 = callback(text_debug_dump<IPv4>, filename);
	    op->tree_walk_table(tw_ipv4);
	} else {
	    Trie::TreeWalker_ipv6 tw_ipv6;
	    if(mrtd)
		tw_ipv6 = callback(mrtd_routeview_dump<IPv6>, filename,
				   &sequence);
	    else
		tw_ipv6 = callback(text_debug_dump<IPv6>, filename);
	    op->tree_walk_table(tw_ipv6);
	}
    } else if("replay" == words[5]) {
	if("" == filename) {
	    xorp_throw(InvalidString,
		       c_format("no filename provided\n[%s]", line.c_str()));
	}
	Trie::ReplayWalker rw;
	if(mrtd)
	    rw = callback(mrtd_replay_dump, filename);
	else
	    rw = callback(text_replay_dump, filename);
	op->replay_walk(rw);
    } else if("debug" == words[5]) {
	if("" == filename) {
	    xorp_throw(InvalidString,
		       c_format("no filename provided\n[%s]", line.c_str()));
	}
	if(ipv4) {
	    Trie::TreeWalker_ipv4 tw_ipv4;
	    if(mrtd)
		tw_ipv4 = callback(mrtd_debug_dump<IPv4>, filename);
	    else
		tw_ipv4 = callback(text_debug_dump<IPv4>, filename);
	    op->tree_walk_table(tw_ipv4);
	} else {
	    Trie::TreeWalker_ipv6 tw_ipv6;
	    if(mrtd)
		tw_ipv6 = callback(mrtd_debug_dump<IPv6>, filename);
	    else
		tw_ipv6 = callback(text_debug_dump<IPv6>, filename);
	    op->tree_walk_table(tw_ipv6);
	}
    } else
	xorp_throw(InvalidString,
		   c_format(
"\"traffic\" or \"routeview\" or \"replay\" or \"debug\" accepted not <%s>\n[%s]",
			    words[5].c_str(), line.c_str()));
}

bool
compare_packets(const BGPPacket *base_one, const BGPPacket *base_two)
{
    if(base_one->type() != base_two->type())
	return false;

    switch(base_one->type()) {
	case MESSAGETYPEOPEN:
	    {
		const OpenPacket *one =
		    dynamic_cast<const OpenPacket *>(base_one);
		const OpenPacket *two =
		    dynamic_cast<const OpenPacket *>(base_two);
		return *one == *two;
	    }
	    break;
	case MESSAGETYPEUPDATE:
	    {
		const UpdatePacket *one =
		    dynamic_cast<const UpdatePacket *>(base_one);
		const UpdatePacket *two =
		    dynamic_cast<const UpdatePacket *>(base_two);
		return *one == *two;
	    }
	    break;
	case MESSAGETYPENOTIFICATION:
	    {
		const NotificationPacket *one =
		    dynamic_cast<const NotificationPacket *>(base_one);
		const NotificationPacket *two =
		    dynamic_cast<const NotificationPacket *>(base_two);
		return *one == *two;
	    }
	    break;
	case MESSAGETYPEKEEPALIVE:
	    {
		const KeepAlivePacket *one =
		    dynamic_cast<const KeepAlivePacket *>(base_one);
		const KeepAlivePacket *two =
		    dynamic_cast<const KeepAlivePacket *>(base_two);
		return *one == *two;
	    }
	    break;
	default:
	    XLOG_FATAL("Unexpected BGP message type %d", base_one->type());
    }
    return false;
}

/*
** We just received a BGP message. Check our expected queue for a match.
*/
void
Peer::check_expect(BGPPacket *rec)
{
    /*
    ** If its already gone bad just return.
    */
    if(!_expect._ok)
	return;
    /*
    ** If there is nothing on the queue return.
    */
    if(_expect._list.empty())
	return;
    const BGPPacket *exp = _expect._list.front();

    debug_msg("Expecting: %s\n", exp->str().c_str());

    if(compare_packets(rec, exp)) {
	_expect._list.pop_front();
	delete exp;
    } else {
	debug_msg("Received packet %s did not match Expected\n",
		  rec->str().c_str());

	_expect._ok = false;
	/*
	** Need to go through this performance in order to copy a
	** packet.
	*/
	size_t rec_len;
	const uint8_t *rec_buf = rec->encode(rec_len);

	switch(rec->type()) {
	case MESSAGETYPEOPEN:
	    {
	    OpenPacket *pac = new OpenPacket(rec_buf, rec_len);
	    _expect._bad = pac;
	    }
	    break;
	case MESSAGETYPEUPDATE:
	    {
	    UpdatePacket *pac = new UpdatePacket(rec_buf, rec_len);
	    _expect._bad = pac;
	    }
	    break;
	case MESSAGETYPENOTIFICATION:
	    {
	    NotificationPacket *pac =
		new NotificationPacket(rec_buf, rec_len);
	    _expect._bad = pac;
	    }
	    break;
	case MESSAGETYPEKEEPALIVE:
	    _expect._bad = new KeepAlivePacket(rec_buf, rec_len);
	    break;
	default:
	    XLOG_FATAL("Unexpected BGP message type %d", rec->type());
	}
	delete [] rec_buf;
    }

}

void
Peer::xrl_callback(const XrlError& error, const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    XLOG_ASSERT(0 != _busy);
    _busy--;
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
 	_session = false;
 	_established = false;
    }
}

void
Peer::xrl_callback_connected(const XrlError& error, const char *comment)
{
    debug_msg("callback_connected %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() == error) {
	_connected = true;
    } else {
	_connected = false;
    }
    xrl_callback(error, comment);
}

/*
** This method receives data from the BGP process under test via the
** test_peer. The test_peer attempts to packetise the data into BGP
** messages.
*/
void
Peer::datain(const bool& status, const TimeVal& tv,
	     const vector<uint8_t>& data)
{
    debug_msg("status: %d secs: %lu micro: %lu data length: %u\n",
	      status, (unsigned long)tv.sec(), (unsigned long)tv.usec(),
	      (uint32_t)data.size());

    /*
    ** A bgp error has occured.
    */
    if(false == status) {
	/*
	** BGP has closed the connection to the test_peer.
	*/
	if(0 == data.size()) {
	    XLOG_WARNING("TCP connection from test_peer: %s to: %s closed",
		       _peername.c_str(), _target_hostname.c_str());
	    _connected = false;
	    _session = false;
	    _established = false;
	    return;
	}
	XLOG_FATAL("Bad BGP message received by test_peer: %s from: %s",
		   _peername.c_str(), _target_hostname.c_str());
    }

    size_t length = data.size();
    uint8_t *buf = new uint8_t[length];
    for(size_t i = 0; i < length; i++)
	buf[i] = data[i];

    const fixed_header *header = 
	reinterpret_cast<const struct fixed_header *>(buf);

    if (!_traffic_recv.is_empty())
	_traffic_recv->dispatch(buf, length, tv);

    try {
	switch(header->type) {
	case MESSAGETYPEOPEN: {
	    debug_msg("OPEN Packet RECEIVED\n");
	    OpenPacket pac(buf, length);
	    debug_msg(pac.str().c_str());
	    
	    if(_session && !_established && _passive)
		send_open();
	    check_expect(&pac);
	}
	    break;
	case MESSAGETYPEKEEPALIVE: {
	    debug_msg("KEEPALIVE Packet RECEIVED %u\n", (uint32_t)length);
	    KeepAlivePacket pac(buf, length);
	    debug_msg(pac.str().c_str());

	    /* XXX
	    ** Send any received keepalive right back.
	    ** At the moment we are not bothering to maintain a timer
	    ** to send keepalives. An incoming keepalive prompts a
	    ** keepalive response. At the beginning of a conversation
	    ** this keepalive response will cause the peer to go to
	    ** established. As opens must have been exchanged for the
	    ** peer to send a keepalive. 
	    ** 
	    */
	    if(_keepalive || (_session && !_established)) {
		XrlTestPeerV0p1Client test_peer(_xrlrouter);
		debug_msg("KEEPALIVE Packet SENT\n");
		_busy++;
		if(!test_peer.send_send(_peername.c_str(), data,
				    callback(this, &Peer::xrl_callback,
					     "keepalive")))
		    XLOG_FATAL("send_send failed");
	    }
	    _established = true;
	    check_expect(&pac);
	}
	    break;
	case MESSAGETYPEUPDATE: {
	    debug_msg("UPDATE Packet RECEIVED\n");
	    UpdatePacket pac(buf, length);
	    debug_msg(pac.str().c_str());
	    /*
	    ** Save the update message in the receive trie.
	    */
	    _trie_recv.process_update_packet(tv, buf, length);
	    check_expect(&pac);
	}
	    break;
	case MESSAGETYPENOTIFICATION: {
	    debug_msg("NOTIFICATION Packet RECEIVED\n");
	    NotificationPacket pac(buf, length);
	    debug_msg(pac.str().c_str());
	    check_expect(&pac);
	}
	    break;
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    XLOG_ERROR("Unknown packet type %d", header->type);
	}
    } catch(CorruptMessage c) {
	/*
	** This peer had sent us a bad message.
	*/
	XLOG_WARNING("From peer %s: %s", _peername.c_str(),
		     c.why().c_str());
    }

    delete [] buf;
}

void 
Peer::datain_error(const string& reason)
{
    XLOG_WARNING("Error on TCP connection from test_peer: %s to %s reason: %s",
		 _peername.c_str(), _target_hostname.c_str(), reason.c_str());
}

void
Peer::datain_closed()
{
    XLOG_WARNING("TCP connection from test_peer: %s to %s closed",
		 _peername.c_str(), _target_hostname.c_str());
    _connected = false;
    _session = false;
    _established = false;
}

void
Peer::send_message(const uint8_t *buf, const size_t len, SMCB cb)
{
    debug_msg("len: %u\n", (uint32_t)len);

    if(!_traffic_sent.is_empty()) {
	TimeVal tv;
	_eventloop->current_time(tv);
	_traffic_sent->dispatch(buf, len, tv);
    }

    vector<uint8_t> v(len);

    for(size_t i = 0; i < len; i++)
	v[i] = buf[i];
    delete [] buf;
    
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    if(!test_peer.send_send(_peername.c_str(), v, cb))
	XLOG_FATAL("send_send failed");
}

void
Peer::send_open()
{
    /*
    ** Create an open packet and send it in.
    */
    OpenPacket bgpopen(_as, _id, _holdtime);
    if(_ipv6)
	bgpopen.add_parameter(
		new BGPMultiProtocolCapability(AFI_IPV6, SAFI_UNICAST));
    size_t len;
    const uint8_t *buf = bgpopen.encode(len);
    debug_msg("OPEN Packet SENT\n%s", bgpopen.str().c_str());
    _busy++;
    send_message(buf, len, callback(this, &Peer::xrl_callback, "open"));
}

/*
** The standard BGP code that deals with path attributes contains
** fairly stringent checking. For testing we want to be able to create
** arbitary path attributes.
*/
class AnyAttribute : public PathAttribute {
public:

    /*
    ** The attr string should be a comma separated list of numbers.
    */

    AnyAttribute(const char *attr) throw(InvalidString)
	// In order to protect against accidents in the BGP code,
	// PathAttribute does not have a default constructor. However,
	// we need to manipulate a PathAttribute so pass in a valid
	// constructor argument.
	: PathAttribute(&_valid[0]) {

	_init_string = attr;

	string line = attr;
	vector<string> v;
	tokenize(line, v, ",");

	_data = new uint8_t [v.size()];
	for(size_t i = 0; i < v.size(); i++) {
	    _data[i] = strtol(v[i].c_str(), 0, 0);
// 	    fprintf(stderr, "%#x ", _data[i]);
	}
// 	fprintf(stderr, "\n");

    };
    
    PathAttribute *clone() const { 
	return new AnyAttribute(_init_string.c_str());
    }
private:
    static const uint8_t _valid[];
    string _init_string;
};

const uint8_t AnyAttribute::_valid[] = {0x80|0x40, 255, 1, 1};

/*
** The input is a comma separated list of numbers that are turned into
** an array.
*/
PathAttribute *
Peer::path_attribute(const char *) const throw(InvalidString)
{
    const uint8_t path[] = {0x80|0x40, 255, 1, 1};
    uint16_t max_len = sizeof(path);
    size_t actual_length;

    return PathAttribute::create(&path[0], max_len, actual_length);
}

/*
** Generate a BGP message from the textual description.
** Note: It is up to the caller of this method to delete the packet
** that has been created.
*/
const BGPPacket *
Peer::packet(const string& line, const vector<string>& words, int index)
    const throw(InvalidString)
{
    BGPPacket *pac = 0;

  try {
    if("notify" == words[index]) {
	switch(words.size() - (index + 1)) {
	case 1:
	    pac = new NotificationPacket(atoi(words[index + 1].c_str()));
	    break;
	case 2:
	    pac = new NotificationPacket(atoi(words[index + 1].c_str()),
					    atoi(words[index + 2].c_str()));
	    break;
	default: {
	    int errlen = words.size() - (index + 3);
	    if (errlen < 1)
		xorp_throw(InvalidString,
			   c_format(
			   "Incorrect number of arguments to notify:\n[%s]",
			   line.c_str()));
	    uint8_t buf[errlen];
	    for (int i=0; i< errlen; i++) {
		int value = atoi(words[index + 3 + i].c_str());
		if (value < 0 || value > 255)
		    xorp_throw(InvalidString,
			       c_format(
			       "Incorrect byte value to notify:\n[%s]",
			       line.c_str()));
			
		buf[i] = (uint8_t)value;
		pac = new NotificationPacket(atoi(words[index+1].c_str()),
					     atoi(words[index+2].c_str()),
					     buf, errlen);
	    }
	}
	}
    } else if("update" == words[index]) {
	size_t size = words.size();
	if(0 != ((size - (index + 1)) % 2))
	    xorp_throw(InvalidString,
	       c_format("Incorrect number of arguments to update:\n[%s]",
				line.c_str()));

	UpdatePacket *bgpupdate = new UpdatePacket();
	MPReachNLRIAttribute<IPv6> mpipv6_nlri(SAFI_UNICAST);
	MPUNReachNLRIAttribute<IPv6> mpipv6_withdraw(SAFI_UNICAST);

	for(size_t i = index + 1; i < size; i += 2) {
	    debug_msg("name: %s value: <%s>\n",
		      words[i].c_str(),
		      words[i + 1].c_str());
	    if("origin" == words[i]) {
		OriginAttribute oa(static_cast<OriginType>
				   (atoi(words[i + 1].c_str())));
		bgpupdate->add_pathatt(oa);
	    } else if("aspath" == words[i]) {
		string aspath = words[i+1];
		ASPathAttribute aspa(AsPath(aspath.c_str()));
		bgpupdate->add_pathatt(aspa);
		debug_msg("aspath: %s\n", 
			  AsPath(aspath.c_str()).str().c_str());
	    } else if("nexthop" == words[i]) {
		IPv4NextHopAttribute nha(IPv4((const char*)
					      (words[i+1].c_str())));
		bgpupdate->add_pathatt(nha);
	    } else if("nexthop6" == words[i]) {
		mpipv6_nlri.set_nexthop(IPv6((const char*)
					     (words[i+1].c_str())));
	    } else if("localpref" == words[i]) {
		LocalPrefAttribute lpa(atoi(words[i+1].c_str()));
		bgpupdate->add_pathatt(lpa);
	    } else if("nlri" == words[i]) {
		BGPUpdateAttrib upa(IPv4Net((const char*)
					    (words[i+1].c_str())));
		bgpupdate->add_nlri(upa);
	    } else if("nlri6" == words[i]) {
		mpipv6_nlri.add_nlri(words[i+1].c_str());
	    } else if("withdraw" == words[i]) {
		BGPUpdateAttrib upa(IPv4Net((const char*)
					    (words[i+1].c_str())));
		bgpupdate->add_withdrawn(upa);
	    } else if("withdraw6" == words[i]) {
		mpipv6_withdraw.add_withdrawn(IPv6Net(words[i+1].c_str()));
	    } else if("med" == words[i]) {
		MEDAttribute ma(atoi(words[i+1].c_str()));
		bgpupdate->add_pathatt(ma);
	    } else if("pathattr" == words[i]) {
		AnyAttribute aa(words[i+1].c_str());
		bgpupdate->add_pathatt(aa);
	    } else
		xorp_throw(InvalidString, 
		       c_format("Illegal argument to update: <%s>\n[%s]",
				words[i].c_str(), line.c_str()));
	}
	if(!mpipv6_nlri.nlri_list().empty()) {
 	    mpipv6_nlri.encode();
	    bgpupdate->add_pathatt(mpipv6_nlri);
	}
	if(!mpipv6_withdraw.wr_list().empty()) {
	    mpipv6_withdraw.encode();
	    bgpupdate->add_pathatt(mpipv6_withdraw);
	}

	pac = bgpupdate;
    } else if("open" == words[index]) {
	size_t size = words.size();
	if(0 != ((size - (index + 1)) % 2))
	    xorp_throw(InvalidString,
	       c_format("Incorrect number of arguments to update:\n[%s]",
				line.c_str()));

	string asnum;
	string bgpid;
	string holdtime;

	for(size_t i = index + 1; i < size; i += 2) {
	    debug_msg("name: %s value: %s\n",
		      words[i].c_str(),
		      words[i + 1].c_str());
	    if("asnum" == words[i]) {
		asnum = words[i + 1];
	    } else if("bgpid" == words[i]) {
		bgpid = words[i + 1];
	    } else if("holdtime" == words[i]) {
		holdtime = words[i + 1];
	    } else 
		xorp_throw(InvalidString, 
		       c_format("Illegal argument to open: <%s>\n[%s]",
				words[i].c_str(), line.c_str()));
	}

	if("" == asnum) {
	    xorp_throw(InvalidString,
	       c_format("AS number not supplied to open:\n[%s]",
				line.c_str()));
	}
	if("" == bgpid) {
	    xorp_throw(InvalidString,
	       c_format("BGP ID not supplied to open:\n[%s]",
				line.c_str()));
	}
	if("" == holdtime) {
	    xorp_throw(InvalidString,
	       c_format("Holdtime not supplied to open:\n[%s]",
				line.c_str()));
	}
 	OpenPacket *open = new OpenPacket(AsNum(
				 static_cast<uint16_t>(atoi(asnum.c_str()))),
					  IPv4(bgpid.c_str()),
					  atoi(holdtime.c_str()));
	pac = open;
    } else if("keepalive" == words[index]) {
	pac = new KeepAlivePacket();
    } else
	xorp_throw(InvalidString,
		   c_format("\"notify\" or"
			    " \"update\" or"
			    " \"open\" or"
			    " \"keepalive\" accepted"
			    " not <%s>\n[%s]",
			    words[index].c_str(), line.c_str()));
  } catch(CorruptMessage c) {
       xorp_throw(InvalidString, c_format("Unable to construct packet "
					"%s\n[%s])", c.why().c_str(),
					line.c_str()));
  }
	
  debug_msg("%s\n", pac->str().c_str());
  
  return pac;
}
