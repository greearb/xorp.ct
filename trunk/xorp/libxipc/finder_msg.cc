// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/finder_msg.cc,v 1.4 2003/01/26 04:06:20 pavlin Exp $"

#include "config.h"

#include <algorithm>
#include <functional>
#include <string>

#include "finder_module.h"
#include "libxorp/xorp.h"
#include "libxorp/c_format.hh"
#include "libxorp/callback.hh"
#include "libxorp/debug.h"

#include "hmac.hh"
#include "finder_msg.hh"

static const uint32_t FINDER_PROTO_MAJOR = 1;
static const uint32_t FINDER_PROTO_MINOR = 1;

static const string msg_header("Finder/%u.%u src=%u seqno=%u payload=%u %s\n");
static const string smallest_msg = c_format(msg_header.c_str(),
					    0, 0, 0, 0, 0, "");

/* ------------------------------------------------------------------------- */
/* FinderMessage code */

const string
FinderMessage::render_header(size_t payload_bytes) const
{
    return c_format(msg_header.c_str(),
		    FINDER_PROTO_MAJOR, FINDER_PROTO_MINOR,
		    srcid(), seqno(), payload_bytes, name().c_str());
};

const string
FinderMessage::render_signature(const HMAC& k, const string& header) const
{
    // NB do not edit this without editing FinderParser::parse_signature
    string sig = k.signature(header);
    return sig + string("\n");
}

const string
FinderMessage::render() const
{
    const string payload = render_payload();
    const string header = render_header(payload.size());
    return header + payload;
}

const string
FinderMessage::render_signed(const HMAC& k) const
{
    const string payload = render_payload();
    const string header = render_header(payload.size() +
					k.signature_size() + 1);
    const string signature = render_signature(k, header);
    return header + signature + payload;
}

/* ------------------------------------------------------------------------- */
/* FinderParser helper routines */

bool
skip_word(string::const_iterator& str_pos,
	  const string& word)
{
    string::const_iterator spos = str_pos;
    string::const_iterator wpos = word.begin();

    while (wpos != word.end()) {
	if (*wpos != *spos)
	    return false;
	wpos++; spos++;
    }
    str_pos = spos;
    return true;
}

inline size_t
skip_equal(string::const_iterator& a,
	   string::const_iterator& b)
{
    size_t matching = 0;
    for (;*a && (*a == *b); a++, b++) {
	matching++;
    }
    return matching;
}

template <class _Predicate> inline const string
get_chars(string::const_iterator& spos, _Predicate chrcls)
{
    string::const_iterator begin = spos;
    while (*spos && chrcls(*spos))
	spos++;
    return string(begin, spos);
}

template <class _Predicate> inline void
skip_chars(string::const_iterator& spos, _Predicate chrcls)
{
    while (*spos && chrcls(*spos))
	spos++;
}

//
// Wrapper functions because the system implementation may not use functions.
//
static int
my_isdigit(int c)
{
    return isdigit(c);
}

static int
my_isalpha(int c)
{
    return isalpha(c);
}

static int
my_iscntrl(int c)
{
    return iscntrl(c);
}

inline bool
get_uint32(string::const_iterator& spos, uint32_t& r)
{
    string u32 = get_chars(spos, ptr_fun(my_isdigit));

    r = 0;
    for (string::const_iterator i = u32.begin(); i != u32.end(); i++) {
	r *= 10;
	r += (*i) - '0';
    }
    return u32.empty() == false;
}

inline bool
get_alphaword(string::const_iterator& spos, string& r)
{
    r = get_chars(spos, ptr_fun(my_isalpha));
    return r.empty() == false;
}

inline bool
get_line(string::const_iterator& spos, string& r)
{
    r = get_chars(spos, not1(ptr_fun(my_iscntrl)));
    return r.empty() == false;
}

inline bool
get_all(string::const_iterator& spos, string& r)
{
    string::const_iterator s = spos;
    while (*spos) spos++;
    r = string(s, spos);
    return r.empty() == false;
}

/* ------------------------------------------------------------------------- */
/* FinderParser routines */

bool
FinderParser::parse_header(string::const_iterator& buffer_pos,
			   uint32_t& major, uint32_t& minor,
			   uint32_t& src, uint32_t& seqno,
			   uint32_t& payload_bytes, string& msg_name)
{
    string::const_iterator scan_pos = msg_header.begin();
    string::const_iterator b_pos = buffer_pos;

    if (skip_equal(scan_pos, b_pos) == 0 ||
	skip_word(scan_pos, "%u") == false ||
	get_uint32(b_pos, major) == false) {
	debug_msg("major\n");
	return false;
    }
    if (skip_equal(scan_pos, b_pos) == 0 ||
	skip_word(scan_pos, "%u") == false ||
	get_uint32(b_pos, minor) == false) {
	debug_msg("minor\n");
	return false;
    }
    if (skip_equal(scan_pos, b_pos) == 0 ||
	skip_word(scan_pos, "%u") == false ||
	get_uint32(b_pos, src) == false) {
	debug_msg("src\n");
	return false;
    }
    if (skip_equal(scan_pos, b_pos) == 0 ||
	skip_word(scan_pos, "%u") == false ||
	get_uint32(b_pos, seqno) == false) {
	debug_msg("seqno\n");
	return false;
    }
    if (skip_equal(scan_pos, b_pos) == 0 ||
	skip_word(scan_pos, "%u") == false ||
	get_uint32(b_pos, payload_bytes) == false) {
	debug_msg("payload_bytes\n");
	return false;
    }
    if (skip_equal(scan_pos, b_pos) == 0 ||
	skip_word(scan_pos, "%s") == false ||
	get_alphaword(b_pos, msg_name) == false) {
	debug_msg("name\n");
	return false;
    }
    if (skip_equal(scan_pos, b_pos) == 0) {
	debug_msg("skip_equal\n");
	return false;
    }
    buffer_pos = b_pos;
    return true;
}

