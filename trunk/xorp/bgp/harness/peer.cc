// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/harness/peer.cc,v 1.13 2003/01/28 03:21:52 rizzo Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include <string>
#include <stdlib.h>

#include "config.h"
#include "bgp/bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4net.hh"

#include "bgp/local_data.hh"
#include "bgp/packet.hh"

#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/test_peer_xif.hh"
#include "peer.hh"
#include "bgppp.hh"

Peer::Peer() : _as(AsNum::AS_INVALID) // XXX
{
}

Peer::~Peer()
{
}

Peer::Peer(XrlRouter *xrlrouter, string peername, string target_hostname,
	   string target_port)
    : _xrlrouter(xrlrouter),
      _peername(peername),
      _target_hostname(target_hostname),
      _target_port(target_port),
      _session(false),
      _passive(false),
      _keepalive(false),
      _established(false),
      _as(AsNum::AS_INVALID), // XXX
      _holdtime(0)
{
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

    _xrlrouter = rhs._xrlrouter;

    _peername = rhs._peername;
    _target_hostname = rhs._target_hostname;
    _target_port = rhs._target_port;

    _session = rhs._session;
    _passive = rhs._passive;
    _keepalive = rhs._keepalive;
    _established = rhs._established;

    _as = rhs._as;
    _holdtime = rhs._holdtime;
    _id = rhs._id;
}

bool
Peer::pending()
{
    if(_session && !_established && !_passive)
	return true;

    return false;
}

void 
Peer::listen(const string& /*line*/, const vector<string>& /*words*/)
	throw(InvalidString)
{
    /*
    ** Retrieved from the router..
    */
    const string coordinator = _xrlrouter->name();

    /* Connect the test peer to the target BGP */
    debug_msg("About to listen on: %s\n", _peername.c_str());
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    test_peer.send_register(_peername.c_str(), coordinator,
			    ::callback(this, &Peer::callback, "register"));
    test_peer.send_packetisation(_peername.c_str(), "bgp",
			   ::callback(this, &Peer::callback, "packetisation"));
    test_peer.send_listen(_peername.c_str(),
			   _target_hostname, atoi(_target_port.c_str()),
			   ::callback(this, &Peer::callback, "listen"));
}

void 
Peer::connect(const string& /*line*/, const vector<string>& /*words*/)
	throw(InvalidString)
{
    /*
    ** Retrieved from the router..
    */
    const string coordinator = _xrlrouter->name();

    /* Connect the test peer to the target BGP */
    debug_msg("About to connect to: %s\n", _peername.c_str());
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    test_peer.send_register(_peername.c_str(), coordinator,
			    ::callback(this, &Peer::callback, "register"));
    test_peer.send_packetisation(_peername.c_str(), "bgp",
			   ::callback(this, &Peer::callback, "packetisation"));
    test_peer.send_connect(_peername.c_str(),
			   _target_hostname, atoi(_target_port.c_str()),
			   ::callback(this, &Peer::callback, "connect"));
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
** At the moment the form of the command is:
** peer1 send packet update \
**                   origin 0 \
**                   aspath 1/2/3 \
**                   nexthop 10.10.10.10 \
**                   nlri 1.2.3/24
**
*/
void
Peer::send_packet(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    /* XXX
    ** For the time being only handle sending update packets.
    ** Make sending updates a separate routine when we support sending
    ** other types of packets.
    */
    const char update[] = "update";
    if(update != words[3])
	xorp_throw(InvalidString,
		   c_format("Third argument should be %s not <%s>\n[%s]",
			    update, words[3].c_str(), line.c_str()));
    
    /*
    ** Parse the command line.
    ** The arguments for this command come in pairs:
    ** name value
    */

    size_t size = words.size();
    if(0 != (size % 2))
	xorp_throw(InvalidString,
		   c_format("Incorrect number of arguments:\n[%s]",
			    line.c_str()));

    const BGPPacket *pkt = Peer::packet(line, words, 3);

    size_t len;
    const uint8_t *buf = pkt->encode(len);
    delete pkt;
    /*
    ** Save the update message in the sent trie.
    */
    struct timeval tp;
    struct timezone tzp;
    if(0 != gettimeofday(&tp, &tzp))
	XLOG_FATAL("gettimeofday failed: %s", strerror(errno));
    _trie_sent.process_update_packet(tp, buf, len);

    send_message(buf, len, ::callback(this, &Peer::callback, "update"));
    fprintf(stderr, "done with send_pkt %p size %d\n",
	buf, len);
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

const
uint8_t *
mrtd_traffic_send(FILE *fp, size_t& len)
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
** peer send dump mrtd update fname
** 0    1    2    3    4      5
*/
void
Peer::send_dump(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    if(6 != words.size())
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
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));


    /*
    ** It could take quite a while to send a large file so we need to
    ** set up the transfer and then return.
    */
    send_dump_callback(XrlError::OKAY(), fp, "mrtd_traffic_send");
}

