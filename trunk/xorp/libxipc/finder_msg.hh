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

// $XORP: xorp/libxipc/finder_msg.hh,v 1.3 2003/03/10 23:20:23 hodson Exp $

#ifndef __IPC_FINDER_MSG_HH__
#define __IPC_FINDER_MSG_HH__

#include <list>
#include <string>

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"
#include "libxorp/callback.hh"
#include "hmac.hh"

/**
 * @short Base class for Finder Protocol messages.
 *
 * Provides the functionality common to all message types: sequencing,
 * source identifier, authentication, and rendering.
 */
struct FinderMessage {

    FinderMessage(uint32_t src, uint32_t seq, const string& name)
	: _src(src), _seq(seq), _name(name) {}
    virtual ~FinderMessage() {}

    typedef ref_ptr<FinderMessage> RefPtr;

    /**
     * Create a string containing ascii packet data.
     */
    const string render() const;
    /**
     * Create a string containing ascii packet data that includes an
     * authentication header.
     *
     * @param h header authentication object.
     */
    const string render_signed(const HMAC& h) const;

    /**
     * @return source identifier of message.
     */
    inline uint32_t srcid() const { return _src; }

    /**
     * @return sequence number of message.
     */
    inline uint32_t seqno() const { return _seq; }

    /**
     * set sequence number of message.
     */
    inline void set_seqno(uint32_t s) { _seq = s; }

    /**
     * @return message name (i.e. 'hello', 'bye')
     */
    inline const string& name() const { return _name; }

protected:
    /**
     * @return HMAC signature of message header.
     */
    const string render_signature(const HMAC& h, const string& msg_header)
	const;

    /**
     * @return Ascii representation of header.
     */
    const string render_header(size_t payload_bytes) const;

    /**
     * FinderMessage sub-classes must implement this method.
     *
     * @return Ascii representation of data associated with packet.
     */
    virtual const string render_payload() const = 0;

protected:
    uint32_t	_src;
    uint32_t	_seq;
    string	_name;
};


/**
 * Exception class thrown by FinderMessage parsing routines when
 * things go wrong.
 */
class BadFinderMessage : public XorpException {
public:
    BadFinderMessage(const char* file, size_t line, const string& why)
	: XorpException("BadFinderMessage", file, line), _why(why) {}
    const string why() const {
	return (_why.empty()) ? string("Not Specified") : _why;
    }
protected:
    const string _why;
};

/**
 * FinderParselet is the base class for mini-parsers that handle the
 * payload data of Finder messages.  They parse the payload of the
 * particular message type they understand and then call a user
 * specified callback.
 */
struct FinderParselet {
    /**
     * Construct a payload parser.
     * @param name the message type understood by the parser.
     */
    FinderParselet(const string& name) : _name(name) {}

    virtual ~FinderParselet() {}

    /**
     * Parse payload of message.
     *
     * @param src Source identifier taken from common message header.
     * @param seq Sequence number taken from common message header.
     * @param b Buffer that contains data to be parsed.
     * @param bpos position that payload parsing should be commenced from.
     * This value should be updated to point to the last character parsed.
     */
    virtual void parse(uint32_t src, uint32_t seq, const string& b,
		       string::const_iterator& bpos) const
	throw (BadFinderMessage) = 0;

    /**
     * @return name of message type understood by parselet.
     */
    const string name() const { return _name; }
protected:
    string _name;
};

/**
 * Parser of finder messages.  Parses common header and passes payload
 * data to registered ParsingElements that invoke user callbacks when
 * packets of given type arrive.
 *
 * Sub-parsers are added with the @ref add method.
 * add takes a ParsingElement as an argument.  These are
 * created by helper functions like hello_notifier, eg:
 *
 <pre>

void hello_handler(const FinderHello& msg) {
	... do something with hello message
}

FinderParser fp;

fp.add(hello_notifier(callback(hello_handler)));

</pre>
*/
struct FinderParser {
    typedef ref_ptr<const FinderParselet> ParsingElement;
    typedef list<ParsingElement> PEList;

    /**
     * Attempts to determine if the supplied string looks like a
     * FinderMessage header.
     * @param header candidate finder message header.
     * @return header size if valid, 0 if there is no enough data to
     * determine if it is a valid header.  Throws an exception is data
     * does not look like a FinderMessage header.
     */
    static ssize_t peek_header_bytes(const string& header)
	throw (BadFinderMessage);

    /**
     * @return maximum header size (actually size can be certain not
     * a valid header).
     */
    static size_t max_header_bytes();

    /**
     * @return the size of the payload data.
     */
    static size_t peek_payload_bytes(const string& header)
	throw (BadFinderMessage);

