// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2012 XORP, Inc and Others
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


#ifndef __OLSR_MESSAGE_HH__
#define __OLSR_MESSAGE_HH__

/**
 * @short An OLSR protocol message.
 */
class Message {
public:
    virtual ~Message() {}

    inline TimeVal	receive_time() const
	{ return _receive_time; }

    inline TimeVal	expiry_time() const
	{ return _expiry_time; }

    inline bool		valid() const
	{ return _is_valid; }

    inline bool		is_first() const
	{ return _is_first; }

    inline bool		is_last() const
	{ return _is_last; }

    inline void		set_is_first(bool arg)
	{ _is_first = arg; }

    inline void		set_is_last(bool arg)
	{ _is_last = arg; }

    inline bool		forwarded() const
	{ return _is_forwarded; }

    inline OlsrTypes::FaceID	faceid() const
	{ return _faceid; }

    inline uint8_t	hops() const
	{ return _hops; }

    inline void		incr_hops() { ++_hops; }

    inline uint16_t	seqno() const
	{ return _seqno; }

    inline uint8_t	ttl() const
	{ return _ttl; }

    inline void		decr_ttl() { --_ttl; }

    inline IPv4		origin() const
	{ return _origin; }

    inline OlsrTypes::MessageType	type() const
	{ return _type; }

    inline void	set_hop_count(uint8_t hops)
	{ _hops = hops; }

    inline void set_forwarded(bool is_forwarded)
	{ _is_forwarded = is_forwarded; }

    inline void set_expiry_time(const TimeVal& expiry_time)
	{ _expiry_time = expiry_time; }

    inline void set_receive_time(const TimeVal& receive_time)
	{ _receive_time = receive_time; }

    inline void	set_seqno(uint16_t seqno)
	{ _seqno = seqno; }

    inline void	set_ttl(uint8_t ttl)
	{ _ttl = ttl; }

    inline void	set_type(OlsrTypes::MessageType type)
	{ _type = type; }

    inline void set_valid(bool is_valid)
	{ _is_valid = is_valid; }

    inline void set_origin(IPv4 origin)
	{ _origin = origin; }

    inline void set_faceid(OlsrTypes::FaceID faceid)
	{ _faceid = faceid; }

    virtual Message* decode(uint8_t* buf, size_t& len)
	throw(InvalidMessage) = 0;

    virtual bool encode(uint8_t* buf, size_t& len) = 0;

    virtual size_t	length() const = 0;

    virtual string str() const = 0;

    string common_str() const;

    /*
     * @return the length of the header common to all types of OLSR
     *         protocol message.
     * As this varies according to the protocol family in use, it needs
     * to become templatized.
     */
    static size_t get_common_header_length() {
	return	sizeof(uint8_t) +	// message type
		sizeof(uint8_t) +	// validity time
		sizeof(uint16_t) +	// message size (if not RA-OLSR)
		IPv4::addr_bytelen() +	// IPv4 origin: family dependent
		sizeof(uint8_t) +	// time-to-live
		sizeof(uint8_t) +	// hop count
		sizeof(uint16_t);	// message sequence number
    }

    uint16_t adv_message_length() const { return _adv_message_length; }

protected:
    size_t decode_common_header(uint8_t* buf, size_t& len)
	throw(InvalidMessage);

    bool encode_common_header(uint8_t* buf, size_t& len);

    void store(uint8_t* ptr, size_t len) {
	_msg.resize(len);
	memcpy(&_msg[0], ptr, len);
    }

protected:
    // common message data and fields
    TimeVal	_receive_time;	// time when this message was received
    TimeVal	_expiry_time;	// this message will self destruct in
				// C*(1+a/16)* 2^b seconds!
    bool	_is_valid;	// this message is valid
    bool	_is_forwarded;	// this message has already been forwarded
    bool	_is_first;	// this message is the first in the packet
    bool	_is_last;	// this message is the last in the packet
    OlsrTypes::FaceID	_faceid;	// interface where this was received
    IPv4	_origin;	// who sent it
    uint8_t	_type;		// type of message
    uint8_t	_ttl;		// time-to-live; updated when forwarded
    uint8_t	_hops;		// hop count; updated when forwarded
    uint16_t	_seqno;		// message sequence number