ssize_t
FinderParser::peek_header_bytes(const string& header)
    throw (BadFinderMessage)
{
    string::size_type pos = header.find('\n');
    if (pos == string::npos)
	return -1;
    if (pos < smallest_msg.size())
	xorp_throw(BadFinderMessage, c_format("Header is too small (%u < %u)",
					      (uint32_t)pos,
					      (uint32_t)smallest_msg.size()));
    return (ssize_t)pos + 1;
}

size_t
FinderParser::max_header_bytes()
{
    return 250;
}

size_t
FinderParser::peek_payload_bytes(const string& header)
    throw (BadFinderMessage)
{
    string::const_iterator h_pos = header.begin();

    uint32_t major, minor, src, seqno, pbytes;
    string name;

    if (parse_header(h_pos, major, minor, src, seqno, pbytes, name) == false)
	xorp_throw(BadFinderMessage,
		   c_format("Junk Header >>%s\n", header.c_str()));
    return pbytes;
}

inline bool
FinderParser::parse_signature(const HMAC*		hmac,
			      const string&		header,
			      const string&		/* buf */,
			      string::const_iterator&	bp) const
{
    string sig = hmac->signature(header) + string("\n");
    return skip_word(bp, sig);
}

bool
FinderParser::parse_payload(uint32_t			src,
			    uint32_t			seqno,
			    const string&		msgname,
			    const string&		buf,
			    string::const_iterator& 	buf_pos) const
{
    PEList::const_iterator pi;
    const FinderParselet* p;
    for (pi = _parsers.begin(); pi != _parsers.end(); ++pi) {
	// pi is an iterator to a ref_ptr wrappered object, get object
	// since both iterator and ref_ptr classes overload operator->
	p = pi->get();
	if (p->name() == msgname) {
	    p->parse(src, seqno, buf, buf_pos);
	    return true;
	}
    }
    return false;
}

size_t
FinderParser::parse(const HMAC*			hmac,
		    const string&		b,
		    string::const_iterator&	bpos) const
    throw (BadFinderMessage)
{
    const string::const_iterator start = bpos;

    uint32_t major, minor, src, seqno, pbytes;
    string msgname;

    if (parse_header(bpos, major, minor, src, seqno, pbytes, msgname) == false)
	xorp_throw(BadFinderMessage,
		   c_format("Junk Header >>%s\n", string(start, bpos).c_str()));

    if (hmac) {
	const string::const_iterator hmac_start = bpos;
	if (parse_signature(hmac, string(start, bpos), b, bpos) == false)
	    xorp_throw(BadFinderMessage, string("Bad HMAC signature"));
	pbytes -= (bpos - hmac_start); // compensate for hmac header
    }
    if (bpos + pbytes > b.end())
	xorp_throw(BadFinderMessage, c_format("Truncated message (%d < %d)",
					      b.end() - bpos, pbytes));

    if (parse_payload(src, seqno, msgname, b, bpos) == false) {
	// skip payload data as no sub-parser installed for packet type
	bpos += pbytes;
    }

    return (bpos - start);
}

bool
FinderParser::add(const ParsingElement& pe)
{
    _parsers.push_back(pe);
    return true;
}

/* ------------------------------------------------------------------------- */
/* HELLO related */

const string
FinderHello::render_payload() const
{
    return string("");
}

void
FinderHelloParser::parse(uint32_t src,
			 uint32_t seqno,
			 const string&, string::const_iterator&) const
    throw (BadFinderMessage)
{
    FinderHello hello(src, seqno);
    _cb->dispatch(hello);
}

/* ------------------------------------------------------------------------- */
/* BYE related */

const string
FinderBye::render_payload() const
{
    return string("");
}

void
FinderByeParser::parse(uint32_t src,
		       uint32_t seqno,
		       const string&, string::const_iterator&) const
    throw (BadFinderMessage)
{
    FinderBye bye(src, seqno);
    _cb->dispatch(bye);
}

/* ------------------------------------------------------------------------- */
/* ACK related */

const string
FinderAck::render_payload() const
{
    return c_format("%u", _ack);
}

void
FinderAckParser::parse(uint32_t src,
		       uint32_t seqno,
		       const string&, string::const_iterator& bpos) const
    throw (BadFinderMessage)
{
    uint32_t ackno;

    if (get_uint32(bpos, ackno) == false)
	xorp_throw(BadFinderMessage, "not an ack");

    FinderAck ack(src, seqno, ackno);
    _cb->dispatch(ack);
}

/* ------------------------------------------------------------------------- */
/* Associate related */

const string
FinderAssociate::render_payload() const
{
    return _t_and_m + string("\n") + _a;
}

void
FinderAssociateParser::parse(uint32_t src,
			     uint32_t seqno,
			     const string&, string::const_iterator& bpos) const
    throw (BadFinderMessage)
{
    string t_and_m;
    if (get_line(bpos, t_and_m) == false)
	xorp_throw(BadFinderMessage, "Bad target_and_method");

    bpos++;
    string a;
    if (get_all(bpos, a) == false)
	xorp_throw(BadFinderMessage, "Missing association");

    FinderAssociate assoc(src, seqno, t_and_m, a);
    _cb->dispatch(assoc);
}