    /**
     * Parses message and calls callbacks if message type has a
     * registered notifier.
     *
     * @param buf buffer to be parsed.
     * @param buf_pos starting position of parsing.  Updated to point to
     * end of parsing upon return.
     */
    inline size_t
    parse(const string& buf, string::const_iterator& buf_pos) const
	throw (BadFinderMessage) {
	return parse(0, buf, buf_pos);
    }

    /**
     * Parses a message using HMAC and calls callbacks if message type
     * has a registered notifier.
     *
     * @param h HMAC instance to be used to check header authentication.
     * @param buf buffer to be parsed.
     * @param buf_pos starting position of parsing.  Updated to point
     * to end of parsing upon return.
     */
    inline size_t
    parse_signed(const HMAC& h, const string& buf,
		 string::const_iterator& buf_pos) const
	throw (BadFinderMessage) {
	return parse(&h, buf, buf_pos);
    }

    /**
     * Parses a message using HMAC and calls callbacks if message type
     * has a registered notifier.
     *
     * @param h pointer to HMAC to be used to check header authentication.
     * @param buf buffer to be parsed.
     * @param buf_pos starting position of parsing.  Updated to point
     * to end of parsing upon return.
     */
    size_t parse(const HMAC* h, const string& b, string::const_iterator& bp)
	const throw (BadFinderMessage);

    /**
     * Add user callback for a particular message type.  Used in
     * conjunction with ParsingElement generating functions (ie
     * @ref notifier and it similarly named brethren).
     */
    bool add(const ParsingElement& pe);

protected:
    static bool parse_header(string::const_iterator&	buffer_pos,
		      uint32_t&				major,
		      uint32_t&				minor,
		      uint32_t&				src,
		      uint32_t&				seq,
		      uint32_t&				payload_bytes,
		      string&				msgname);

    bool parse_payload(uint32_t				src,
		       uint32_t				seq,
		       const string&			msgname,
		       const string&			buf,
		       string::const_iterator&		buf_pos) const;

    inline bool parse_signature(const HMAC*		h,
				const string&		header,
				const string&		b,
				string::const_iterator& bp) const;


    PEList _parsers;
};

// ----------------------------------------------------------------------------

/**
 * HELLO message.
 */
struct FinderHello : public FinderMessage {
    FinderHello(uint32_t src, uint32_t seq) : FinderMessage(src, seq, "hello") {}
    const string render_payload() const;
};

/**
 * HELLO message parser.
 */
struct FinderHelloParser : public FinderParselet {
    typedef XorpCallback1<void, const FinderHello&>::RefPtr Callback;

    FinderHelloParser(const Callback& cb)
	: FinderParselet("hello"), _cb(cb) {}

    void parse(uint32_t src, uint32_t seq,
	       const string&, string::const_iterator&) const
	throw (BadFinderMessage);
protected:
    Callback _cb;
};

/**
 * ParsingElement generating function to hook user callbacks when
 * HELLO messages are received.  Used in conjunction with
 * @ref FinderParser::add.
 */
inline FinderParser::ParsingElement
hello_notifier(const FinderHelloParser::Callback& cb)
{
    return FinderParser::ParsingElement(new FinderHelloParser(cb));
}

// ----------------------------------------------------------------------------

/**
 * BYE Mesasge.
 */
struct FinderBye : public FinderMessage {
    FinderBye(uint32_t src, uint32_t seq) : FinderMessage(src, seq, "bye") {}
    const string render_payload() const;
};

/**
 * BYE Message Parser.
 */
struct FinderByeParser : public FinderParselet {
    typedef XorpCallback1<void, const FinderBye&>::RefPtr Callback;

    FinderByeParser(const Callback& cb)
	: FinderParselet("bye"), _cb(cb) {}

    void parse(uint32_t src, uint32_t seq,
	       const string&, string::const_iterator&) const
	throw (BadFinderMessage);

protected:
    Callback _cb;
};

/**
 * ParsingElement generating function to hook user callbacks when
 * BYE messages are received.  Used in conjunction with
 * @ref FinderParser::add.
 */
inline FinderParser::ParsingElement
bye_notifier(const FinderByeParser::Callback& cb)
{
    return FinderParser::ParsingElement(new FinderByeParser(cb));
}

// ----------------------------------------------------------------------------

/**
 * ACK Mesasge.
 */
struct FinderAck : public FinderMessage {
    FinderAck(uint32_t src, uint32_t seq, uint32_t acking_no)
	: FinderMessage(src, seq, "ack"), _ack(acking_no) {}
    const string render_payload() const;
protected:
    uint32_t _ack;
};