    uint16_t	_adv_message_length;    // length field seen on wire; used by
				        // derived class decoders.

    vector<uint8_t> _msg;	// on-wire format
};

/**
 * @short Helper class which represents the link code used in a link tuple.
 */
class LinkCode {
private:
    static const char* linktype_to_str(OlsrTypes::LinkType t);
    static const char* neighbortype_to_str(OlsrTypes::NeighborType t);
public:
    LinkCode() : _linkcode(OlsrTypes::UNSPEC_LINK) { }

    LinkCode(OlsrTypes::NeighborType ntype, OlsrTypes::LinkType ltype)
     throw(BadLinkCode) {
	_linkcode = ((ntype << 2) & 0x0C) | (ltype & 0x03);
	throw_if_not_valid();
    }

    LinkCode(uint8_t code)
     throw(BadLinkCode)
     :  _linkcode(code) {
	throw_if_not_valid();
    }

    LinkCode(const LinkCode& rhs)
     : _linkcode(rhs._linkcode)
    {}

    LinkCode& operator=(const LinkCode& rhs) {
	_linkcode = rhs._linkcode;
	return (*this);
    }

    inline LinkCode& operator=(const uint8_t& rhs)
     throw(BadLinkCode) {
	_linkcode = rhs;
	throw_if_not_valid();
	return (*this);
    }

    inline operator uint8_t() const {
	return _linkcode;
    }

    inline OlsrTypes::NeighborType neighbortype() const {
	return (_linkcode & 0x0C) >> 2;
    }

    inline OlsrTypes::LinkType linktype() const {
	return _linkcode & 0x03;
    }

    inline bool is_unspec_link() const {
	return linktype() == OlsrTypes::UNSPEC_LINK;
    }

    inline bool is_asym_link() const {
	return linktype() == OlsrTypes::ASYM_LINK;
    }

    inline bool is_sym_link() const {
	return linktype() == OlsrTypes::SYM_LINK;
    }

    inline bool is_lost_link() const {
	return linktype() == OlsrTypes::LOST_LINK;
    }

    inline bool is_mpr_neighbor() const {
	return neighbortype() == OlsrTypes::MPR_NEIGH;
    }

    inline bool is_sym_neighbor() const {
	return neighbortype() == OlsrTypes::SYM_NEIGH;
    }

    inline bool is_not_neighbor() const {
	return neighbortype() == OlsrTypes::NOT_NEIGH;
    }

    inline string str() const {
	return c_format("link %s neighbor %s",
	    linktype_to_str(linktype()), neighbortype_to_str(neighbortype()));
    }

private:
    inline void throw_if_not_valid() {
	if (!is_valid()) {
		xorp_throw(BadLinkCode,
			   c_format("Bad link code: neighbor %u link %u",
				    XORP_UINT_CAST(neighbortype()),
				    XORP_UINT_CAST(linktype())));
	}
    }

    inline bool is_valid() {
	if (linktype() > OlsrTypes::LINKTYPE_END ||
	    neighbortype() > OlsrTypes::NEIGHBORTYPE_END ||
	    (linktype() == OlsrTypes::SYM_LINK &&
	     neighbortype() == OlsrTypes::NOT_NEIGH)) {
	    return false;
	}
	return true;
    }

private:
    uint8_t	_linkcode;
};


/**
 * @short Wrapper for per-address information found in HELLO and TC.
 *
 * This is space-pessimized for the case where ETX measurements are in use.
 */
class LinkAddrInfo {
public:
    explicit LinkAddrInfo(const bool has_lq)
	    : _has_etx(has_lq), _near_etx(0), _far_etx(0)
    {}

    explicit LinkAddrInfo(const IPv4& addr)
	    : _has_etx(false), _remote_addr(addr), _near_etx(0), _far_etx(0)
    {}

