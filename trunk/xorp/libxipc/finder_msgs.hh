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

// $XORP: xorp/libxipc/finder_msgs.hh,v 1.2 2003/02/26 19:46:03 hodson Exp $

#ifndef __LIBXIPC_FINDER_MSGS_HH__
#define __LIBXIPC_FINDER_MSGS_HH__

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"

#include "xrl.hh"
#include "xrl_error.hh"

/**
 * Base class for FinderMessage classes.
 */
class FinderMessageBase {
public:
    /**
     * Construct a message.  Initialized string representation with
     * filled in header fields.  Child constructors add their own
     * payload representation.
     *
     * @param seqno The sequence number to be associated with the message
     * @param type  The message type.
     */
    FinderMessageBase(uint32_t seqno, char type);

    virtual ~FinderMessageBase();

    /**
     * String render method.  This method provides 
     */
    const string& str() const { return _rendered; }

protected:
    // String manipluated by base and child classes to provide serializable
    // representation.
    string   _rendered;	

    static const char* c_msg_template;

    friend class ParsedFinderMessageBase;
};

/**
 * Finder Message class for Xrl transport.
 */
class FinderXrlMessage : public FinderMessageBase {
public:
    FinderXrlMessage(const Xrl& xrl);
    inline uint32_t seqno() const { return _seqno; }
    
protected:
    uint32_t _seqno;
    static const char c_type = 'x';
    static uint32_t c_seqno;
    static const char* c_msg_template;

    friend class ParsedFinderXrlMessage;
};

/**
 * Finder Message class for Xrl Response transport.
 */
class FinderXrlResponse : public FinderMessageBase {
public:
    FinderXrlResponse(uint32_t seqno, const XrlError& e, const XrlArgs*);
    
protected:
    static const char c_type = 'r';
    static const char* c_msg_template;

    friend class ParsedFinderXrlResponse;
};

/**
 * Exception for badly formatted message data.
 */
struct BadFinderMessageFormat : public XorpReasonedException {
    BadFinderMessageFormat(const char* file, size_t line, const string& why)
	: XorpReasonedException("BadFinderMessageFormat", file, line, why)
    {}
};

/**
 * Exception for mismatched finder message type.
 */
struct WrongFinderMessageType : public XorpException {
    WrongFinderMessageType(const char* file, size_t line)
	: XorpException("WrongFinderMessageType", file, line)
    {}
};

/**
 * Base class for parsed Finder Messages.
 */
class ParsedFinderMessageBase {
public:

    /**
     * Constructor
     *
     * Initializes common header fields of seqno and type from string
     * representation and provides accessors to these values.
     *
     */
    ParsedFinderMessageBase(const char* data, char type)
	throw (BadFinderMessageFormat, WrongFinderMessageType);

    virtual ~ParsedFinderMessageBase();
    
    uint32_t seqno() const { return _seqno; }
    char type() const { return _type; }

protected:
    uint32_t bytes_parsed() const { return _bytes_parsed; }

    uint32_t _seqno;
    char     _type;
    uint32_t _bytes_parsed;

    static const char* c_msg_template;
};

/**
 * Parses finder protocol messages into Xrls.
 */
class ParsedFinderXrlMessage : public ParsedFinderMessageBase {
public:
    /**
     * Constructor for received Xrl messages.
     * Attempts to extract Xrl from data.
     *
     * @throws BadFinderMessageFormat when bad packet data received.
     * @throws WrongFinderMessageType if message is not a Finder Xrl message.
     * @throws InvalidString if the data within the Xrl Message could not be
     *         rendered as an Xrl.
     */
    ParsedFinderXrlMessage(const char* data)
	throw (BadFinderMessageFormat, WrongFinderMessageType, InvalidString);

    ~ParsedFinderXrlMessage();

    const Xrl& xrl() const { return *_xrl; }

private:
    Xrl* _xrl;
};

/**
 * Parses finder protocol messages into Xrl responses.
 */
class ParsedFinderXrlResponse : public ParsedFinderMessageBase {
public:
    /**
     * Constructor for received Xrl Response messages.
     * Attempts to extract Xrl Reponse from data.
     *
     * @throws BadFinderMessageFormat when bad packet data received.
     * @throws WrongFinderMessageType if message is not a Finder Xrl Response
     *         Message.
     * @throws InvalidString if the data within the Xrl Response
     *         message could not be rendered as an Xrl.
     */
    ParsedFinderXrlResponse(const char* data)
	throw (BadFinderMessageFormat, WrongFinderMessageType, InvalidString);

    ~ParsedFinderXrlResponse();
    
    const XrlError& xrl_error() const { return _xrl_error; }
    XrlArgs* xrl_args() const { return _xrl_args; }

private:
    XrlError _xrl_error;
    XrlArgs* _xrl_args;
};

#endif // __LIBXIPC_FINDER_MSGS_HH__