void
Peer::send_dump_callback(const XrlError& error, FILE *fp, const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
    }

    size_t len;
    const uint8_t *buf;

    while(0 != (buf = mrtd_traffic_send(fp, len))) {
	const fixed_header *header = 
	    reinterpret_cast<const struct fixed_header *>(buf);
	if(MESSAGETYPEUPDATE == header->type) {
	    _smcb = ::callback(this, &Peer::send_dump_callback,
			       fp,
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
    if(words.size() < 2)
	xorp_throw(InvalidString,
		   c_format("Insufficient arguments:\n[%s]", line.c_str()));

    if("packet" == words[2]) {
	_expect._list.push_back(packet(line, words, 3));
#if	0
    } else if("check" == words[2]) {
	if(!_expect._ok)
	    xorp_throw(InvalidString,
	       c_format("Expect queue violated\nExpect: %sReceived: %s",
			_expect._list.front()->str().c_str(),
			_expect._bad->str().c_str()));
	    
	switch(words.size()) {
	case 4:
	    break;
	case 5:
	    if(static_cast<unsigned int>(atoi(words[4].c_str())) !=
	       _expect._list.size())
		xorp_throw(InvalidString, 
			   c_format("Expected list size to be %d actual %d",
			      atoi(words[4].c_str()), _expect._list.size()));
	    break;
	default:
	    xorp_throw(InvalidString, 
		   c_format("Illegal number of arguments to \"check\"\n[%s]",
			    line.c_str()));
	    break;
	}
    } else if("established" == words[2]) {
	if(!_established)
	    xorp_throw(InvalidString,
		       c_format("No session established"));
#endif
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
** peer assert established
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
    } else if("established" == words[2]) {
	if(!_established)
	    xorp_throw(InvalidString,
		       c_format("No session established"));
    } else
	xorp_throw(InvalidString,
		   c_format(
		   "\"queue\" \"established\" accepted not <%s>\n[%s]",
		   words[2].c_str(), line.c_str()));
}

void
mrtd_traffic_dump(const uint8_t *buf, const size_t len , const timeval tv,
	  const string fname)
{
#if	0
    {
    /* XXX
    ** Only save update messages for the time being.
    */
    const fixed_header *header = 
	reinterpret_cast<const struct fixed_header *>(buf);
    struct fixed_header fh = *header;
    if(MESSAGETYPEUPDATE != header->type)
	return;
    }
#endif
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    mrt_header header;
    header.time = htonl(tv.tv_sec);
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

void
text_traffic_dump(const uint8_t *buf, const size_t len, const timeval, 
	  const string fname)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    fprintf(fp, bgppp(buf, len).c_str());
    fclose(fp);
}

void
mrtd_debug_dump(const UpdatePacket* p, const IPv4Net& /*net*/,
		const timeval& tv,
		const string fname)
{
    size_t len;
    const uint8_t *buf = p->encode(len);
    mrtd_traffic_dump(buf, len , tv, fname);
    delete [] buf;
}

void
text_debug_dump(const UpdatePacket* p, const IPv4Net& net,
		const timeval& /*tv*/,
		const string fname)
{
    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(), strerror(errno));

    fprintf(fp, "%s\n%s\n", net.str().c_str(), p->str().c_str());
    fclose(fp);
}

/*
** peer dump <recv/sent> <mtrd/text> <traffic/routeview/current/debug> <fname>
** 0    1    2           3           4                                 5
*/
void
Peer::dump(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    
    if(words.size() < 5)
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
    
    string filename;
    if(words.size() == 6)
	filename = words[5];

    if("traffic" == words[4]) {
	if("" == filename) {
	    dumper->release();
	    return;
	}
 	if(mrtd)
 	    *dumper = ::callback(mrtd_traffic_dump, filename);
 	else
	    *dumper = ::callback(text_traffic_dump, filename);
    } else if("routeview" == words[4]) {
    } else if("current" == words[4]) {
    } else if("debug" == words[4]) {
	if("" == filename) {
	    xorp_throw(InvalidString,
		       c_format("no filename provided\n[%s]", line.c_str()));
	}
	Trie::TreeWalker tw;
	if(mrtd)
	    tw = ::callback(mrtd_debug_dump, filename);
	else
	    tw = ::callback(text_debug_dump, filename);
	op->tree_walk_table(tw);
    } else
	xorp_throw(InvalidString,
		   c_format(
"\"traffic\" or \"routeview\" or \"current\" or \"debug\" accepted not <%s>\n[%s]",
			    words[4].c_str(), line.c_str()));
	
#if	0
    FILE *fp = fopen(words[2].c_str(), "w");
    if(0 == fp)
	xorp_throw(InvalidString,
		   c_format("fopen of %s failed: %s\n[%s]", words[2].c_str(),
			    strerror(errno), line.c_str()));
    _trie_recv.save_routing_table(fp);
    fclose(fp);
#endif
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
Peer::callback(const XrlError& error, const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
	_session = false;
	_established = false;
    }
}

/*
** This method receives data from the BGP process under test via the
** test_peer. The test_peer attempts to packetise the data into BGP
** messages.
*/
void
Peer::datain(const bool& status, const timeval& tv,
	     const vector<uint8_t>& data)
{
    debug_msg("status: %d secs: %lu micro: %lu data length: %u\n",
	      status, (unsigned long)tv.tv_sec, (unsigned long)tv.tv_usec,
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
		test_peer.send_send(_peername.c_str(), data,
				    ::callback(this, &Peer::callback,
					       "keepalive"));
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
    _session = false;
    _established = false;
}

void
Peer::send_message(const uint8_t *buf, const size_t len, SMCB cb)
{
    debug_msg("len: %u\n", (uint32_t)len);

    if(!_traffic_sent.is_empty()) {
	struct timeval tp;
	struct timezone tzp;
	if(0 != gettimeofday(&tp, &tzp))
	    XLOG_FATAL("gettimeofday failed: %s", strerror(errno));
	_traffic_sent->dispatch(buf, len, tp);
    }

    vector<uint8_t> v(len);

    for(size_t i = 0; i < len; i++)
	v[i] = buf[i];
    delete [] buf;
    
    XrlTestPeerV0p1Client test_peer(_xrlrouter);
    test_peer.send_send(_peername.c_str(), v, cb);
}

void
Peer::send_open()
{
    /*
    ** Create an open packet and send it in.
    */
    OpenPacket bgpopen(_as, _id, _holdtime);
    size_t len;
    const uint8_t *buf = bgpopen.encode(len);
    debug_msg("OPEN Packet SENT\n%s", bgpopen.str().c_str());
    send_message(buf, len, ::callback(this, &Peer::callback, "open"));
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
			       c_format("Incorrect number of arguments to notify:\n[%s]",
					line.c_str()));
		uint8_t buf[errlen];
		for (int i=0; i< errlen; i++) {
		    int value = atoi(words[index + 3 + i].c_str());
		    if (value < 0 || value > 255)
			xorp_throw(InvalidString,
				   c_format("Incorrect byte value to notify:\n[%s]",
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

	for(size_t i = index + 1; i < size; i += 2) {
	    debug_msg("name: %s value: %s\n",
		      words[i].c_str(),
		      words[i + 1].c_str());
	    if("origin" == words[i]) {
		bgpupdate->add_pathatt(OriginAttribute(static_cast<OriginType>
				     (atoi(words[i + 1].c_str()))));
	    } else if("aspath" == words[i]) {
		string aspath = words[i+1];
		bgpupdate->add_pathatt(ASPathAttribute(AsPath(
						       aspath.c_str())));
		debug_msg("aspath: %s\n", 
			  AsPath(aspath.c_str()).str().c_str());
	    } else if("nexthop" == words[i]) {
		bgpupdate->add_pathatt(IPv4NextHopAttribute(IPv4(
						      words[i+1].c_str())));
	    } else if("localpref" == words[i]) {
		bgpupdate->add_pathatt(LocalPrefAttribute(atoi(
						      words[i+1].c_str())));
	    } else if("nlri" == words[i]) {
		bgpupdate->add_nlri(BGPUpdateAttrib(IPv4Net(
						      words[i+1].c_str())));
	    } else if("withdraw" == words[i]) {
		bgpupdate->add_withdrawn(BGPUpdateAttrib(IPv4Net(
						      words[i+1].c_str())));
	    } else if("med" == words[i]) {
		bgpupdate->add_pathatt(MEDAttribute(atoi(
						      words[i+1].c_str())));
	    } else
		xorp_throw(InvalidString, 
		       c_format("Illegal argument to update: <%s>\n[%s]",
				words[i].c_str(), line.c_str()));
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
    } else  if("keepalive" == words[index]) {
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
	
    return pac;
}