    explicit LinkAddrInfo(const IPv4& addr,
	const double& near_etx, const double& far_etx)
     : _has_etx(true),
       _remote_addr(addr),
       _near_etx(near_etx),
       _far_etx(far_etx)
    {}
#ifdef XORP_USE_USTL
    LinkAddrInfo() { }
#endif

    bool has_etx() const { return _has_etx; }
    IPv4 remote_addr() const { return _remote_addr; }
    double near_etx() const { return _near_etx; }
    double far_etx() const { return _far_etx; }

    inline size_t size() const {
	size_t byte_count = IPv4::addr_bytelen();
	if (has_etx())
	    byte_count += (sizeof(uint8_t) * 2);
	return byte_count;
    }

    size_t copy_in(const uint8_t *from_uint8);
    size_t copy_out(uint8_t* to_uint8) const;

    inline string str() const {
	string str = _remote_addr.str();
	if (has_etx()) {
	    str += c_format("[nq %.2f, fq %.2f]",
			    near_etx(),
			    far_etx());
	}
	return str;
    }

private:
    bool	_has_etx;
    IPv4	_remote_addr;

    double	_near_etx;
    double	_far_etx;
};

/**
 * @short Representation of a HELLO protocol message.
 */
class HelloMessage : public Message {
public:
    typedef multimap<LinkCode, LinkAddrInfo> LinkBag;

public:
    HelloMessage()
	{ this->set_type(OlsrTypes::HELLO_MESSAGE); }
    ~HelloMessage() {}

    Message* decode(uint8_t* buf, size_t& len) throw(InvalidMessage);
    bool encode(uint8_t* buf, size_t& len);

    inline size_t	min_length() const {
	return	    get_common_header_length() +
		    sizeof(uint16_t) + // reserved
		    sizeof(uint8_t) +	// Htime
		    sizeof(uint8_t);	// Willingness
    }

    size_t	length() const {
	size_t len = get_common_header_length() +
		     sizeof(uint16_t) +	// reserved
		     sizeof(uint8_t) +	// Htime
		     sizeof(uint8_t) +	// Willingness
		     get_links_length();
	return (len);
    }

    inline const TimeVal get_htime() const
	{ return _htime; }

    inline void set_htime(const TimeVal& htime)
	{ _htime = htime; }

    inline OlsrTypes::WillType willingness() const
	{ return _willingness; }

    inline void set_willingness(OlsrTypes::WillType willingness)
	{ _willingness = willingness; }

    inline void add_link(const LinkCode code, const IPv4& remote_addr) {
	add_link(code, LinkAddrInfo(remote_addr));
    }

    /**
     * Remove a given neighbor interface address from ALL link tuples.
     */
    size_t remove_link(const IPv4& remote_addr);

    inline void clear() {
	_htime = TimeVal::ZERO();
	_willingness = OlsrTypes::WILL_DEFAULT;
	_links.clear();
    }

    inline const LinkBag& links() const { return _links; }

    /**
     * Print a HelloMessage as a string.
     */
    virtual string str() const;

    /**
     * Calculate the on-wire size of all link state tuples.
     */
    virtual size_t get_links_length() const;

protected:
    inline void add_link(const LinkCode code,
			 const LinkAddrInfo& lai) {
	_links.insert(make_pair(code, lai));
    }

    /**
     * Decode a single link tuple from the buffer into the HelloMessage.
     *
     * @param buf pointer to the buffer to decode.
     * @param len the number of bytes in the buffer.
     * @param skiplen the number of bytes consumed by this function.
     * @param has_lq true if this function is being called from derived
     *                    class EtxHelloMessage to process ETX information,
     *                    otherwise false.
     * @return the number of bytes consumed in the input stream to produce
     * a decoded link tuple.
     * @throw InvalidLinkTuple if an invalid link tuple was found
     *        during message decoding.
     */
    virtual size_t decode_link_tuple(uint8_t* buf, size_t& len,
        size_t& skiplen, bool has_lq = false)
	throw(InvalidLinkTuple);