/**
 * ACK Message Parser.
 */
struct FinderAckParser : public FinderParselet {
    typedef XorpCallback1<void, const FinderAck&>::RefPtr Callback;

    FinderAckParser(const Callback& cb) : FinderParselet("ack"), _cb(cb) {}

    void parse(uint32_t src, uint32_t seq,
	       const string& buf, string::const_iterator& buf_pos) const
	throw (BadFinderMessage);

protected:
    Callback _cb;
};

/**
 * ParsingElement generating function to hook user callbacks when
 * ACK messages are received.  Used in conjunction with
 * @ref FinderParser::add.
 */
inline FinderParser::ParsingElement
ack_notifier(const FinderAckParser::Callback& cb)
{
    return FinderParser::ParsingElement(new FinderAckParser(cb));
}

// ----------------------------------------------------------------------------

/**
 * ASSOCIATE Mesasge.
 */
struct FinderAssociate : public FinderMessage {

    FinderAssociate(uint32_t		src,
		    uint32_t		seq,
		    const string&	t_and_m,	// target and method
		    const string&	a)		// associated value
	: FinderMessage(src, seq, "associate"), _t_and_m(t_and_m), _a(a) {}

    const string render_payload() const;
protected:
    const string _t_and_m;
    const string _a;
};

/**
 * ASSOCIATE Message Parser.
 */
struct FinderAssociateParser : public FinderParselet {
    typedef XorpCallback1<void, const FinderAssociate&>::RefPtr Callback;

    FinderAssociateParser(const Callback& cb)
	: FinderParselet("associate"), _cb(cb) {}

    void parse(uint32_t src, uint32_t seqno,
	       const string&, string::const_iterator&) const
	throw (BadFinderMessage);
protected:
    Callback _cb;
};

/**
 * ParsingElement generating function to hook user callbacks when
 * ASSOCIATE messages are received.  Used in conjunction with
 * @ref FinderParser::add.
 */
inline FinderParser::ParsingElement
associate_notifier(const FinderAssociateParser::Callback& cb)
{
    return FinderParser::ParsingElement(new FinderAssociateParser(cb));
}

// ----------------------------------------------------------------------------

/**
 * QUESTION Mesasge.
 */
struct FinderQuestion : public FinderMessage {
    FinderQuestion(uint32_t src, uint32_t seq, const string& target_and_method) :
	FinderMessage(src, seq, "question"), _t_m(target_and_method) {}
    const string render_payload() const;
protected:
    const string _t_m;
};

/**
 * QUESTION Message Parser.
 */
struct FinderQuestionParser : FinderParselet {
    typedef XorpCallback1<void, const FinderQuestion&>::RefPtr Callback;
    FinderQuestionParser(const Callback& cb)
	: FinderParselet("question"), _cb(cb) {}
    void parse(uint32_t src, uint32_t seqno,
	       const string& buf, string::const_iterator& buf_pos) const
	throw (BadFinderMessage);
protected:
    Callback _cb;
};

/**
 * ParsingElement generating function to hook user callbacks when
 * QUESTION messages are received.  Used in conjunction with
 * @ref FinderParser::add.
 */
inline FinderParser::ParsingElement
question_notifier(const FinderQuestionParser::Callback& cb)
{
    return FinderParser::ParsingElement(new FinderQuestionParser(cb));
}

// ----------------------------------------------------------------------------

/**
 * ANSWER Mesasge.
 */
struct FinderAnswer : public FinderMessage {
    FinderAnswer(uint32_t src, uint32_t seq, const string& target_and_method) :
	FinderMessage(src, seq, "answer"), _t_m(target_and_method) {}
protected:
    const string _t_m;
};

/**
 * ANSWER Message Parser.
 */
struct FinderAnswerParser : FinderParselet {
    typedef XorpCallback1<void, const FinderAnswer&>::RefPtr Callback;
    FinderAnswerParser(const Callback& cb)
	: FinderParselet("answer"), _cb(cb) {}
    void parse(uint32_t src, uint32_t seqno,
	       const string& buf, string::const_iterator& buf_pos) const
	throw (BadFinderMessage);
protected:
    Callback _cb;
};

/**
 * ParsingElement generating function to hook user callbacks when
 * ANSWER messages are received.  Used in conjunction with
 * @ref FinderParser::add.
 */
inline FinderParser::ParsingElement
answer_notifier(const FinderAnswerParser::Callback& cb)
{
    return FinderParser::ParsingElement(new FinderAnswerParser(cb));
}

#endif // __IPC_FINDER_MSG_HH__
