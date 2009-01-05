// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/finder_msgs.hh,v 1.12 2008/10/02 21:57:20 bms Exp $

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
    uint32_t seqno() const { return _seqno; }
    
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