    inline size_t link_tuple_header_length() const {
	return	sizeof(uint8_t) + // link code
		sizeof(uint8_t) + // reserved
		sizeof(uint16_t); // link message size
    }

    TimeVal			_htime;		// hello interval
    OlsrTypes::WillType		_willingness;	// willingness-to-forward
    LinkBag			_links;		// link tuples
};

/**
 * @short Specialization of a HELLO message with ETX measurements.
 */
class EtxHelloMessage : public HelloMessage {
public:
    EtxHelloMessage()
	{ this->set_type(OlsrTypes::LQ_HELLO_MESSAGE); }
    ~EtxHelloMessage() {}

protected:
    size_t decode_link_tuple(uint8_t* buf, size_t& len,
			     size_t& skiplen, bool has_lq = true)
	throw(InvalidLinkTuple)
    {
	// Overriding a virtual with default arguments means the signatures
	// have to match. We are invoked via a pointer.
	return HelloMessage::decode_link_tuple(buf, len, skiplen, has_lq);
    }

    inline void add_link(const LinkCode code,
			 const IPv4& remote_addr,
			 const double& near_etx,
			 const double& far_etx) {
	HelloMessage::add_link(code,
			       LinkAddrInfo(remote_addr, near_etx,
					    far_etx));
    }
};

/**
 * @short Representation of a MID protocol message.
 */
class MidMessage : public Message {
public:
    MidMessage()
	{ this->set_type(OlsrTypes::MID_MESSAGE); }
    ~MidMessage() {}

    Message* decode(uint8_t* buf, size_t& len) throw(InvalidMessage);

    bool encode(uint8_t* buf, size_t& len);

    inline size_t	length() const {
	return	get_common_header_length() +
		(_interfaces.size() * IPv4::addr_bytelen());
    }

    inline void add_interface(const IPv4& addr) {
	_interfaces.push_back(addr);
    }

    inline void clear() { _interfaces.clear(); }

    inline const vector<IPv4>& interfaces() const { return _interfaces; }

    string str() const;

private:
    vector<IPv4>    _interfaces;
};

/**
 * @short Representation of a TC protocol message.
 */
class TcMessage : public Message {
public:
    TcMessage() { this->set_type(OlsrTypes::TC_MESSAGE); }
    ~TcMessage() {}

    virtual Message* decode(uint8_t* buf, size_t& len) throw(InvalidMessage);
    bool encode(uint8_t* buf, size_t& len);

    inline size_t	length() const {
	return	get_common_header_length() +
		sizeof(uint16_t) +	// ANSN
		sizeof(uint16_t) +	// Reserved
		( _neighbors.size() * sizeof(uint32_t)); // Neighbor
    }

    static inline size_t min_length() {
	return	get_common_header_length() +
		sizeof(uint16_t) +	// ANSN
		sizeof(uint16_t);	// Reserved
    }

    inline uint16_t	ansn() const { return _ansn; }
    inline const vector<LinkAddrInfo>&	neighbors() const {
	return _neighbors;
    }

    inline void add_neighbor(const IPv4& remote_addr) {
	add_neighbor(LinkAddrInfo(remote_addr));
    }

    inline size_t remove_neighbor(const IPv4& remote_addr) {
	size_t removed_count = 0;
	vector<LinkAddrInfo>::iterator ii = _neighbors.begin();
	while (ii != _neighbors.end()) {
	    if ((*ii).remote_addr() == remote_addr) {
		ii = _neighbors.erase(ii);
		++removed_count;
	    } else {
		++ii;
	    }
	}
	return removed_count;
    }

    inline void set_ansn(uint16_t ansn) { _ansn = ansn; }

    inline void clear() { _neighbors.clear(); _ansn = 0; }

    inline string str() const {
	string str = this->common_str();
	str += c_format("TC ansn %u ", XORP_UINT_CAST(ansn()));
	if (!_neighbors.empty()) {
	    vector<LinkAddrInfo>::const_iterator ii;
	    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++)
		str += (*ii).str() + " ";
	}
	return (str += '\n');
    }

protected:
    inline void add_neighbor(const LinkAddrInfo& lai) {
	_neighbors.push_back(lai);
    }

    void decode_tc_common(uint8_t* buf, size_t& len, bool has_lq = false)
	throw(InvalidMessage);

private:
    uint16_t		    _ansn;	// advertised neighbor sequence no.
    vector<LinkAddrInfo>    _neighbors; // advertised neighbor set.
};

/**
 * @short Specialization of a TC message with ETX measurements.
 */
class EtxTcMessage : public TcMessage {
public:
    EtxTcMessage() { this->set_type(OlsrTypes::LQ_TC_MESSAGE); }
    ~EtxTcMessage() {}

    inline void add_neighbor(const IPv4& remote_addr,
			     const double& near_etx,
			     const double& far_etx) {
	TcMessage::add_neighbor(LinkAddrInfo(remote_addr, near_etx, far_etx));
    }

    Message* decode(uint8_t* buf, size_t& len) throw(InvalidMessage);
};

/**
 * @short Representation of an HNA message, containing external routes.
 */
class HnaMessage : public Message {
public:
    HnaMessage()
	{ this->set_type(OlsrTypes::HNA_MESSAGE); }
    ~HnaMessage() {}

    Message* decode(uint8_t* buf, size_t& len) throw(InvalidMessage);
    bool encode(uint8_t* buf, size_t& len);

    inline size_t	length() const {
	return	get_common_header_length() +
		(_networks.size() *
		 (sizeof(uint32_t) +	// network address
		  sizeof(uint32_t)));	// network mask
    }

    inline const vector<IPv4Net>& networks() const { return _networks; }

    inline void add_network(const IPv4Net& network) {
	_networks.push_back(network);
    }

    inline size_t remove_network(const IPv4Net& network) {
	size_t removed_count = 0;
	vector<IPv4Net>::iterator ii = _networks.begin();
	while (ii != _networks.end()) {
	    if ((*ii) == network) {
		ii = _networks.erase(ii);
		++removed_count;
	    } else {
		++ii;
	    }
	}
	return removed_count;
    }

    inline void clear() { _networks.clear(); }

    inline string str() const {
	string str = this->common_str();
	str += "HNA ";
	if (!_networks.empty()) {
	    vector<IPv4Net>::const_iterator ii;
	    for (ii = _networks.begin(); ii != _networks.end(); ii++)
		str += ii->str() + " ";
	}
	return (str += "\n");
    }

private:
    vector<IPv4Net>	_networks;
};

/**
 * @short Wrapper class for an unknown OLSR message type.
 *
 * Used for passing messages to entities which need to deal with them
 * in an opaque way, e.g. Secured OLSR.
 */
class UnknownMessage : public Message {
public:
    Message* decode(uint8_t* buf, size_t& len) throw(InvalidMessage);

    bool encode(uint8_t* buf, size_t& len);

    inline size_t	length() const { return _msg.size(); }

    inline vector<uint8_t> opaque_data() { return this->_msg; }

    inline string str() const {
	string str = this->common_str() + "bytes ";
	vector<uint8_t>::const_iterator ii;
	for (ii = _msg.begin(); ii != _msg.end(); ii++)
	    str += c_format("0x%0x ", *ii);
	return (str += '\n');
    }

private:
    size_t	_opaque_data_offset;
};

/**
 * @short Decoder for OLSR protocol messages.
 *
 * Empty messages are registered with this class in a producer/consumer
 * pattern. When adding a new message class, make sure in its decode()
 * method that you are storing the message properties in the message
 * you've created -- not the fields of the empty message registered
 * with this class.
 */
class MessageDecoder {
public:
    ~MessageDecoder();

    Message* decode(uint8_t* ptr, size_t len) throw(InvalidMessage);

    void register_decoder(Message* message);

private:
    map<OlsrTypes::MessageType, Message* >  _olsrv1;
    UnknownMessage			    _olsrv1_unknown;
};

/**
 * @short An OLSR packet containing Messages.
 *
 * Packets contain Messages. They are coalesced up up to the available
 * MTU size, to save power on devices where transmission may have a high
 * energy cost.
 */
class Packet {
public:
    Packet(MessageDecoder& md, OlsrTypes::FaceID faceid = 0)
     : _message_decoder(md),
       _is_valid(false),
       _faceid(faceid),
       _seqno(0),
       _mtu(0)
    {}

    ~Packet() {}

    static size_t get_packet_header_length() {
	return	sizeof(uint16_t) +	// packet length
		sizeof(uint16_t);	// packet sequence number
    }

    /**
     * @return the size of the packet payload.
     */
    size_t length() const;

    /**
     * @return the amount of free space in this packet for an OLSR
     * packet payload, after MTU is taken into account.
     */
    size_t mtu_bound() const;

    /**
     * @return the size of the packet payload which will fit inside
     * the MTU without splitting any messages.
     * If no MTU is set, returns length().
     */
    size_t bounded_length() const;

    void decode(uint8_t* ptr, size_t len) throw(InvalidPacket);

    /**
     * Decode an OLSR packet header.
     *
     * An OLSR packet is considered valid if its length is greater than
     * the size of a standard packet header.
     * The host IP stack takes care of UDP checksums, IP fragmentation and
     * reassembly, and MTU size checks for us.
     */
    size_t decode_packet_header(uint8_t* ptr, size_t len)
	throw(InvalidPacket);

    /**
     * Encode a packet, including any nested messages.
     */
    bool encode(vector<uint8_t>& pkt);
    void update_encoded_seqno(vector<uint8_t>& pkt);

    inline uint16_t seqno() const { return _seqno; }
    inline void set_seqno(uint16_t seqno) { _seqno = seqno; }

    inline OlsrTypes::FaceID	faceid() const
	{ return _faceid; }

    inline uint32_t mtu() const { return _mtu; }
    inline void set_mtu(const uint32_t mtu) { _mtu = mtu; }

    inline void set_faceid(OlsrTypes::FaceID faceid)
	{ _faceid = faceid; }

    /**
     * @return a string representation of the entire packet.
     */
    string str() const;

    inline void add_message(Message* m) {
	_messages.push_back(m);
    }

    inline void clear() {
	_seqno = 0;
	_mtu = 0;
	_messages.clear();
	_is_valid = false;
    }

    inline const vector<Message*>& messages() { return _messages; }

    /**
     * Get a non-const reference to a packet's messages.
     * For debugging use only, ie in simulation of multiple hops.
     *
     * @return reference to the messages contained within the packet.
     */
    inline vector<Message*>& get_messages() { return _messages; }

    bool valid() const { return _is_valid; }

    vector<uint8_t>& get() { return _pkt; }

    void store(uint8_t* ptr, size_t len) {
	_pkt.resize(len);
	memcpy(&_pkt[0], ptr, len);
    }

private:
    MessageDecoder&	_message_decoder;
    bool		_is_valid;
    OlsrTypes::FaceID	_faceid;	// interface where this was received
    uint16_t		_seqno;
    uint32_t		_mtu;		// maximum transmission unit
    vector<Message*>	_messages;	// messages this packet contains.
    vector<uint8_t>	_pkt;		// on-wire packet data.
};

inline
void
initialize_message_decoder(MessageDecoder& message_decoder)
{
    //
    // NOTE: Do not register UnknownMessage explicitly -- it has no
    // type field to match. It is deliberately used from the message
    // parser so that we will forward opaque messages as per the OLSR RFC.
    //
    message_decoder.register_decoder(new HelloMessage());
    message_decoder.register_decoder(new TcMessage());
    message_decoder.register_decoder(new MidMessage());
    message_decoder.register_decoder(new HnaMessage());
#ifdef notyet
    message_decoder.register_decoder(new EtxHelloMessage());
    message_decoder.register_decoder(new EtxTcMessage());
#endif
}

#endif // __OLSR_MESSAGE_HH__
